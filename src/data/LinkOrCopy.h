#pragma once

#include <filesystem>

namespace spirula::files {

enum class Transfer { Failed, Linked, Copied };

inline Transfer link_or_copy(const std::filesystem::path& from,
                             const std::filesystem::path& to, std::error_code& error) {
    std::filesystem::create_hard_link(from, to, error);
    if (!error) return Transfer::Linked;
    error.clear();
    return std::filesystem::copy_file(from, to, error) ? Transfer::Copied : Transfer::Failed;
}

}  // namespace spirula::files
