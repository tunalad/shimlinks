#include "config.hpp"

#include <iostream>

using namespace std;

namespace fs = std::filesystem;

static bool non_empty_string(Yaml::Node &node) {
    return node.IsScalar() && !node.As<string>().empty();
}

bool load_config(const fs::path &conf, Yaml::Node &root) {
    try {
        Yaml::Parse(root, conf.c_str());
    } catch (const Yaml::Exception &e) {
        cerr << "shimlinks: failed to parse config " << conf << ": " << e.what()
             << "\n";
        return false;
    }
    return true;
}

bool validate_config(const fs::path &conf, Yaml::Node &root) {
    bool ok = true;
    if (!root.IsMap()) {
        cerr << "shimlinks: " << conf << ": config is not a mapping\n";
        return false;
    }
    if (!non_empty_string(root["shimdir"])) {
        cerr << "shimlinks: " << conf
             << ": \"shimdir\" must be a non-empty path\n";
        ok = false;
    }
    Yaml::Node &rules = root["rules"];
    if (rules.IsNone()) {
        cerr << "shimlinks: " << conf << ": warning: no rules defined\n";
    } else if (!rules.IsSequence()) {
        cerr << "shimlinks: " << conf << ": \"rules\" must be a list\n";
        ok = false;
    } else {
        for (size_t i = 0; i < rules.Size(); i++) {
            Yaml::Node &rule = rules[i];
            if (!rule.IsMap()) {
                cerr << "shimlinks: " << conf << ": rules[" << i
                     << "] must be a mapping\n";
                ok = false;
                continue;
            }
            if (!non_empty_string(rule["target"])) {
                cerr << "shimlinks: " << conf << ": rules[" << i
                     << "]: \"target\" must be a non-empty path\n";
                ok = false;
            }
            if (!non_empty_string(rule["rewrite"])) {
                cerr << "shimlinks: " << conf << ": rules[" << i
                     << "]: \"rewrite\" must be a non-empty path\n";
                ok = false;
            }
            Yaml::Node &only = rule["only"];
            if (only.IsNone()) {
                // ignore rules without "only" list
            } else if (!only.IsSequence()) {
                cerr << "shimlinks: " << conf << ": rules[" << i
                     << "]: \"only\" must be a list\n";
                ok = false;
            } else {
                for (size_t j = 0; j < only.Size(); j++) {
                    if (!non_empty_string(only[j])) {
                        cerr << "shimlinks: " << conf << ": rules[" << i
                             << "]: \"only\" entry " << j
                             << " must be a name\n";
                        ok = false;
                    }
                }
            }
            for (auto it = rule.Begin(); it != rule.End(); it++) {
                const string &key = (*it).first;
                if (key != "target" && key != "rewrite" && key != "only" &&
                    key != "name" && key != "mode") {
                    cerr << "shimlinks: " << conf << ": rules[" << i
                         << "]: unknown key \"" << key << "\" ignored\n";
                }
            }
        }
    }
    return ok;
}

vector<pair<string, string>> find_rules(Yaml::Node &root, const string &cmd) {
    vector<pair<string, string>> out;
    Yaml::Node &rules = root["rules"];
    for (size_t i = 0; i < rules.Size(); i++) {
        Yaml::Node &only = rules[i]["only"];
        for (size_t j = 0; j < only.Size(); j++) {
            if (only[j].As<string>() == cmd) {
                out.emplace_back(rules[i]["target"].As<string>(),
                                 rules[i]["rewrite"].As<string>());
            }
        }
    }
    return out;
}

set<string> collect_names(Yaml::Node &root) {
    set<string> names;
    Yaml::Node &rules = root["rules"];
    for (size_t i = 0; i < rules.Size(); i++) {
        Yaml::Node &only = rules[i]["only"];
        for (size_t j = 0; j < only.Size(); j++) {
            string name = only[j].As<string>();
            // never shim our own name
            if (name != "shimlinks") {
                names.insert(name);
            }
        }
    }
    return names;
}
