#include "util.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace std;

namespace fs = std::filesystem;

bool dbg_enabled() {
    // added debugging cuz this program legit forkbombed my ass
    // due to a typo in the config lmaooo
    static bool on = getenv("SHIMLINKS_DEBUG") != nullptr;
    return on;
}

void dbg(const string &msg) {
    if (dbg_enabled()) {
        cerr << "[dbg] " << msg << "\n";
    }
}

string expand_home(string s) {
    const char *home = getenv("HOME");
    string h = home ? home : "";
    if (s.rfind("${HOME}", 0) == 0) {
        return h + s.substr(7);
    }
    if (s.rfind("$HOME", 0) == 0) {
        return h + s.substr(5);
    }
    if (!s.empty() && s[0] == '~') {
        return h + s.substr(1);
    }
    return s;
}

string config_path() {
    const char *env = getenv("SHIMLINKS_CONFIG");
    if (env && *env) {
        return expand_home(env);
    }
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        return (fs::path(xdg) / "shimlinks/config.yaml").string();
    }
    return expand_home("~/.config/shimlinks/config.yaml");
}
