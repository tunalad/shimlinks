#pragma once

#include "yaml/Yaml.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

bool load_config(const std::filesystem::path &conf, Yaml::Node &root);
bool validate_config(const std::filesystem::path &conf, Yaml::Node &root);
std::vector<std::pair<std::string, std::string>>
find_rules(Yaml::Node &root, const std::string &cmd);
std::set<std::string> collect_names(Yaml::Node &root);
