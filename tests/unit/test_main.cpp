/**
 * @file
 * Physics subsystem acceptance test suite.
 *
 * Runs deterministic checks across bounding volumes, collision pair types,
 * forces, spawning, matrix scenarios, wall containment, CCD, sleep/wake,
 * allocation-free stepping, UI integration, long-run energy/momentum
 * conservation, stack stability, and CPU/CUDA parity. Each test throws on
 * the first failed assertion; the runner reports per-test pass/fail and
 * exits non-zero when any test fails.
 */

#include "settings/collisions/constants.h"
#include "physics/scenarios/scenario_io.h"
#include "shared/sim/physics.h"

#include <atomic>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>

/**
 * Counts global operator new calls so hot-stepping tests can assert that
 * per-tick physics work stays allocation-free.
 */
std::atomic<std::size_t> allocationCount{0};

/**
 * Tracked replacement for global operator new. Delegates to malloc so the
 * matching operator delete can free without recording per-allocation sizes.
 *
 * @param n Bytes requested by the caller.
 * @return Allocated block of at least n bytes, never null on success.
 * @throws std::bad_alloc When the underlying malloc returns null.
 */
void* operator new(std::size_t n) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }

/**
 * Matching deletions for the tracked operator new overloads. Every overload
 * releases the block with free because the matching allocation came from
 * malloc; the sized variants ignore the size argument, which exists only so
 * the compiler can select a sized-delete overload when one is available.
 *
 * @param p Block previously returned by the tracked operator new or new[].
 * @param size Bytes originally requested; present for the sized overloads
 *             and ignored by their bodies.
 */
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {

using namespace sim;

/**
 * Shared geometric epsilon used across bounding, contact and force checks.
 */
const Real EPSILON = settings::Collisions::Geometry::EPSILON;

/**
 * Y position of a unit sphere resting on the floor of the default box.
 */
constexpr Real FLOOR_Y = Real(-14.5);

/**
 * Abort the current test when the condition is false.
 *
 * @param condition Checked precondition; a false value throws.
 * @param message   Diagnostic message attached to the thrown exception.
 */
void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

/**
 * Abort the current test when two values differ by more than tolerance,
 * appending both observed values to the caller's message.
 *
 * @param actual    Observed value.
 * @param expected  Expected reference value.
 * @param tolerance Maximum permitted absolute difference.
 * @param message   Diagnostic prefix attached to the thrown exception.
 */
void near(Real actual, Real expected, Real tolerance, const std::string& message) {
    require(absolute(actual - expected) <= tolerance, message + " actual=" + std::to_string(actual) + " expected=" + std::to_string(expected));
}

/**
 * Build a world configuration that isolates the tested interaction: no
 * gravity, no friction, no damping, perfect restitution, sleeping disabled.
 *
 * @return Minimally interacting world configuration for analytic tests.
 */
WorldConfig conservative() {
    WorldConfig c;
    c.gravity_mps2 = 0;
    c.frictionStatic_unitless = c.frictionDynamic_unitless = 0;
    c.restitution_unitless = 1;
    c.linearDamping_per_s = c.angularDamping_per_s = 0;
    c.enableSleep = false;
    return c;
}

/**
 * Spawn helper: a unit-radius sphere at the given position and velocity.
 *
 * @param p Initial position.
 * @param v Initial linear velocity; zero when omitted.
 * @return Body configured as a unit sphere at p with velocity v.
 */
Body sphere(Vec3 p, Vec3 v = {}) {
    Body b;
    b.position_m = p;
    b.velocity_mps = v;
    b.halfExtents_m = {Real(.5), Real(.5), Real(.5)};
    return b;
}

/**
 * Verify that the refreshed AABB encloses every rotated OBB vertex, that the
 * OBB is tighter than the AABB, that the bounding sphere radius reaches the
 * corner, and that an identity orientation refreshes the AABB.
 */
void test_bv__aabb_obb_bounding_sphere() {
    constexpr Real ROTATION_RAD = settings::Collisions::Geometry::PI / 8;

    State s(4);
    Body b;
    b.shape = Shape::Cube;
    b.halfExtents_m = {2, 1, Real(.5)};
    b.orientation = {0, Real(sin(ROTATION_RAD)), 0, Real(cos(ROTATION_RAD))};
    require(s.spawn(b), "spawn");

    computeAabb(s.view(), 0);
    Vec3 extent = s.aabbExtent_m[0];
    for (int x : {-1, 1})
        for (int y : {-1, 1})
            for (int z : {-1, 1}) {
                Vec3 p = rotate(b.orientation, {Real(x) * 2, Real(y), Real(z) * Real(.5)});
                require(
                    absolute(p.x) <= extent.x + EPSILON && 
                    absolute(p.y) <= extent.y + EPSILON && 
                    absolute(p.z) <= extent.z + EPSILON, 
                "AABB excludes vertex");
            }

    require(extent.x * extent.y * extent.z > 2, "OBB should be tighter");
    require(s.boundingRadius_m[0] >= length(b.halfExtents_m) - EPSILON, "bounding sphere radius");

    s.orientation[0] = {};
    computeAabb(s.view(), 0);
    near(s.aabbExtent_m[0].x, 2, EPSILON, "AABB refresh");
}

/**
 * Verify ray/triangle distance and hit/miss classification, and triangle SAT
 * for non-coplanar, contained coplanar, separated coplanar, and degenerate triangle pairs.
 */
void test_collision__ray_triangle_and_triangle_sat() {
    Triangle a{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    Real distance = 0;

    require(rayTriangle({Real(.2), Real(.2), 1}, {0, 0, -1}, a, distance), "ray hit");
    near(distance, 1, EPSILON, "ray distance");
    require(!rayTriangle({2, 2, 1}, {0, 0, -1}, a, distance), "ray miss");

    require(triangleTriangle(a, {{Real(.2), Real(.2), -1}, {Real(.2), Real(.2), 1}, {Real(.8), Real(.2), 0}}), "noncoplanar triangles");
    require(triangleTriangle(a, {{Real(.1), Real(.1), 0}, {Real(.3), Real(.1), 0}, {Real(.1), Real(.3), 0}}), "contained coplanar triangle");

    require(!triangleTriangle(a, {{2, 2, 0}, {3, 2, 0}, {2, 3, 0}}), "separated coplanar triangles");
    require(!triangleTriangle(a, {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}), "degenerate triangle");
}


/**
 * Verify all ten ordered shape pairings produce symmetric contacts and that
 * randomized contacts stay finite and shallow.
 *
 * Each pair is queried forward and reversed; the normal is expected to flip
 * and the depth to stay identical. The random sweep spawns many mixed-shape
 * pairs and requires every reported contact to have finite normal and point
 * and depth within the contact slop.
 */
void test_collision__all_ten_pair_types_and_symmetry() {
    constexpr int RANDOM_RUNS = 1000;
    const Real SYMMETRY_TOLERANCE = Real(1e-5);

    State s(2);
    WorldConfig c;
    for (int a = 0; a < 4; ++a)
        for (int b = a; b < 4; ++b) {
            s.clear();
            Body x = sphere({0, 0, 0}), y = sphere({Real(.7), 0, 0});
            x.shape = static_cast<Shape>(a);
            y.shape = static_cast<Shape>(b);
            s.spawn(x);
            s.spawn(y);

            Contact first{}, second{};
            require(pairContact(s.view(), 0, 1, c.dt_s, c, first), "pair type missing");
            require(pairContact(s.view(), 1, 0, c.dt_s, c, second), "reverse pair missing");
            near(dot(first.normal, second.normal), -1, SYMMETRY_TOLERANCE, "normal symmetry");
            near(first.depth_m, second.depth_m, SYMMETRY_TOLERANCE, "depth symmetry");
        }

    std::mt19937_64 rng(99);
    for (int run = 0; run < RANDOM_RUNS; ++run) {
        s.clear();
        Body a = sphere({Real(int(rng() % 100) - 50) / 20, 0, 0});
        Body b = sphere({Real(int(rng() % 100) - 50) / 20, Real(.2), 0});

        a.shape = static_cast<Shape>(rng() % 4);
        b.shape = static_cast<Shape>(rng() % 4);
        s.spawn(a);
        s.spawn(b);

        Contact ct{};
        if (pairContact(s.view(), 0, 1, c.dt_s, c, ct))
            require(finite(ct.normal) && finite(ct.point_m) && ct.depth_m >= -c.contactSlop_m, "random contact invalid");
    }
}


/**
 * Verify gravity integrates to the analytic semi-implicit position, that
 * friction slows a sliding box, and that the Coulomb impulse stays within
 * the static limit before saturating at the dynamic one.
 */
void test_forces__gravity_and_coulomb_friction() {
    constexpr int FRICTION_FRAMES = 120;
    const Real GRAVITY_TOLERANCE = Real(1e-6);
    const Real COULOMB_TOLERANCE = Real(1e-5);

    State s(2);
    s.spawn(sphere({0, 0, 0}));
    WorldConfig c;
    c.linearDamping_per_s = 0;
    auto backend = createCpuBackend(2);

    backend->step(s, c);
    near(s.velocity_mps[0].y, -c.gravity_mps2 * c.dt_s, GRAVITY_TOLERANCE, "gravity");
    near(s.position_m[0].y, -c.gravity_mps2 * c.dt_s * c.dt_s, GRAVITY_TOLERANCE, "semi implicit position");

    s.clear();
    Body b = sphere({0, FLOOR_Y, 0}, {2, 0, 0});
    b.shape = Shape::Cube;
    s.spawn(b);
    Real before = s.velocity_mps[0].x;
    for (int i = 0; i < FRICTION_FRAMES; ++i)
        backend->step(s, c);
    require(s.velocity_mps[0].x < before, "friction should slow box");

    s.clear();
    s.spawn(sphere({0, 0, 0}, {4, 1, 0}));
    Contact ct;
    ct.normal = {0, 1, 0};
    ct.point_m = {0, Real(.5), 0};
    ct.targetVelocity_mps = 0;

    solveContactConstraint(s.view(), ct, c);
    require(length(ct.tangentImpulse_Ns) <= c.frictionStatic_unitless * ct.normalImpulse_Ns + EPSILON, "Coulomb limit");
    near(length(ct.tangentImpulse_Ns), c.frictionDynamic_unitless * ct.normalImpulse_Ns, COULOMB_TOLERANCE, "dynamic coefficient");
}


/**
 * Verify spawning across all four shapes is deterministic, stays inside the
 * box at the requested initial speed, and produces an unbiased initial
 * position and direction distribution across many spawns.
 */
void test_spawn__all_shapes_uniform_deterministic_and_inside() {
    constexpr int SPAWN_COUNT = 100;
    const Real MAX_INITIAL_SPEED = Real(5);
    const Real DIRECTION_BIAS_TOLERANCE = Real(.15);
    const Real POSITION_BIAS_TOLERANCE = Real(2);

    Scenario scenario;
    scenario.counts = {SPAWN_COUNT, SPAWN_COUNT, SPAWN_COUNT, SPAWN_COUNT};
    State a, b;
    populate(a, scenario);
    populate(b, scenario);
    require(hashState(a) == hashState(b), "same-seed state mismatch");

    Vec3 directions{}, positions{};
    for (int i = 0; i < a.count_i; ++i) {
        require(length(a.velocity_mps[i]) <= MAX_INITIAL_SPEED + EPSILON, "initial speed");
        directions += normalize(a.velocity_mps[i]);
        positions += a.position_m[i];
        computeAabb(a.view(), i);
    }

    require(staysInside(a, scenario.world, 0), "initial containment");
    require(length(directions) / Real(a.count_i) < DIRECTION_BIAS_TOLERANCE, "direction distribution biased");
    require(length(positions) / Real(a.count_i) < POSITION_BIAS_TOLERANCE, "position distribution biased");
}


/**
 * Verify that matrix scenarios A, B and C load with the same combined
 * cylinder and cone counts the generator produces, and that spawning from
 * the loaded scenario reproduces those counts.
 */
void test_matrix__a_b_c_counts_exact() {
    constexpr int LOWERCASE_OFFSET = 32;

    for (const char* id : {"A", "B", "C"}) {
        const auto basePath = std::filesystem::exists("configs") ? std::filesystem::current_path() : std::filesystem::path(PROJECT_DIR);
        const auto configPath = basePath / "configs" / (std::string("scenarios_matrix_") + static_cast<char>(id[0] + LOWERCASE_OFFSET) + ".yaml");

        auto config = loadScenario(configPath);
        auto expected = matrixScenario(id);
        require(config.counts == expected.counts, "matrix YAML counts");

        State s;
        populate(s, config);
        std::array<int, 4> counts{};
        for (int i = 0; i < s.count_i; ++i)
            ++counts[s.shape_i[i]];
        require(counts == expected.counts, "spawn counts");
    }
}


/**
 * Verify that all six walls register contacts for every shape and that the
 * body never escapes the box, independently of render visibility. Also
 * verifies that the near-wall alpha defaults stay within the expected range.
 */
void test_box__all_walls_collide_and_near_wall_visibility() {
    const Real WALL_PROBE_OFFSET = Real(.51);
    const Real WALL_PROBE_SPEED = Real(5);
    const Real WALL_ESCAPE_TOLERANCE = Real(.005);

    WorldConfig c = conservative();
    require(!c.nearWallVisible &&
                c.wallAlpha_unitless > 0 &&
                c.wallAlpha_unitless < 1,
            "wall defaults");

    State s(1);
    auto backend = createCpuBackend(1);
    for (int shape = 0; shape < 4; ++shape)
        for (int face = 0; face < 6; ++face) {
            s.clear();
            Body b = sphere({});
            b.shape = static_cast<Shape>(shape);

            int axis = face / 2;
            Real sign = face % 2 ? Real(1) : Real(-1);
            b.position_m[axis] = sign * (c.boxHalfExtents_m[axis] - WALL_PROBE_OFFSET);
            b.velocity_mps[axis] = sign * WALL_PROBE_SPEED;
            s.spawn(b);

            auto r = backend->step(s, c);
            require(r.diagnostics.contacts_i > 0, "wall contact not generated");
            require(staysInside(s, c, WALL_ESCAPE_TOLERANCE), "wall escape");
        }
}


/**
 * Verify sphere/sphere and sphere/wall CCD at high speed. OBB sweeps are not
 * covered by this test; that limitation is documented separately.
 */
void test_ccd__high_speed_spheres_and_walls() {
    const Real HIGH_SPEED_MPS = Real(200);
    const Real HIGH_SPEED_WALL_START = Real(19);
    const Real CCD_ESCAPE_TOLERANCE = Real(.001);

    WorldConfig c = conservative();
    State s(2);
    auto backend = createCpuBackend(2);
    s.spawn(sphere({-1, 0, 0}, {HIGH_SPEED_MPS, 0, 0}));
    s.spawn(sphere({1, 0, 0}, {-HIGH_SPEED_MPS, 0, 0}));

    auto r = backend->step(s, c);
    require(r.diagnostics.contacts_i > 0, "swept sphere contact");
    require(s.velocity_mps[0].x < 0 && s.velocity_mps[1].x > 0, "swept sphere rebound");

    s.clear();
    s.spawn(sphere({HIGH_SPEED_WALL_START, 0, 0}, {HIGH_SPEED_MPS, 0, 0}));
    backend->step(s, c);
    require(staysInside(s, c, CCD_ESCAPE_TOLERANCE), "high speed wall escape");
}


/**
 * Verify a resting body falls asleep and that a moving impact wakes it.
 */
void test_sleep__resting_and_wake_on_contact() {
    constexpr int SLEEP_FRAMES = 100;
    const Real WAKE_IMPACT_OFFSET = Real(.8);
    const Real WAKE_IMPACT_SPEED = Real(5);

    State s(2);
    auto c = WorldConfig{};
    c.restitution_unitless = 0;
    Body b = sphere({0, FLOOR_Y, 0});
    b.shape = Shape::Cube;
    s.spawn(b);

    auto backend = createCpuBackend(2);
    for (int i = 0; i < SLEEP_FRAMES; ++i)
        backend->step(s, c);
    require(s.isSleeping[0] != 0, "rest body not sleeping");

    Body moving = sphere({-WAKE_IMPACT_OFFSET, FLOOR_Y, 0}, {WAKE_IMPACT_SPEED, 0, 0});
    s.spawn(moving);
    backend->step(s, c);
    require(s.isSleeping[0] == 0, "impact should wake resting body");
}

/**
 * Verify that repeated stepping allocates no heap and that two independent
 * backends fed identical scenes produce identical state hashes.
 */
void test_step__no_heap_allocation_and_determinism() {
    constexpr int DETERMINISM_TICKS = 20;
    constexpr int DETERMINISM_BODIES = 64;

    Scenario scenario;
    scenario.counts = {10, 10, 5, 5};
    State a(DETERMINISM_BODIES), b(DETERMINISM_BODIES);
    populate(a, scenario);
    populate(b, scenario);

    auto first = createCpuBackend(DETERMINISM_BODIES);
    auto second = createCpuBackend(DETERMINISM_BODIES);
    first->step(a, scenario.world);
    second->step(b, scenario.world);

    auto before = allocationCount.load();
    for (int i = 0; i < DETERMINISM_TICKS; ++i) {
        first->step(a, scenario.world);
        second->step(b, scenario.world);
    }
    auto after = allocationCount.load();

    require(after == before, "CPU hot step allocated");
    require(hashState(a) == hashState(b), "CPU deterministic replay hash");
}


/**
 * Verify that UI actions (spawn, checkpoint, reset) reuse the same
 * step-boundary operations as the headless loop.
 */
void test_ui__spawn_reset_print_checkpoint() {
    constexpr int UI_COUNT_PER_SHAPE = 2;
    constexpr int UI_BODIES = 32;
    constexpr int UI_SPAWN_COUNT = 4;

    Scenario scene;
    State s(UI_BODIES);
    scene.counts = {
        UI_COUNT_PER_SHAPE, UI_COUNT_PER_SHAPE,
        UI_COUNT_PER_SHAPE, UI_COUNT_PER_SHAPE
    };
    populate(s, scene);

    auto backend = createCpuBackend(UI_BODIES);
    backend->step(s, scene.world);

    std::mt19937_64 rng(5);
    for (int i = 0; i < UI_SPAWN_COUNT; ++i)
        require(spawnRandom(s, static_cast<Shape>(i), scene, rng), "runtime spawn");

    std::filesystem::create_directories("test-artifacts");
    printMotion("test-artifacts/motion.csv", s);
    saveState("test-artifacts/state.json", s);

    State restored = loadState("test-artifacts/state.json");
    require(hashState(s) == hashState(restored), "checkpoint changes state");
    require(std::filesystem::file_size("test-artifacts/motion.csv") > 100, "motion CSV");

    s.clear();
    require(s.tick_i == 0 && s.count_i == 0, "reset state");
}

/**
 * Verify that a conservative equal-mass sphere pair conserves momentum and
 * kinetic energy over a long contact-heavy run.
 */
void test_energy_momentum__100000_ticks() {
    constexpr int ENERGY_TICKS = 100000;
    const Real MOMENTUM_TOLERANCE = Real(.01);
    const Real ENERGY_TOLERANCE = Real(.01);

    auto c = conservative();
    State s(2);
    s.spawn(sphere({-2, 0, 0}, {2, 0, 0}));
    s.spawn(sphere({2, 0, 0}, {-2, 0, 0}));

    auto backend = createCpuBackend(2);
    Real initial = emitDiagnostics(s.view(), c).kineticEnergy_J;

    Real worst = 0;
    for (int i = 0; i < ENERGY_TICKS; ++i) {
        auto r = backend->step(s, c);
        worst = maximum(worst, absolute(r.diagnostics.kineticEnergy_J - initial) / initial);
        require(length(r.diagnostics.linearMomentum_kgmps) < MOMENTUM_TOLERANCE, "momentum drift");
    }

    require(worst < ENERGY_TOLERANCE, "energy drift over 1%");
    std::cout << "  maximum relative energy drift=" << worst << '\n';
}


/**
 * Verify that an aligned four-box stack settles without 
 * drift, NaN, or escape over a million actual ticks.
 */
void test_stack__four_boxes_1000000_ticks() {
    constexpr int STACK_HEIGHT = 4;
    constexpr int STACK_TICKS = 1000000;
    constexpr int STACK_CHECK_INTERVAL = 1000;
    constexpr int STACK_SOLVER_ITERATIONS = 40;

    const Real STACK_SPACING = Real(1.001);
    const Real STACK_ESCAPE_TOLERANCE = Real(.01);
    const Real STACK_SETTLING_TOLERANCE = Real(.1);
    const Real STACK_REST_VELOCITY = Real(.05);

    State s(STACK_HEIGHT);
    WorldConfig c;
    c.restitution_unitless = 0;
    c.solverIterations_i = STACK_SOLVER_ITERATIONS;

    auto backend = createCpuBackend(STACK_HEIGHT);
    for (int i = 0; i < STACK_HEIGHT; ++i) {
        Body b = sphere({0, FLOOR_Y + Real(i) * STACK_SPACING, 0});
        b.shape = Shape::Cube;
        s.spawn(b);
    }

    const int top = STACK_HEIGHT - 1;
    Real initialY = s.position_m[top].y;

    for (int i = 0; i < STACK_TICKS; ++i) {
        auto r = backend->step(s, c);
        if (i % STACK_CHECK_INTERVAL == 0) {
            require(r.diagnostics.nanCount_i == 0, "stack NaN");
            require(staysInside(s, c, STACK_ESCAPE_TOLERANCE), "stack escape");
        }
    }

    std::filesystem::create_directories("test-artifacts");
    saveState("test-artifacts/stack-final.json", s);

    require(absolute(s.position_m[top].y - initialY) < STACK_SETTLING_TOLERANCE, "stack settling drift " + std::to_string(s.position_m[top].y - initialY));
    require(length(s.velocity_mps[top]) < STACK_REST_VELOCITY, "rest stack velocity");
    std::cout << "  top-box displacement_m=" << s.position_m[top].y - initialY << '\n';
}


/**
 * Verify CPU/CUDA parity across five seeds with gravity and contacts active,
 * requiring position and velocity deltas to stay within master13.4 tolerance.
 */
void test_cuda__parity_five_seeds() {
    constexpr int PARITY_SEEDS = 5;
    constexpr int PARITY_TICKS = 240;
    constexpr int PARITY_BODIES = 128;
    constexpr int SHAPE_COUNT = 25;
    const Real PARITY_DT = Real(1.0 / 240.0);
    const Real MAX_POSITION_ERROR = Real(.001);
    const Real MAX_VELOCITY_ERROR = Real(.01);

    require(hasCudaDevice(), "CUDA device required");
    Real maxPosition = 0, maxVelocity = 0;

    for (std::uint64_t seed = 1; seed <= PARITY_SEEDS; ++seed) {
        Scenario scene;
        scene.seed = seed;
        scene.counts = {SHAPE_COUNT, SHAPE_COUNT, SHAPE_COUNT, SHAPE_COUNT};
        scene.world.dt_s = PARITY_DT;

        State cpu(PARITY_BODIES), gpu(PARITY_BODIES);
        populate(cpu, scene);
        populate(gpu, scene);

        auto cb = createCpuBackend(PARITY_BODIES);
        auto gb = createCudaBackend(PARITY_BODIES);
        gb->upload(gpu);

        for (int i = 0; i < PARITY_TICKS; ++i) {
            cb->step(cpu, scene.world);
            gb->step(gpu, scene.world);
        }
        gb->download(gpu);

        for (int i = 0; i < cpu.count_i; ++i) {
            for (int axis = 0; axis < 3; ++axis) {
                maxPosition = maximum(maxPosition, absolute(cpu.position_m[i][axis] - gpu.position_m[i][axis]));
                maxVelocity = maximum(maxVelocity, absolute(cpu.velocity_mps[i][axis] - gpu.velocity_mps[i][axis]));
            }
        }
    }

    std::cout << "  parity maxPosition_m=" << maxPosition << " maxVelocity_mps=" << maxVelocity << '\n';
    require(maxPosition <= MAX_POSITION_ERROR && maxVelocity <= MAX_VELOCITY_ERROR, "CPU CUDA parity exceeded master tolerance");
}


/**
 * Verify that matrix scenarios run headless on the GPU, that results are
 * deterministic across two independent runs, and that bodies stay inside the box.
 */
void test_cuda__matrix_runs_and_determinism() {
    constexpr int MATRIX_TICKS = 120;

    for (const char* id : {"A", "B", "C"}) {
        Scenario scene = matrixScenario(id);
        State a, b;
        populate(a, scene);
        populate(b, scene);

        auto backend = createCudaBackend();
        backend->upload(a);
        for (int i = 0; i < MATRIX_TICKS; ++i)
            backend->step(a, scene.world);
        backend->download(a);

        backend->upload(b);
        for (int i = 0; i < MATRIX_TICKS; ++i)
            backend->step(b, scene.world);
        backend->download(b);

        require(hashState(a) == hashState(b), "CUDA deterministic hash");
        require(staysInside(a, scene.world), "matrix GPU escape");
    }
}

} // namespace (anonymous)

/**
 * Entry point. Selects the CUDA suite, the long-running suite, or the default
 * deterministic suite based on the first command-line argument, then runs
 * each test and reports per-test pass/fail.
 *
 * @param argc Argument count.
 * @param argv Argument vector; argv[1] selects --cuda, --long or none.
 * @return Zero when every selected test passes, one otherwise.
 */
int main(int argc, char** argv) {
    constexpr const char* CUDA_FLAG = "--cuda";
    constexpr const char* LONG_FLAG = "--long";

    bool cuda = argc > 1 && std::string(argv[1]) == CUDA_FLAG;
    bool longRun = argc > 1 && std::string(argv[1]) == LONG_FLAG;

    std::vector<std::pair<std::string, std::function<void()>>> tests;
    if (cuda)
        tests = {
            {"test_cuda__parity_five_seeds", test_cuda__parity_five_seeds},
            {"test_cuda__matrix_runs_and_determinism", test_cuda__matrix_runs_and_determinism}};
    else if (longRun)
        tests = {
            {"test_energy_momentum__100000_ticks", test_energy_momentum__100000_ticks},
            {"test_stack__four_boxes_1000000_ticks", test_stack__four_boxes_1000000_ticks}};
    else
        tests = {
            {"test_bv__aabb_obb_bounding_sphere", test_bv__aabb_obb_bounding_sphere},
            {"test_collision__ray_triangle_and_triangle_sat", test_collision__ray_triangle_and_triangle_sat},
            {"test_collision__all_ten_pair_types_and_symmetry", test_collision__all_ten_pair_types_and_symmetry},
            {"test_forces__gravity_and_coulomb_friction", test_forces__gravity_and_coulomb_friction},
            {"test_spawn__all_shapes_uniform_deterministic_and_inside", test_spawn__all_shapes_uniform_deterministic_and_inside},
            {"test_matrix__a_b_c_counts_exact", test_matrix__a_b_c_counts_exact},
            {"test_box__all_walls_collide_and_near_wall_visibility", test_box__all_walls_collide_and_near_wall_visibility},
            {"test_ccd__high_speed_spheres_and_walls", test_ccd__high_speed_spheres_and_walls},
            {"test_sleep__resting_and_wake_on_contact", test_sleep__resting_and_wake_on_contact},
            {"test_step__no_heap_allocation_and_determinism", test_step__no_heap_allocation_and_determinism},
            {"test_ui__spawn_reset_print_checkpoint", test_ui__spawn_reset_print_checkpoint}};

    int failed = 0;
    for (auto& [name, run] : tests) {
        try {
            run();
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception& e) {
            ++failed;
            std::cerr << "FAIL " << name << ": " << e.what() << '\n';
        }
    }

    std::cout << tests.size() - static_cast<std::size_t>(failed) << '/' << tests.size() << " tests passed\n";
    return failed ? 1 : 0;
}
