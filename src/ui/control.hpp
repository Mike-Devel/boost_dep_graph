#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <core/utils.hpp>

namespace mdev::bdg {

std::filesystem::path determine_boost_root();

std::optional<String_t> get_root_library_name();

} // namespace mdev::bdg
