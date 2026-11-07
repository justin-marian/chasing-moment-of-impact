# Keep the fetched dependencies lean: no docs, no tests, no examples.
set(GLFW_BUILD_DOCS     OFF CACHE BOOL   "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL   "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL   "" FORCE)
set(GLAD_PROFILE        core CACHE STRING "" FORCE)
set(GLAD_API            "gl=4.6" CACHE STRING "" FORCE)
set(GLAD_REPRODUCIBLE   ON CACHE BOOL    "" FORCE)

FetchContent_Declare(glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG        3.4
	GIT_SHALLOW    TRUE
	GIT_SUBMODULES "")

FetchContent_Declare(glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG        1.0.1
	GIT_SHALLOW    TRUE
	GIT_SUBMODULES "")

FetchContent_Declare(glad
	GIT_REPOSITORY https://github.com/Dav1dde/glad.git
	GIT_TAG        v0.1.36
	GIT_SHALLOW    TRUE
	GIT_SUBMODULES "")

FetchContent_Declare(imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG        v1.91.8
	GIT_SHALLOW    TRUE
	GIT_SUBMODULES "")

FetchContent_MakeAvailable(glfw glm glad imgui)

# Dear ImGui, wired to the GLFW platform backend and to the project's
# unmodified upstream OpenGL renderer backend.
add_library(ui_imgui
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

target_include_directories(ui_imgui PUBLIC
	${imgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}/backends)
target_include_directories(ui_imgui PRIVATE src)
target_link_libraries(ui_imgui PUBLIC glfw)

# [COMMENTED FOR COLLISION FOCUS] glTF loader and skinned-animation runtime.
# if(BUILD_MMO)
# 	add_library(animation
# 		src/client/animation/gltf/reader.cpp
# 		src/client/animation/gltf/scene_loader.cpp
# 		src/client/animation/gltf/mesh_loader.cpp
# 		src/client/animation/animator.cpp
# 		src/client/animation/model.cpp)
#
# 	target_link_libraries(animation PUBLIC glm::glm nlohmann_json::nlohmann_json)
# 	target_include_directories(animation PUBLIC src)
# 	strict(animation)
# endif()

# OpenGL renderer and scene drawing.
add_library(render_gl
	src/client/render_gl/controls.cpp
	src/client/render_gl/rigid_bodies/session_renderer.cpp
	src/client/render_gl/meshes/unit_shapes.cpp
	src/client/render_gl/gpu_resources.cpp
	src/client/render_gl/rigid_bodies/draw_environment.cpp
	src/client/render_gl/rigid_bodies/draw_objects.cpp
	src/client/render_gl/impact_history.cpp
	src/client/render_gl/interface_style.cpp)

target_link_libraries(render_gl PUBLIC sim_core glm::glm glfw glad ui_imgui PRIVATE project_files)

# [COMMENTED FOR COLLISION FOCUS] MMO World Scene rendering
# if(BUILD_MMO)
# 	target_sources(render_gl PRIVATE
# 		src/client/render_gl/world_scene/draw_frame.cpp
# 		src/client/render_gl/world_scene/avatars.cpp
# 		src/client/render_gl/world_scene/assets.cpp
# 		src/client/render_gl/world_scene/impacts.cpp
# 		src/client/render_gl/world_scene/overlays.cpp)
# 	target_link_libraries(render_gl PUBLIC animation gameplay_model)
# 	target_compile_definitions(render_gl PUBLIC SIM_BUILD_MMO)
# endif()

target_compile_definitions(render_gl PUBLIC GLFW_INCLUDE_NONE)
target_compile_definitions(render_gl PRIVATE ASSET_ROOT="${CMAKE_SOURCE_DIR}")
target_include_directories(render_gl SYSTEM PRIVATE ${glfw_SOURCE_DIR}/deps)
strict(render_gl)
