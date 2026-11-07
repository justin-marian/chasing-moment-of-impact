/**
 * @file
 * World settings: one owner for each named policy and dimension.
 */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>

#include "shared/sim/math.h"


namespace settings {

struct World {
    struct Ui;
    struct Rendering;

    struct Scene {
        static constexpr sim::Real HALF_EXTENT_M = sim::Real(32);
        static constexpr int HEIGHT_M = 100;
    };


    // MMO specific: Avatar rig defaults.
    // struct PlayerDefaults {
    //     /// The default rig must be one of the server-supported male/female character IDs.
    //     static constexpr const char* CHARACTER = "male";
    //     /// Unit-range RGB tint; keeping it here makes the default appearance explicit.
    //     static constexpr sim::Vec3 COLOR{sim::Real(.33), sim::Real(.43), sim::Real(.52)};
    // };


    // MMO specific: Player movement, health, and respawn logic.
    // struct Gameplay {
    //     /**
    //          * Fixed-step timing and upright capsule dimensions.
    //          * DT is expressed in seconds, while RADIUS, HEIGHT and HALF are metres.
    //          * HALF describes the playable world's horizontal half-width.
    //          */
    //     static constexpr sim::Real DT = sim::Real(1.0 / 120.0);
    //     static constexpr sim::Real RADIUS = sim::Real(.32), HEIGHT = sim::Real(1.8);
    //
    //     /**
    //          * Authoritative locomotion parameters.
    //          * WALK and RUN are horizontal speeds in metres per second, GRAVITY is the
    //          * downward acceleration and JUMP_SPEED is the initial upward jump velocity.
    //          */
    //     static constexpr sim::Real WALK = sim::Real(2.4), RUN = sim::Real(5.5);
    //     static constexpr sim::Real GRAVITY = sim::Real(9.81), JUMP_SPEED = sim::Real(5.2);
    //
    //     /**
    //          * Player lifecycle and controller timing parameters.
    //          * These values define full health, death-to-respawn delay, stale-input
    //          * expiry and the duration of the landing animation state.
    //          */
    //     static constexpr sim::Real MAX_HEALTH = sim::Real(100), RESPAWN_DELAY = sim::Real(3);
    //     static constexpr sim::Real INPUT_TIMEOUT = sim::Real(.3), LAND_DURATION = sim::Real(.18);
    //
    //     /**
    //          * Capacity and contact-management limits.
    //          * MAX_PLAYERS bounds authoritative player storage, SPAWN_ATTEMPTS bounds the
    //          * deterministic placement search and HIT_COOLDOWN is measured in simulation ticks.
    //          */
    //     static constexpr std::size_t MAX_PLAYERS = 64;
    //     static constexpr int SPAWN_ATTEMPTS = 512;
    //     static constexpr std::uint64_t HIT_COOLDOWN = 90;
    // };


    // MMO specific: Player controller and spawn grid.
    // struct Controller {
    //     /**
    //          * Fixed controller thresholds and placement parameters.
    //          * Distances are metres, speeds are metres per second and impact age is seconds.
    //          */
    //     /// Stop retaining physical hit records after this age; the renderer controls visible fading.
    //     inline static constexpr sim::Real MAX_IMPACT_AGE = sim::Real(120);
    //     /// Deterministic spawn grid spacing and displacement away from the origin.
    //     inline static constexpr sim::Real SPAWN_SPACING = sim::Real(1.2), SPAWN_Z_OFFSET = sim::Real(4);
    //
    //     /// Support tolerances avoid oscillating between grounded and falling near a surface.
    //     inline static constexpr sim::Real GROUND_EPSILON = sim::Real(.01), SUPPORT_EPSILON = sim::Real(.002);
    //     inline static constexpr sim::Real FLOOR_EPSILON = sim::Real(1e-5);
    //
    //     /// Ignore tiny horizontal drift when selecting blocked/idle animation states.
    //     inline static constexpr sim::Real BLOCKED_SPEED_EPSILON = sim::Real(.05),
    //                                       IDLE_SPEED_EPSILON = sim::Real(.05);
    //     /// Limit controller travel per collision substep to a fraction of its capsule radius.
    //     inline static constexpr sim::Real SUBSTEP_RADIUS_FRACTION = sim::Real(.25);
    //
    //     /// Number of deterministic spawn columns and horizontal grid offset.
    //     inline static constexpr int SPAWN_COLUMNS = 16, SPAWN_X_OFFSET = 8;
    // };


    // MMO specific: Multiplayer networking, datagrams, and replication.
    // struct Network {
    //     struct CommandHash {
    //         /// FNV-1a fixes case-sensitive CLI/protocol command identities; this is not solver tuning.
    //         static constexpr std::uint64_t OFFSET_BASIS = 14695981039346656037ULL;
    //         static constexpr std::uint64_t PRIME = 1099511628211ULL;
    //     };
    //
    //     struct Datagrams {
    //         inline static constexpr char HEX[] = "0123456789abcdef";
    //         inline static constexpr std::size_t HEADER = 48, CHUNK = 1100, TICKET_SIZE = 32;
    //         inline static constexpr std::size_t MAX_CHUNKS = 256, MAX_ASSEMBLIES = 8;
    //         /// Limit receive work per poll and reliable-send storage/retries per connection.
    //         inline static constexpr int MAX_DATAGRAMS = 512, MAX_PENDING = 32, MAX_ATTEMPTS = 20;
    //         /// Retry spacing in seconds; social traffic is acknowledged while movement can be superseded.
    //         inline static constexpr double RETRY_SECONDS = .25;
    //     };
    //
    //     struct Transport {
    //         /// Platform-neutral invalid native socket handle.
    //         static constexpr std::uintptr_t BAD_SOCKET = std::numeric_limits<std::uintptr_t>::max();
    //         /// Largest valid TCP or UDP port number.
    //         static constexpr int MAX_PORT = 65535;
    //         /// Base transport buffer size.
    //         static constexpr std::size_t BUFFER_SIZE = 4096;
    //         /// Maximum buffered outgoing TCP data before a slow consumer is rejected.
    //         static constexpr std::size_t MAX_QUEUE_BYTES = 1024 * 1024;
    //     };
    //
    //     struct Server {
    //         struct Identity {
    //             /// Reject a numerically zero quaternion before normalization; text accepts printable ASCII only.
    //             static constexpr sim::Real MIN_QUATERNION_NORM2 = sim::Real(1e-12);
    //             static constexpr unsigned char FIRST_PRINTABLE = 32, LAST_PRINTABLE = 126;
    //             static constexpr std::uint32_t MAXIMUM = 0x7FFFFFFF;
    //             /// Bound collision retries instead of allowing a broken entropy source to stall the server.
    //             static constexpr int ATTEMPTS = 128;
    //         };
    //
    //         struct Policy {
    //             /// At 120 Hz: three seconds without UDP traffic, then 250 ms between reliable-event retries.
    //             static constexpr std::int64_t UDP_SILENCE_TICKS = 360, RETRANSMIT_DELAY_TICKS = 30;
    //             static constexpr int MAX_DELIVERY_ATTEMPTS = 20;
    //
    //             /// Bound unacknowledged UDP events and accepted login/cache metadata.
    //             static constexpr std::size_t MAX_PENDING_DELIVERIES = 128, USERNAME_BYTES = 32,
    //                                          CACHE_PREFIX_BYTES = 80;
    //             /// Cache TTL is bounded to one week; the extra minute protects a nearly expired resume snapshot.
    //             static constexpr int MIN_SESSION_SECONDS = 1, MAX_SESSION_SECONDS = 60 * 60 * 24 * 7;
    //             static constexpr int PERSIST_GRACE_S = 60;
    //             /// Remember recent RPC replies so a retried social request is applied only once.
    //             static constexpr std::size_t RPC_REPLY_LIMIT = 128;
    //             /// Keep public/private history bounded in memory and in the persistence cache.
    //             static constexpr std::size_t CHAT_HISTORY_LIMIT = 100;
    //             /// Byte limits also size the corresponding client edit buffers, including a terminator there.
    //             static constexpr std::size_t LOBBY_NAME_LIMIT = 40, CHAT_TEXT_LIMIT = 256;
    //             /// Bound total rooms, memberships per player and participants per private room.
    //             static constexpr std::size_t ROOM_LIMIT = 128, PLAYER_ROOM_LIMIT = 8,
    //                                          ROOM_PARTICIPANT_LIMIT = 16;
    //             /// Reject stale RPC sequence numbers; rate-limit chat in authoritative simulation ticks.
    //             static constexpr std::uint64_t RPC_STALE_WINDOW = 128, CHAT_RATE_TICKS = 12;
    //             /// Default resumable-session TTL is one day; startup options may override it.
    //             static constexpr int DEFAULT_SESSION_SECONDS = 86400;
    //             /// Derive the advertised simulation rate from the shared fixed step to avoid divergent settings.
    //             static constexpr int TICK_RATE_HZ =
    //                 int(sim::Real(1) / settings::World::Gameplay::DT + sim::Real(.5));
    //             /// Replicate poses less often than simulation ticks; clients interpolate the intervening frames.
    //             static constexpr int SNAPSHOT_RATE_HZ = 20;
    //             /// Public world metadata derives from the same controller bounds, capacity and health values.
    //             /// Snapshot cadence is derived from the same simulation and replication rates advertised to clients.
    //             static constexpr std::uint64_t SNAPSHOT_EVERY_TICKS = TICK_RATE_HZ / SNAPSHOT_RATE_HZ;
    //             static constexpr std::uint64_t SAVE_EVERY_TICKS = 600;
    //             /// One truncated nanosecond period preserves the existing 120 Hz scheduling interval.
    //             inline static constexpr std::chrono::nanoseconds TICK_PERIOD{1'000'000'000 / TICK_RATE_HZ};
    //         };
    //     };
    //
    //     struct Replication {
    //         /// Keep interpolation memory bounded even if a client renders slowly.
    //         static constexpr std::size_t MAX_HISTORY = 32;
    //         /// Render 100 ms behind arrival time so adjacent server snapshots can be interpolated.
    //         static constexpr double RENDER_DELAY_S = .1;
    //     };
    // };


    // MMO specific: 3D avatar preview settings.
    // struct Preview {
    //     struct Ids {
    //         static constexpr std::uint32_t LOCAL = 0x5D3A91C7, FEMALE = 0x2B7E406D, MALE = 0x6F128AB3;
    //         static constexpr std::array<std::uint32_t, 3> ALL{LOCAL, FEMALE, MALE};
    //     };
    //
    //     struct Camera {
    //         static constexpr float IMPACT_DISTANCE = 7.5f;
    //     };
    // };


    struct Timing {
        // MMO specific: Client movement send cadence.
        // /// Client movement send cadence; jump input bypasses this wait.
        // static constexpr double INPUT_PERIOD_S = 1.0 / 30.0;

        /// Maximum live frame duration in seconds; bounds work after a window stall.
        static constexpr double MAXIMUM_ELAPSED_S = .1;
        /// Deterministic capture cadence: 60 visual frames per simulated second.
        static constexpr double CAPTURE_STEP_S = 1.0 / 60.0;
    };


    struct Launcher {
        struct Limits {
            inline static constexpr int MIN_WIDTH = 640, MAX_WIDTH = 3840;
            inline static constexpr int MIN_HEIGHT = 480, MAX_HEIGHT = 2160;
            inline static constexpr float MIN_IMPACT_LIFE_S = .1f, MAX_IMPACT_LIFE_S = 120.f;
            inline static constexpr float MIN_IMPACT_RED_ENERGY_J = .01f, MAX_IMPACT_RED_ENERGY_J = 10000.f;
            // Zero selects offline mode; nonzero values must fit a network port.
            inline static constexpr int MIN_PORT = 0, MAX_PORT = 65535;
        };
    };


    struct PhysicsApp {
        struct Policy {
            /// Retained UI event count; older messages are discarded without affecting simulation state.
            static constexpr std::size_t LOG_ENTRIES = 200;
            /// Convert byte RGB command-line colors into the renderer's unit-range color components.
            static constexpr float RGB_SCALE = 1.f / 255.f;
            /// Bound live-frame catch-up after a rendering stall; the solver still uses its fixed dt.
            /// Four seconds at a nominal 60 FPS, keeping the timing plot's storage bounded.
            static constexpr std::size_t TIMING_HISTORY = 240;
        };
    };


    // MMO specific: 3D assets, glTF parsers, and model paths.
    // struct Assets {
    //     /// Protocol-visible order of the supplied environment choices.
    //     inline static constexpr std::array<const char*, 4> ENVIRONMENTS{
    //         "village", "forest", "highlands", "town"
    //     };
    //
    //     struct Skeleton {
    //         static constexpr int MAX_BONES = 128;
    //     };
    //
    //     struct GltfReader {
    //         /// Reject excessive accessor allocations before trusting an asset-declared element count.
    //         inline static constexpr std::size_t MAX_ACCESSOR_COUNT = 16u * 1024u * 1024u;
    //         /// Little-endian glTF 2.0 chunk type codes, spelling JSON and BIN respectively.
    //         inline static constexpr std::uint32_t JSON_CHUNK = 0x4e4f534a;
    //         inline static constexpr std::uint32_t BIN_CHUNK = 0x004e4942;
    //         inline static constexpr std::uint32_t GLTF_V2_0 = 0x46546c67;
    //     };
    //
    //     struct MeshValidation {
    //         /// Reject a skin-weight sum too small to normalize reliably.
    //         static constexpr float MINIMUM_WEIGHT = 1e-6f;
    //         /// Initial min/max reduction sentinel; actual mesh bounds are reduced from vertex coordinates.
    //         static constexpr float BOUNDS_LIMIT = 1e20f;
    //     };
    //
    //     struct Paths {
    //         static inline constexpr std::string_view MALE_MODEL = "assets/models/avatars/UAL1_Standard.glb";
    //         static inline constexpr std::string_view FEMALE_MODEL = "assets/models/avatars/Mannequin_F.glb";
    //         static inline constexpr std::string_view ANIMATION_LIBRARY = "assets/models/avatars/UAL2_Standard.glb";
    //         static inline constexpr std::string_view SKY_CATALOG = "generated/runtime-assets/skies/catalog.json";
    //     };
    // };
};

} // namespace settings
