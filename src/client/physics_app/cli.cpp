/**
 * @file
 * Parse and validate glass-viewer and shared-world launcher options.
 */

#include "settings/world/constants.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "client/physics_app/cli.h"
#include "shared/utils/command.h"


namespace app {

namespace {

/**
 * Keep strict number parsing local to this command-line implementation.
 */
namespace detail {

/**
 * Parse an integer strictly, checking for trailing characters and range limits.
 *
 * @tparam T Target integral type.
 * @param option Option name used for error messages.
 * @param text String to parse.
 * @param base Numeric base for conversion.
 * @return Parsed integer value.
 * @throws std::runtime_error If parsing fails, characters trail, or the value is out of range.
 */
template <typename T>
T parseInteger(std::string_view option, std::string_view text, int base = 10) {
    static_assert(std::is_integral_v<T>);

    if (text.empty()) {
        throw std::runtime_error("Missing numeric value for " + std::string(option));
    }

    try {
        std::size_t consumed = 0;

        if constexpr (std::is_same_v<T, unsigned long long>) {
            const auto value = std::stoull(std::string(text), &consumed, base);
            
            if (consumed != text.size()) {
                throw std::invalid_argument("trailing characters");
            }
            
            return value;
        } else {
            const auto value = std::stoll(std::string(text), &consumed, base);
            
            if (consumed != text.size()) {
                throw std::invalid_argument("trailing characters");
            }
                
            if (value < static_cast<long long>(std::numeric_limits<T>::min()) ||
                value > static_cast<long long>(std::numeric_limits<T>::max())) {
                throw std::out_of_range("integer outside target range");
            }

            return static_cast<T>(value);
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid integer for " + std::string(option) + ": " + std::string(text));
    }
}


/**
 * Parse a finite floating-point value strictly.
 *
 * @param option Option name used for error messages.
 * @param text String to parse.
 * @return Parsed floating-point value.
 * @throws std::runtime_error If parsing fails or the value is not finite.
 */
float parseFloat(std::string_view option, std::string_view text) {
    try {
        std::size_t consumed = 0;
        const float value = std::stof(std::string(text), &consumed);

        if (consumed != text.size() || !std::isfinite(value)) {
            throw std::invalid_argument("invalid float");
        }
        
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid number for " + std::string(option) + ": " + std::string(text));
    }
}


/**
 * Check if a value matches any of the provided choices.
 *
 * @tparam T Value type.
 * @tparam U Variadic choice types.
 * @param value Value to check.
 * @param choices Acceptable values.
 * @return True if a match is found, false otherwise.
 */
template <class T, class... U>
constexpr bool oneOf(const T& value, const U&... choices) {
    return ((value == choices) || ...);
}

} // namespace detail


/**
 * Parsed command-line argument, supporting inline values.
 */
struct Argument {
    std::string_view name;
    std::string_view inlineValue;
    bool hasInlineValue = false;
    std::uint64_t key = 0;
};


/**
 * Split a raw command-line token into its name and optional inline value.
 *
 * @param raw The raw command-line token.
 * @return Parsed argument structure.
 */
Argument splitArgument(std::string_view raw) {
    Argument argument;

    const auto equalsSign = raw.find('=');
    argument.name = raw.substr(0, equalsSign);
    argument.key = utils::commandKey(argument.name);

    if (equalsSign != std::string_view::npos) {
        argument.inlineValue = raw.substr(equalsSign + 1);
        argument.hasInlineValue = true;
    }

    return argument;
}


/**
 * Fetch the value for an argument, either inline or from the next slot.
 *
 * Advances the argument index if the next slot is consumed.
 *
 * @param arg Argument requiring a value.
 * @param index Current argument index.
 * @param argc Total argument count.
 * @param argv Array of argument strings.
 * @return The resolved value string.
 * @throws std::runtime_error If the value is missing.
 */
std::string requireValue(const Argument& arg, int& index, int argc, char** argv) {
    if (arg.hasInlineValue) {
        if (arg.inlineValue.empty()) {
            throw std::runtime_error("Missing value for " + std::string(arg.name));
        }
        return std::string(arg.inlineValue);
    }

    if (++index >= argc) {
        throw std::runtime_error("Missing value for " + std::string(arg.name));
    }
    
    return argv[index];
}


#ifdef SIM_BUILD_MMO
/**
 * Parse a 6-digit hexadecimal color string into a vector.
 *
 * @param raw The input color string, optionally prefixed with '#'.
 * @return The parsed color vector.
 * @throws std::runtime_error If the string is not a valid 6-digit hex code.
 */
sim::Vec3 parseHexColor(std::string_view raw) {
    if (!raw.empty() && raw.front() == '#') {
        raw.remove_prefix(1);
    }

    const bool allHex = raw.size() == 6 && std::all_of(raw.begin(), raw.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
    
    if (!allHex) {
        throw std::runtime_error("Color must be 6 hexadecimal digits, for example #3B82F6");
    }

    const auto rgb = static_cast<unsigned long>(std::stoul(std::string(raw), nullptr, 16));

    const auto channel = [](unsigned long bits) {
        return static_cast<float>(bits & 0xff) * settings::World::PhysicsApp::Policy::RGB_SCALE;
    };
    
    return {channel(rgb >> 16), channel(rgb >> 8), channel(rgb)};
}
#endif


/**
 * Validate the parsed viewer options.
 *
 * Checks enum-like flags, window size limits, impact tuning, capture
 * extensions, and networking ports against the configured limits.
 *
 * @param o The launcher options to validate.
 * @throws std::runtime_error If any option is invalid.
 */
void validate(const ViewerOptions& o) {
    if (!detail::oneOf(o.simulation.backend, "cpu", "cuda")) {
        throw std::runtime_error("Backend must be cpu or cuda");
    }
    
#ifdef SIM_BUILD_MMO
    if (!detail::oneOf(o.scene.mode, "physics", "world")) {
        throw std::runtime_error("Scene must be physics or world");
    }
    
    if (!detail::oneOf(o.scene.character, "male", "female")) {
        throw std::runtime_error("Character must be male or female");
    }
#else
    if (o.scene.mode != "physics") {
        throw std::runtime_error("This viewer build supports only --scene physics");
    }
#endif

    if (o.view.width < settings::World::Launcher::Limits::MIN_WIDTH ||
        o.view.width > settings::World::Launcher::Limits::MAX_WIDTH ||
        o.view.height < settings::World::Launcher::Limits::MIN_HEIGHT ||
        o.view.height > settings::World::Launcher::Limits::MAX_HEIGHT) {
        throw std::runtime_error("Window size must be within 640x480..3840x2160");
    }

    if (!std::isfinite(o.impact.life) ||
        o.impact.life < settings::World::Launcher::Limits::MIN_IMPACT_LIFE_S ||
        o.impact.life > settings::World::Launcher::Limits::MAX_IMPACT_LIFE_S) {
        throw std::runtime_error("Impact lifetime must be in [0.1, 120] seconds");
    }
    
    if (!std::isfinite(o.impact.redEnergy) ||
        o.impact.redEnergy < settings::World::Launcher::Limits::MIN_IMPACT_RED_ENERGY_J ||
        o.impact.redEnergy > settings::World::Launcher::Limits::MAX_IMPACT_RED_ENERGY_J) {
        throw std::runtime_error("Impact red energy must be in [0.01, 10000] J");
    }

    if (o.capture.screenshot.extension() != ".png") {
        throw std::runtime_error("Capture path must end in .png");
    }

    if (o.capture.frames < -1) {
        throw std::runtime_error("Frame count cannot be less than -1");
    }
    
#ifdef SIM_BUILD_MMO
    if (o.network.port < settings::World::Launcher::Limits::MIN_PORT ||
        o.network.port > settings::World::Launcher::Limits::MAX_PORT) {
        throw std::runtime_error("Port must be in [0, 65535]");
    }
#endif
}


/**
 * Print the command-line usage help and exit successfully.
 */
[[noreturn]] void showHelpAndExit() {
#ifdef SIM_BUILD_MMO
    std::cout 
        << "physics_viewer --scene physics|world --backend cpu|cuda --scenario A|B|C|impact\n"
        << "  --environment village|forest|highlands|town --connect PORT --user NAME\n"
#else
    std::cout   << "physics_viewer --scene physics --backend cpu|cuda --scenario demo|A|B|C|impact\n"
#endif
                << "  --physics-settings --hidden --paused --frames N --capture FILE --width W --height H\n";
    std::exit(EXIT_SUCCESS);
}


/**
 * Attempt to apply a boolean flag argument.
 *
 * @param o The launcher options to modify.
 * @param arg The argument to process.
 * @return True if the flag was recognized and applied.
 */
bool applyFlag(ViewerOptions& o, const Argument& arg) {
    switch (arg.key) {
        case utils::commandKey("--help"):
        case utils::commandKey("-h"): showHelpAndExit();
        case utils::commandKey("--hidden"):  o.view.visible = false; return true;
        case utils::commandKey("--presentation"): o.view.presentation = true; return true;
        case utils::commandKey("--physics-settings"): o.view.physicsSettings = true; return true;
        case utils::commandKey("--paused"): o.simulation.paused = true; return true;
        case utils::commandKey("--no-impact"): o.impact.enabled = false; return true;
        case utils::commandKey("--impact-hold"): o.impact.hold = true; return true;
#ifdef SIM_BUILD_MMO
        case utils::commandKey("--world-impact-demo"): o.impact.avatarImpactDemo = true; return true;
        case utils::commandKey("--lobby"): o.network.lobby = true; return true;
#endif
        default: return false;
    }
}


/**
 * Apply a value-bearing argument.
 *
 * @param o The launcher options to modify.
 * @param arg The argument to process.
 * @param i Current argument index.
 * @param argc Total argument count.
 * @param argv Array of argument strings.
 * @throws std::runtime_error If the argument is unknown.
 */
void applyValue(ViewerOptions& o, const Argument& arg, int& i, int argc, char** argv) {
    auto next = [&] { return requireValue(arg, i, argc, argv); };

    switch (arg.key) {
        case utils::commandKey("--backend"): o.simulation.backend = next(); break;
        case utils::commandKey("--scene"): o.scene.mode = next(); break;
        case utils::commandKey("--scenario"): o.simulation.scenario = next(); break;
        case utils::commandKey("--capture"): o.capture.screenshot = next(); break;
        case utils::commandKey("--frames"): o.capture.frames = detail::parseInteger<int>(arg.name, next()); break;
        case utils::commandKey("--width"): o.view.width = detail::parseInteger<int>(arg.name, next());  break;
        case utils::commandKey("--height"): o.view.height = detail::parseInteger<int>(arg.name, next()); break;
        case utils::commandKey("--seed"): o.simulation.seed = detail::parseInteger<unsigned long long>(arg.name, next(), 0); break;
        case utils::commandKey("--impact-life"): o.impact.life = detail::parseFloat(arg.name, next()); break;
        case utils::commandKey("--impact-red-energy"): o.impact.redEnergy = detail::parseFloat(arg.name, next()); break;
#ifdef SIM_BUILD_MMO
        case utils::commandKey("--environment"): o.scene.environment = next(); break;
        case utils::commandKey("--clip"): o.scene.previewClip = next(); break;
        case utils::commandKey("--character"): o.scene.character = next(); break;
        case utils::commandKey("--host"): o.network.host = next(); break;
        case utils::commandKey("--user"): o.network.user = next(); break;
        case utils::commandKey("--token"): o.network.token = next(); break;
        case utils::commandKey("--sky"): o.view.sky = detail::parseInteger<int>(arg.name, next()); break;
        case utils::commandKey("--connect"): o.network.port = detail::parseInteger<int>(arg.name, next()); break; 
        case utils::commandKey("--sky-rotation"): o.view.skyRotation = detail::parseFloat(arg.name, next()); break;
        case utils::commandKey("--sky-exposure"): o.view.skyExposure = detail::parseFloat(arg.name, next()); break;   
        case utils::commandKey("--color"): o.scene.characterColor = parseHexColor(next()); break;
#endif
        default: throw std::runtime_error("Unknown viewer option: " + std::string(arg.name));
    }
}

} // namespace (anonymous)


/**
 * Parse and validate all command-line arguments.
 *
 * @param argc Total argument count.
 * @param argv Array of argument strings.
 * @return Fully populated and validated viewer options.
 */
ViewerOptions parseViewerOptions(int argc, char** argv) {
    ViewerOptions options;

#ifdef SIM_BUILD_MMO
    bool explicitScene = false;
    bool physicsScenario = false;
#else
    options.scene.mode = "physics";
#endif

    for (int i = 1; i < argc; ++i) {
        const auto argument = splitArgument(argv[i]);

#ifdef SIM_BUILD_MMO
        switch (argument.key) {
            case utils::commandKey("--scene"): explicitScene = true; break;
            case utils::commandKey("--scenario"): physicsScenario = true; break;
            default: break;
        }
#endif
        if (!applyFlag(options, argument)) {
            applyValue(options, argument, i, argc, argv);
        }
    }

#ifdef SIM_BUILD_MMO
    // Existing benchmark commands still select physics, while a bare
    // executable opens the world. Only --scene (or --scenario, implied) wins.
    if (!explicitScene && (physicsScenario || options.view.physicsSettings)) {
        options.scene.mode = "physics";
    }
#endif

    validate(options);
    return options;
}

} // namespace app
