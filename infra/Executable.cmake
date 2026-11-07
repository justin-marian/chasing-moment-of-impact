# Simulation CLI.
add_executable(physics src/demos/main_physics.cpp)
target_link_libraries(physics PRIVATE backend_cpu backend_cuda io_config)
strict(physics)

# [COMMENTED FOR COLLISION FOCUS] MMO Server and Headless Client
# if(BUILD_MMO)
# 	# Authoritative server.
# 	add_executable(physics_server 
# 		src/demos/main_server.cpp
# 		src/server/services/cache.cpp
# 		src/server/services/snapshots.cpp
# 		src/server/services/chat_history.cpp
# 		src/server/validation.cpp
# 		src/server/lifecycle.cpp
# 		src/server/transports.cpp
# 		src/server/sessions.cpp
# 		src/server/dispatch.cpp
# 		src/server/handlers/players.cpp
# 		src/server/handlers/social.cpp)
# 	target_link_libraries(physics_server PRIVATE network gameplay_model)
# 	strict(physics_server)
#
# 	# Headless client.
# 	add_executable(physics_client src/demos/main_client.cpp src/client/console.cpp)
# 	target_link_libraries(physics_client PRIVATE network)
# 	strict(physics_client)
# endif()

if(BUILD_VIEWER)
	# Interactive viewer.
	add_executable(physics_viewer 
		src/demos/main_viewer.cpp
		src/client/physics_app/run_session.cpp
		src/client/physics_app/cli.cpp
		src/physics/scenarios/impact_demo.cpp
		src/client/physics_app/settings/diagnostic_windows.cpp
		src/client/physics_app/settings/parameter_windows.cpp)
		
	target_link_libraries(physics_viewer PRIVATE render_gl backend_cpu backend_cuda io_config)
	
	# [COMMENTED FOR COLLISION FOCUS] MMO Client World App Sources
	# if(BUILD_MMO)
	# 	target_sources(physics_viewer PRIVATE
	# 		src/client/world_app/server_connection.cpp
	# 		src/client/world_app/hud/character.cpp
	# 		src/client/world_app/hud/environment.cpp
	# 		src/client/world_app/hud/movement.cpp
	# 		src/client/world_app/hud/social.cpp
	# 		src/client/world_app/hud/layout.cpp
	# 		src/client/world_app/hud/status.cpp
	# 		src/client/world_app/start_session.cpp
	# 		src/client/world_app/update_frame.cpp
	# 		src/client/world_app/session_loop.cpp)
	# 	target_link_libraries(physics_viewer PRIVATE network client_replication)
	# endif()
	
	strict(physics_viewer)
endif()
