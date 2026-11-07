/**
 * @file
 * Construct and populate physics scenarios for the viewer application.
 */

#include "physics/scenarios/impact_demo.h"


namespace app {

/**
 * Create a simulation scenario based on an identifier and seed.
 *
 * If the identifier is "impact", configures a specialized zero-gravity
 * scenario with a smaller bounding box and no damping for precise impact
 * testing.
 *
 * @param id   Scenario identifier.
 * @param seed Deterministic seed for the scenario.
 * @return Configured simulation scenario.
 */
sim::Scenario viewerScenario(const std::string& id, std::uint64_t seed) {
    auto scene = sim::matrixScenario(id == "impact" ? "demo" : id, seed);
    
    if (id == "impact") {
        scene.id = id;
        scene.counts = {3, 3, 0, 0};
        scene.world.boxHalfExtents_m = {7, 4, 7};
        scene.world.gravity_mps2 = 0;
        scene.world.linearDamping_per_s = 0;
        scene.world.angularDamping_per_s = 0;
        scene.world.enableSleep = false;
    }
    
    return scene;
}


/**
 * Populate the simulation state based on the given scenario.
 *
 * For the "impact" scenario, spawns three cube targets and three projectile
 * spheres with equal geometry and mass, moving at 1, 4, and 9 m/s to test
 * quadratic impact energy scaling.
 *
 * @param state The simulation state to populate.
 * @param scene The scenario defining the bodies to spawn.
 */
void populateViewer(sim::State& state, const sim::Scenario& scene) {
    if (scene.id != "impact") {
        sim::populate(state, scene);
        return;
    }

    constexpr float SPEEDS[3] = {1.0f, 4.0f, 9.0f};
    constexpr float X_SPACING = 3.8f;
    constexpr float TARGET_EXTENT = 1.1f;
    constexpr float TARGET_MASS = 50.f;
    constexpr float PROJECTILE_EXTENT = .45f;
    constexpr float PROJECTILE_BASE_Z = .55f;
    constexpr float PROJECTILE_Z_FACTOR = .4f;

    state.clear();
    
    for (int i = 0; i < 3; ++i) {
        const float speed = SPEEDS[i];
        const float x = float(i - 1) * X_SPACING;
        
        sim::Body target;
        target.shape = sim::Shape::Cube;
        target.halfExtents_m = {TARGET_EXTENT, TARGET_EXTENT, TARGET_EXTENT};
        target.mass_kg = {TARGET_MASS};
        target.position_m = {x, 0, -1};
        state.spawn(target);
        
        sim::Body projectile;
        projectile.position_m = {
            x + PROJECTILE_EXTENT, 
            .2f, 
            PROJECTILE_BASE_Z + PROJECTILE_Z_FACTOR * speed
        };

        projectile.halfExtents_m = {PROJECTILE_EXTENT, PROJECTILE_EXTENT, PROJECTILE_EXTENT};
        projectile.velocity_mps = {0, 0, -speed};
        state.spawn(projectile);
    }
}

} // namespace app
