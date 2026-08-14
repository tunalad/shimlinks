#pragma once

#include <filesystem>
#include <set>
#include <string>

int do_install(const std::filesystem::path &shim_dir,
               const std::filesystem::path &self,
               const std::set<std::string> &names);
int do_uninstall(const std::filesystem::path &shim_dir,
                 const std::filesystem::path &self,
                 const std::set<std::string> &names, bool all);
