#include "install.hpp"

#include "resolve.hpp"
#include "util.hpp"

#include <iostream>

using namespace std;

namespace fs = std::filesystem;

int do_install(const fs::path &shim_dir, const fs::path &self,
               const set<string> &names) {
    error_code ec;
    fs::create_directories(shim_dir, ec);
    if (ec) {
        cerr << "shimlinks: failed to create shimdir " << shim_dir << ": "
             << ec.message() << "\n";
        return 1;
    }
    if (names.empty()) {
        cout << "shimlinks: no shims to install (no rules with \"only\" "
                "entries)\n";
    }
    for (const string &name : names) {
        fs::path p = shim_dir / name;
        if (fs::is_symlink(p, ec) || fs::exists(p, ec)) {
            if (points_at_us(p, self)) {
                dbg("install: already installed " + name);
                cout << "already installed: " << name << "\n";
            } else {
                cerr << "shimlinks: warning: " << p
                     << " exists and is not ours, leaving it\n";
            }
            continue;
        }
        fs::create_symlink(self, p, ec);
        if (ec) {
            cerr << "shimlinks: failed to create symlink " << p << " -> "
                 << self << ": " << ec.message() << "\n";
            continue;
        }
        cout << "created: " << name << "\n";
    }
    return 0;
}

int do_uninstall(const fs::path &shim_dir, const fs::path &self,
                 const set<string> &names, bool all) {
    error_code ec;
    if (!fs::is_directory(shim_dir, ec)) {
        cout << "no shims to remove\n";
        return 0;
    }
    size_t removed = 0;
    for (const auto &entry : fs::directory_iterator(shim_dir, ec)) {
        const fs::path &p = entry.path();
        string name = p.filename().string();
        if (!all && (names.count(name) || !points_at_us(p, self)))
            continue;
        if (all && !fs::is_symlink(p, ec))
            continue;
        fs::remove(p, ec);
        if (ec) {
            cerr << "shimlinks: failed to remove " << p << ": " << ec.message()
                 << "\n";
            continue;
        }
        cout << (all ? "removed: " : "removed stale: ") << name << "\n";
        ++removed;
    }
    if (removed == 0) {
        cout << "no shims to remove\n";
    }
    return 0;
}
