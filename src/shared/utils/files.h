/**
 * @file
 * Resolve project files consistently and read binary assets with contextual errors.
 */
#pragma once

#include <filesystem>
#include <stdexcept>
#include <string_view>


namespace utils {

/**
 * Return the directory containing the running executable without relying on
 * its working directory.
 *
 * @return Absolute executable directory, or an empty path when the platform
 *         cannot report it.
 */
std::filesystem::path executableDirectory();


/**
 * Locate an asset from the working directory, extracted executable ancestors,
 * or the configured source root.
 *
 * Executable-relative lookup precedes the development fallback so a moved
 * release uses its own assets. Releases keep exports and builds below the
 * root that owns the assets and prepared data.
 *
 * @param relative   Project-relative supplied asset path resolved through the shared lookup policy.
 * @param sourceRoot Development-only fallback used after executable-relative candidates.
 * @return First existing asset path.
 * @throws std::runtime_error If no supported location contains the asset.
 */
inline std::filesystem::path findAsset(std::string_view relative, const std::filesystem::path& sourceRoot) {
    for (const auto& base : {std::filesystem::current_path(), std::filesystem::current_path().parent_path()}) {
        if (std::filesystem::exists(base / relative))
            return base / relative;
    }

    for (auto base = executableDirectory(); !base.empty(); base = base.parent_path()) {
        if (std::filesystem::exists(base / relative))
            return base / relative;
        if (base == base.root_path())
            break;
    }

    if (std::filesystem::exists(sourceRoot / relative))
        return sourceRoot / relative;

    throw std::runtime_error("Missing asset: " + std::string(relative));
}

} // namespace utils
