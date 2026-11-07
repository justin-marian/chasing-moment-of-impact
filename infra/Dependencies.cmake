include(FetchContent)

# Each FetchContent_Declare is preceded by a check for a vendored copy under
# tools/vendor/<name>. When one is present the vendored source is used and no
# network access happens; otherwise the pinned git tag is fetched.
if(EXISTS "${CMAKE_SOURCE_DIR}/tools/vendor/json/CMakeLists.txt")
	set(FETCHCONTENT_SOURCE_DIR_JSON "${CMAKE_SOURCE_DIR}/tools/vendor/json")
endif()

FetchContent_Declare(json
	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG        v3.11.3
	GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(json)
