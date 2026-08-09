#include "config.hpp"
#include "install.hpp"
#include "resolve.hpp"
#include "util.hpp"
#include "yaml/Yaml.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

using namespace std;

namespace fs = std::filesystem;

// tracks the shadow symlinks apply_rules creates and removes them
struct LinkGuard {
    vector<fs::path> paths;
    ~LinkGuard() {
        error_code ec;
        for (const fs::path &p : paths) {
            if (fs::is_symlink(p, ec)) {
                fs::remove(p, ec);
                if (ec) {
                    cerr << "shimlinks: warning: failed to remove symlink " << p
                         << ": " << ec.message() << "\n";
                }
            }
        }
    }
};

struct Config {
    Yaml::Node root;
    fs::path shim_dir;
};

// function to make us survive SIGINT/SIGTERM signals
static void on_signal(int) {}

static void print_usage() {
    cout << "shimlinks " VERSION "\n"
            "Runs a command while its dotfile/state dirs temporarily point\n"
            "somewhere else.\n"
            "\n"
            "USAGE\n"
            "  shimlinks [OPTIONS]      manage shims (run by this name)\n"
            "  <shim-name> [ARGS...]    run a command through a shim\n"
            "\n"
            "OPTIONS\n"
            "  --install      create shims for every name in the config (safe "
            "to "
            "re-run)\n"
            "  --uninstall    remove shims that are no longer in the config\n"
            "  -h, --help     show this help\n"
            "  --version      show version\n"
            "\n"
            "CONFIG\n"
            "  ~/.config/shimlinks/config.yaml\n"
            "    shimdir:  where shims live (must be in your PATH)\n"
            "    rules:    target/rewrite pairs; \"only\" names become shims\n"
            "\n"
            "ENVIRONMENT\n"
            "  SHIMLINKS_CONFIG    use this file instead of the config above\n"
            "  SHIMLINKS_DEBUG=1   verbose logging to stderr\n"
            "\n"
            "See 'man shimlinks' for setup and examples.\n";
}

static void print_version() { cout << "shimlinks " VERSION "\n"; }

enum class Opt { Help, Version, Install, Uninstall, Unknown, Name };

static Opt parse_option(const string &flag) {
    if (flag == "--help" || flag == "-h")
        return Opt::Help;
    if (flag == "--version")
        return Opt::Version;
    if (flag == "--install")
        return Opt::Install;
    if (flag == "--uninstall")
        return Opt::Uninstall;
    if (flag[0] == '-')
        return Opt::Unknown;
    return Opt::Name;
}

static bool load_shimdir(const fs::path &conf, Config &cfg) {
    if (!load_config(conf, cfg.root)) {
        return false;
    }
    dbg("config parsed: " + conf.string());

    if (!validate_config(conf, cfg.root)) {
        return false;
    }

    string shimdir = cfg.root["shimdir"].As<string>();
    cfg.shim_dir = expand_home(shimdir);
    dbg("shimdir=" + cfg.shim_dir.string());
    if (!shimdir_in_path(cfg.shim_dir)) {
        cerr << "shimlinks: shimdir \"" << cfg.shim_dir.string()
             << "\" is not in PATH; add 'export PATH=\""
             << cfg.shim_dir.string() << ":$PATH\"' to your shell config\n";
        return false;
    }
    return true;
}

static int run_install(const fs::path &conf, const fs::path &self_exe) {
    Config cfg;
    if (!load_shimdir(conf, cfg)) {
        return 1;
    }
    return do_install(cfg.shim_dir, self_exe, collect_names(cfg.root));
}

static int run_uninstall(const fs::path &conf, const fs::path &self_exe) {
    Config cfg;
    if (!load_shimdir(conf, cfg)) {
        return 1;
    }
    return do_uninstall(cfg.shim_dir, self_exe, collect_names(cfg.root));
}

static int apply_rules(Yaml::Node &root, const string &cmd, LinkGuard &guard) {
    auto rules = find_rules(root, cmd);
    if (rules.empty()) {
        dbg("no rule matched");
    }
    for (const auto &[tgt, rw] : rules) {
        dbg("rule matched: target=" + tgt + " rewrite=" + rw);
        const fs::path target = expand_home(tgt);
        const fs::path rewrite = expand_home(rw);

        error_code ec;
        fs::create_directories(target.parent_path(), ec);
        if (ec) {
            cerr << "shimlinks: failed to create parent of " << target << ": "
                 << ec.message() << "\n";
            return 1;
        }
        if (fs::is_symlink(target, ec)) {
            fs::remove(target, ec);
        } else if (fs::exists(target, ec)) {
            cerr << "shimlinks: refusing to overwrite non-symlink path "
                 << target << "\n";
            return 1;
        }
        fs::create_symlink(rewrite, target, ec);
        if (ec) {
            cerr << "shimlinks: failed to create symlink " << target << " -> "
                 << rewrite << ": " << ec.message() << "\n";
            return 1;
        }
        guard.paths.push_back(target);
        dbg("shadow symlink created: " + target.string() + " -> " +
            rewrite.string());
    }
    return 0;
}

static int exec_program(char *argv[], const string &cmd,
                        const fs::path &shim_dir, const fs::path &self_exe) {
    fs::path real_bin = resolve_bin(cmd, shim_dir);
    if (real_bin.empty()) {
        cerr << "shimlinks: " << cmd << ": command not found\n";
        return 127;
    }
    dbg("resolve: real_bin=" + real_bin.string());

    error_code ec;
    // never run ourselves. Shim pointing at this binary means the real
    // program is missing from PATH
    if (fs::canonical(real_bin, ec) == self_exe) {
        cerr << "shimlinks: refusing to exec self: " << real_bin << "\n";
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        int e = errno;
        cerr << "shimlinks: fork failed: " << strerror(e) << "\n";
        return 1;
    }

    if (pid == 0) {
        argv[0] = const_cast<char *>(real_bin.c_str());
        dbg("child: execv(" + real_bin.string() + ")");
        execv(real_bin.c_str(), argv);
        int e = errno;
        cerr << "shimlinks: exec failed: " << strerror(e) << "\n";
        _exit(127);
    }

    int status = 0;
    // retry the wait if a signal interrupted it
    while (waitpid(pid, &status, 0) == -1 && errno == EINTR) {
    }

    dbg("parent: waitpid status=" + to_string(status));

    // mirror shell conventions: the child's status or 128 + signal number
    return WIFEXITED(status)     ? WEXITSTATUS(status)
           : WIFSIGNALED(status) ? 128 + WTERMSIG(status)
                                 : 1;
}

int main(int argc, char *argv[]) {
    fs::path conf = config_path();
    string cmd = fs::path(argv[0]).filename().string();

    // "shimlinks" is our own name, other name means we invoked a shim
    bool cli = cmd == "shimlinks";

    error_code sec;
    fs::path self_exe = fs::read_symlink("/proc/self/exe", sec);
    const char *home = getenv("HOME");
    dbg("cmd=" + cmd + " argv0=" + argv[0] + " HOME=" + (home ? home : "") +
        " conf=" + conf.string());
    dbg("self_exe=" + self_exe.string());

    if (cli) {
        if (argc == 1) {
            print_usage();
            return 1;
        }
        switch (parse_option(argv[1])) {
        case Opt::Help:
            print_usage();
            return 0;
        case Opt::Version:
            print_version();
            return 0;
        case Opt::Install:
            return run_install(conf, self_exe);
        case Opt::Uninstall:
            return run_uninstall(conf, self_exe);
        case Opt::Unknown:
            cerr << "shimlinks: unknown option: " << argv[1] << "\n"
                 << "try 'shimlinks --help'\n";
            return 2;
        case Opt::Name:
            cerr << "shimlinks: can't wrap a program by name; use a shim "
                    "symlink "
                 << "in the shimdir (see 'shimlinks --help')\n";
            return 1;
        }
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    Config cfg;
    if (!load_shimdir(conf, cfg)) {
        return 1;
    }

    if (!fs::is_directory(cfg.shim_dir)) {
        cerr << "shimlinks: shimdir \"" << cfg.shim_dir.string()
             << "\" is not a directory; fix \"shimdir\" in the config so the "
                "shim "
                "can find the real program\n";
        return 1;
    }

    LinkGuard guard;
    if (int rc = apply_rules(cfg.root, cmd, guard)) {
        return rc;
    }

    return exec_program(argv, cmd, cfg.shim_dir, self_exe);
}
