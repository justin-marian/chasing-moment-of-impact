# [COMMENTED FOR COLLISION FOCUS] Entire MMO Networking Module

# # Platform TCP and UDP transport; server cache commands live in server/services.
# add_library(network
# 	src/networking/tcp.cpp
# 	src/networking/transport.cpp
# 	src/networking/udp.cpp)
#
# target_link_libraries(network PUBLIC sim_core nlohmann_json::nlohmann_json)
#
# if(WIN32)
# 	target_link_libraries(network PRIVATE ws2_32)
# endif()
#
# strict(network)
#
# # Authoritative gameplay and the shared player codec. ASSET_ROOT lets the model
# # can locate its assets relative to the source tree during development.
# add_library(gameplay_model
# 	src/gameplay/environment.cpp
# 	src/gameplay/simulation.cpp
# 	src/gameplay/controller.cpp
# 	src/protocol/codec.cpp)
#
# target_link_libraries(gameplay_model PUBLIC sim_core project_files nlohmann_json::nlohmann_json)
# target_compile_definitions(gameplay_model PRIVATE ASSET_ROOT="${CMAKE_SOURCE_DIR}")
# strict(gameplay_model)
#
# # Client interpolation is separate from the server-authoritative model.
# add_library(client_replication src/client/world_app/snapshot_buffer.cpp)
# target_link_libraries(client_replication PUBLIC gameplay_model)
# strict(client_replication)
