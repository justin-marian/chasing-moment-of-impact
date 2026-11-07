# Scenario and gameplay_model configuration. The source fingerprint is baked in as a
# define so a written report records exactly which sources produced it.
add_library(io_config 
    src/physics/scenarios/state_io.cpp 
    src/physics/scenarios/parse_config.cpp 
    src/physics/scenarios/run_log.cpp)

target_link_libraries(io_config PUBLIC sim_core nlohmann_json::nlohmann_json)
target_compile_definitions(io_config PRIVATE SIM_SOURCE_SHA256="${sourceFingerprint}")
strict(io_config)
