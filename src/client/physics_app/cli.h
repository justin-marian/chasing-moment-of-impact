/**
 * @file
 * Parse and validate glass-viewer and shared-world launcher options.
 *
 * Command-line options for the physics viewer and their parser.
 * Validates every range and enumerates the accepted values before returning,
 * so the caller can trust the options it receives without any further checks.
 * Any unknown flag or out-of-range value throws std::runtime_error with a
 * message that names the offending option.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "shared/sim/state.h"


namespace app {

/**
 * Every value the physics viewer needs to start.
 *
 * Defaults match the values a bare invocation uses; the parser overrides them
 * from the command line.
 */
struct ViewerOptions {
    /**
     * Backend, deterministic seed and initial playback settings.
     */
    struct Simulation {
        std::string backend{"cpu"};   /* Physics backend. */
        std::string scenario{"demo"}; /* Initial simulation scenario. */
        std::uint64_t seed{42};       /* Deterministic simulation seed. */
        bool paused{};                /* Start with simulation paused. */
    } simulation;


    /**
     * Endpoint and login settings used when the viewer enters the shared-world scene.
     */
    struct Network {
        std::string host{"127.0.0.1"};   /* Server IPv4 address. */
        std::string user{"viewer"};      /* Login user name. */
        std::string token{"local-demo"}; /* Login token. */
        int port{};                      /* Server port; 0 disables connection. */
        bool lobby{};                    /* Open the lobby view. */
    } network;


    /**
     * World environment, supplied character rig and initial animation/appearance selection.
     */
    struct Scene {
        std::string mode{"world"};          /* Double-clicking the viewer opens the mannequin world. */
        std::string environment{"village"}; /* World environment. */
        std::string character{"male"};      /* Character model. */
        std::string previewClip;            /* Animation clip preview. */
        sim::Vec3 characterColor{sim::Real(.33), sim::Real(.43), sim::Real(.52)};
    } scene;


    /**
     * Window, surface response and sky presentation settings.
     *
     * These do not change authoritative physics.
     */
    struct View {
        int width{1440}, height{900}; /* Initial window dimensions. */
        int sky{4};                   /* Sky preset. */
        float skyRotation{};          /* Sky rotation. */
        float skyExposure{1};         /* Sky exposure. */
        bool visible{true};           /* Create a visible window. */
        bool presentation{};          /* Enable presentation mode. */
        bool physicsSettings{};       /* Open the physics parameter window at launch. */
    } view;


    /**
     * Heatmap retention and energy-to-color settings for visible collision feedback.
     */
    struct Impact {
        float life{8};           /* Impact visualization lifetime. */
        float redEnergy{10};     /* Energy mapped to the red threshold. */
        bool enabled{true};      /* Enable impact visualization. */
        bool hold{};             /* Preserve impact markers. */
        bool avatarImpactDemo{}; /* Enable the world impact demonstration. */
    } impact;


    /**
     * Optional finite-frame capture settings.
     *
     * A negative frame count keeps the interactive loop running.
     */
    struct Capture {
        int frames{-1}; /* Frame count; -1 disables automatic capture. */
        std::filesystem::path screenshot{"generated/images/captures/viewer.png"}; /* Screenshot output path. */
    } capture;
};


/**
 * Parse the command line into validated viewer options.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Fully parsed and validated viewer options.
 * @throws std::runtime_error If any argument is unknown or invalid.
 */
ViewerOptions parseViewerOptions(int argc, char** argv);

} // namespace app
