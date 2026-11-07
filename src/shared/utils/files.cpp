/**
 * @file
 * Resolve the executable location shared by asset, shader, font and
 * environment discovery.
 */

#include "shared/utils/files.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif


namespace utils {

/**
 * Determine the directory containing the currently running executable.
 *
 * On Windows, the wide API is used to preserve non-ASCII extraction paths
 * and to support long Windows paths.
 *
 * @return Directory containing the executable, or an empty path on failure.
 */
std::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::wstring path(32768, L'\0');

    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length >= path.size())
        return {};

    path.resize(length);
    return std::filesystem::path(path).parent_path();
#else
    std::error_code error;
    const auto path = std::filesystem::read_symlink("/proc/self/exe", error);
    return error ? std::filesystem::path{} : path.parent_path();
#endif
}

} // namespace utils
