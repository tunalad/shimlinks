#pragma once

#include <filesystem>
#include <string>

std::filesystem::path resolve_bin(const std::string &cmd,
                                  const std::filesystem::path &shim_dir);
bool shimdir_in_path(const std::filesystem::path &shim_dir);
bool points_at_us(const std::filesystem::path &p,
                  const std::filesystem::path &self);
