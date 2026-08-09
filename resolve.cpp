#include "resolve.hpp"

#include "util.hpp"

#include <cstdlib>
#include <string_view>
#include <unistd.h>
#include <vector>

using namespace std;

namespace fs = std::filesystem;

static vector<string_view> split_path() {
    string_view s = getenv("PATH") ? getenv("PATH") : "";
    vector<string_view> dirs;
    size_t start = 0;
    while (start <= s.size()) {
        size_t colon = s.find(':', start);
        dirs.push_back(s.substr(start, colon == string_view::npos
                                           ? string_view::npos
                                           : colon - start));
        if (colon == string_view::npos)
            break;
        start = colon + 1;
    }
    return dirs;
}

fs::path resolve_bin(const string &cmd, const fs::path &shim_dir) {
    string path_str = getenv("PATH") ? getenv("PATH") : "";
    dbg("resolve: PATH=" + path_str);
    for (string_view dir : split_path()) {
        if (dir.empty())
            continue;
        fs::path candidate = fs::path(dir) / cmd;
        bool skip = fs::path(dir) == shim_dir;
        bool regular = fs::is_regular_file(candidate);
        bool exec_ok = access(candidate.c_str(), X_OK) == 0;
        dbg("resolve: candidate=" + candidate.string() + " skip_shim=" +
            (skip ? "y" : "n") + " regular=" + (regular ? "y" : "n") +
            " exec=" + (exec_ok ? "y" : "n"));
        if (!skip && regular && exec_ok) {
            return candidate;
        }
    }
    return {};
}

bool shimdir_in_path(const fs::path &shim_dir) {
    for (string_view dir : split_path()) {
        if (!dir.empty() && fs::path(dir) == shim_dir) {
            return true;
        }
    }
    return false;
}

bool points_at_us(const fs::path &p, const fs::path &self) {
    error_code ec;
    if (!fs::is_symlink(p, ec)) {
        return false;
    }
    return fs::canonical(p, ec) == self;
}
