#pragma once

#include <string>

std::string expand_home(std::string s);
std::string config_path();
bool dbg_enabled();
void dbg(const std::string &msg);
