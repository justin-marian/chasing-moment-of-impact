include(FetchContent)

# Fetch pinned dependencies into the build tree.
FetchContent_Declare(json
	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG        v3.11.3
	GIT_SHALLOW    TRUE
	GIT_SUBMODULES "")

FetchContent_MakeAvailable(json)
