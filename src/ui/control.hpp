#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace mdev::bdg {

std::filesystem::path determine_boost_root();

std::optional<std::string> get_root_library_name();

} // namespace mdev::bdg
