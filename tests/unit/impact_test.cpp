/**
 * @file
 * Impact telemetry acceptance test.
 *
 * Verifies that enabling impact tracking never perturbs physics, that impact
 * energy scales quadratically with closing speed, that marks land on the
 * correct local surface and follow the body transform, that history
 * hold/fade/clear behaves, that wall marks land on the glass plane, and that
 * resting contacts do not accumulate heat. If a CUDA device is present, the
 * CPU and CUDA peaks must agree within the test's tolerance.
 */

#include "settings/collisions/constants.h"
#include "physics/scenarios/impact_demo.h"
#include "client/render_gl/impact_history.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace {

constexpr int SEVERITY_BODIES = 3;
constexpr int PROBE_BODY = 4;
constexpr int TELEMETRY_TICKS = 120;

/**
 * Per-hull event capacity, aliased from the simulation's published event
 * budget so the flat impact layout here cannot drift against a second
 * hand-written value. Every body reserves two hull slots so it can report
 * against more than one collision surface in the same tick; this test only
 * inspects the first slot.
 */
constexpr int EVENTS_PER_HULL = settings::Collisions::Impacts::EVENTS_PER_BODY;

/**
 * Abort the current test when the condition is false.
 *
 * @param condition Checked precondition; a false value throws.
 * @param message   Diagnostic message attached to the thrown exception.
 */
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

/**
 * Flattened index into the backend impact buffer for the event-th impact on
 * the hull-th sub-body of body.
 *
 * @param body  Body index within the impact buffer.
 * @param hull  Sub-body index within the body (0 or 1).
 * @param event Impact slot within the hull.
 * @return Linear index into the backend impact array.
 */
std::size_t impactIndex(int body, int hull, int event) {
    constexpr int HULLS_PER_BODY = 2;
    return static_cast<std::size_t>((body * HULLS_PER_BODY + hull) * EVENTS_PER_HULL + event);
}

/**
 * Per-body peak impact energies gathered across the severity ladder.
 */
struct SeverityResult {
    std::array<float, SEVERITY_BODIES> peaks{};
    bool anyImpact = false;
};

/**
 * Advance two identical scenes in lockstep - one tracking impacts, one
 * silent. Their state hashes must match every tick. Each emitted impact is
 * checked against the expected local surface, and per-body peaks are captured.
 *
 * @param scene    Scenario world, seed-derived surface state and time step.
 * @param tracked  State advanced by the tracking backend.
 * @param baseline State advanced by the silent backend.
 * @param tracker  Backend with impact tracking enabled.
 * @param silent   Backend without impact tracking.
 * @param history  Impact history that ages tracked marks each tick.
 * @return Peak energies and whether any impact was observed.
 */
template <typename Scene, typename Tracker, typename Silent>
SeverityResult runSeverityLadder(
    const Scene& scene, sim::State& tracked, sim::State& baseline,
    Tracker& tracker, Silent& silent,
    render::ImpactHistory& history
) {
    constexpr float MIN_CLOSING_MPS = 0.5f;

    SeverityResult result;

    for (int tick = 0; tick < TELEMETRY_TICKS; ++tick) {
        tracker.step(tracked, scene.world);
        tracker.download(tracked);

        silent.step(baseline, scene.world);
        silent.download(baseline);

        require(sim::hashState(tracked) == sim::hashState(baseline), "Impact telemetry changed physics motion");

        history.advance(tracked, tracker.impacts(), scene.world.dt_s);

        for (int body = 0; body < SEVERITY_BODIES; ++body) {
            const auto bodyIndex = static_cast<std::size_t>(body);

            for (int slot = 0; slot < EVENTS_PER_HULL; ++slot) {
                const auto& event = tracker.impacts()[impactIndex(body, /*hull=*/0, slot)];

                if (event.energy_J <= 0.f)
                    continue;

                result.anyImpact = true;
                result.peaks[bodyIndex] = std::max(result.peaks[bodyIndex], event.energy_J);

                require(event.impulse_Ns > 0.f && event.closing_mps >= MIN_CLOSING_MPS, "Unconfirmed impact");
                require(
                    event.localNormal.z > 0.99f && std::abs(event.localPoint_m.z - 1.1f) < 0.03f,
                    "Impact point/normal is not on the struck local surface");
            }
        }
    }

    return result;
}

/**
 * Confirm a fresh violent impact produces a red mark whose local point
 * round-trips through the body transform unchanged (position + orientation).
 *
 * @param tracked State that produced the recorded impacts.
 * @param history History whose retained mark is validated.
 */
void verifyPatchFollowsBody(const sim::State& tracked, render::ImpactHistory& history) {
    constexpr float RED_STRENGTH_THRESHOLD = 0.8f;

    const auto patch = history.body(PROBE_BODY)[0];
    require(patch.energy_J > 0.f && history.strength(patch) > RED_STRENGTH_THRESHOLD, "Violent impact should be red");

    const auto& position = tracked.position_m[PROBE_BODY];
    const auto& orientation = tracked.orientation[PROBE_BODY];

    const auto worldPoint = position + sim::rotate(orientation, patch.point_m);
    const auto local = sim::rotate(sim::conjugate(orientation), worldPoint - position);

    require(sim::length(local - patch.point_m) < 0.0001f, "Mark does not follow transformed body");
}

/**
 * Verify that hold freezes marks, releasing it lets them age out, and clear
 * wipes them and zeroes the last-energy stat.
 *
 * @param tracked State passed through to history advancement.
 * @param history History whose hold/fade/clear behaviour is validated.
 */
void verifyHistoryRetentionAndFade(const sim::State& tracked, render::ImpactHistory& history) {
    constexpr float HISTORY_ADVANCE_S = 20.f;

    history.hold = true;
    history.advance(tracked, {}, HISTORY_ADVANCE_S);
    require(history.body(PROBE_BODY)[0].energy_J > 0.f, "Hold failed to keep marks");

    history.hold = false;
    history.advance(tracked, {}, HISTORY_ADVANCE_S);
    require(history.body(PROBE_BODY)[0].energy_J == 0.f, "Expired impact did not fade out");

    history.clear();
    require(history.lastEnergy_J == 0.f, "Reset retains stale impact");
}

/**
 * Drive a fast sphere at x = 6.64 into the glass wall at x = 7. The strike
 * must register on wall face 1, with the mark lying exactly on the wall plane.
 *
 * @param cuda  Whether to run on the CUDA backend.
 * @param world Scenario world providing gravity and time step.
 */
template <typename World>
void verifyWallImpactMark(bool cuda, const World& world) {
    sim::State state(1);

    sim::Body sphere;
    sphere.position_m = {6.64f, 0.f, 0.f};
    sphere.velocity_mps = {5.f, 0.f, 0.f};
    state.spawn(sphere);

    auto backend = cuda ? sim::createCudaBackend(1) : sim::createCpuBackend(1);
    backend->setImpactTracking(true);
    backend->upload(state);
    backend->step(state, world);
    backend->download(state);

    const auto& hit = backend->impacts()[0];

    require(hit.wallFace_i == 1 && hit.energy_J > 0.f, "Missing glass wall impact");
    require(std::abs(hit.wallPoint_m.x - 7.f) < 0.0001f, "Wall mark not on wall plane");
}

/**
 * Confirm a sphere resting on the floor under normal gravity produces contact
 * impulses every tick but never reports new impact energy, otherwise the heat
 * map would saturate on any resting object.
 *
 * @param cuda  Whether to run on the CUDA backend.
 * @param world Scenario world whose gravity is overridden for the test.
 */
template <typename World>
void verifyRestingContactIsFiltered(bool cuda, const World& world) {
    sim::State state(1);

    sim::Body sphere;
    sphere.position_m = {0.f, -3.65f, 0.f};
    sphere.velocity_mps = {};
    state.spawn(sphere);

    auto restingWorld = world;
    restingWorld.gravity_mps2 = 9.81f;

    auto backend = cuda ? sim::createCudaBackend(1) : sim::createCpuBackend(1);
    backend->setImpactTracking(true);
    backend->upload(state);

    for (int tick = 0; tick < TELEMETRY_TICKS; ++tick) {
        backend->step(state, restingWorld);
        backend->download(state);

        for (const auto& event : backend->impacts()) {
            require(event.energy_J == 0.f, "Resting contact continually paints heat");
        }
    }
}

/**
 * Run the full impact telemetry suite on one backend.
 *
 * @param cuda Whether to exercise the CUDA backend (false runs the CPU one).
 * @return Per-body peak impact energies observed during the severity ladder.
 */
std::array<float, SEVERITY_BODIES> run(bool cuda) {
    constexpr int BODY_COUNT = 8;

    auto scene = app::viewerScenario("impact", 42);

    sim::State tracked(BODY_COUNT);
    sim::State baseline(BODY_COUNT);
    app::populateViewer(tracked, scene);
    app::populateViewer(baseline, scene);

    auto tracker = cuda ? sim::createCudaBackend(BODY_COUNT) : sim::createCpuBackend(BODY_COUNT);
    auto silent = cuda ? sim::createCudaBackend(BODY_COUNT) : sim::createCpuBackend(BODY_COUNT);

    tracker->setImpactTracking(true);
    tracker->upload(tracked);
    silent->upload(baseline);

    render::ImpactHistory history(BODY_COUNT);

    const auto severity = runSeverityLadder(scene, tracked, baseline, *tracker, *silent, history);

    require(
        severity.anyImpact && severity.peaks[0] > 0.1f &&
        severity.peaks[1] > severity.peaks[0] * 10.f &&
        severity.peaks[2] > severity.peaks[1] * 4.f,
    "Impact energy does not increase quadratically with closing speed");

    verifyPatchFollowsBody(tracked, history);
    verifyHistoryRetentionAndFade(tracked, history);

    verifyWallImpactMark(cuda, scene.world);
    verifyRestingContactIsFiltered(cuda, scene.world);

    std::cout
        << (cuda ? "CUDA" : "CPU") << " energies J: "
        << severity.peaks[0] << ", " << severity.peaks[1] << ", " << severity.peaks[2] << '\n';

    return severity.peaks;
}

} // namespace (anonymous)

/**
 * Entry point: run the CPU suite, compare against CUDA when available, and
 * report failures through the exit status.
 *
 * @return Zero on success, one when any telemetry assertion fails.
 */
int main() {
    constexpr float BACKEND_TOLERANCE_J = 0.0001f;

    try {
        const auto cpu = run(false);
        if (sim::hasCudaDevice()) {
            const auto cuda = run(true);
            for (int i = 0; i < SEVERITY_BODIES; ++i) {
                const auto idx = static_cast<std::size_t>(i);
                require(std::abs(cpu[idx] - cuda[idx]) < BACKEND_TOLERANCE_J, "CPU/CUDA impact severity differs");
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
