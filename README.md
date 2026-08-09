# shimlinks

Runs a command while its dotfile/state dirs temporarily point somewhere else.

## Features

- Redirects dotfile/state dirs to different locations with symlinks
- Creates the redirects only while the wrapped command is running
- Preserves the wrapped command's exit status
- Accepts the boxxy config format

## Install

Requires a C++17 compiler and `make`. Build settings (compiler, flags, install paths) live in `config.mk`.

```
git clone https://github.com/tunalad/shimlinks.git
cd shimlinks
make && make install
```

Or, to install under your home directory:

```
make install PREFIX=$HOME/.local
```

## Configuration

The config file is `~/.config/shimlinks/config.yaml`. See `man shimlinks` for the full format.

```
shimdir: ~/.local/shimlinks
rules:
  - target: ~/.java
    rewrite: ~/.local/share/java
    only:
      - java
  - target: ~/.gnupg
    rewrite: ~/.local/share/gnupg
    only:
      - gpg
```

`shimdir` must appear first in your `PATH`. The `only` list must be in block form; flow-style lists like `only: [java]` are rejected with an error. A rule without an `only` list creates no shims and is ignored.

## Usage

```
shimlinks --install
```

Creates a shim in the shimdir for every name in the config. Safe to re-run.

```
java -jar app.jar
```

Runs the real program through the shim, redirecting your dotfiles while it runs and cleaning up afterwards.

```
shimlinks --uninstall
```

Removes shims that are no longer in the config.

## Environment

`SHIMLINKS_CONFIG` overrides the config path.

`SHIMLINKS_DEBUG=1` enables verbose logging.

## Acknowledgments

Inspired by [boxxy](https://github.com/queer/boxxy), which redirects a program's file operations with Linux namespaces, without touching the filesystem. Written in R*st.

[mini-yaml](https://github.com/jimmiebergmann/mini-yaml) by Jimmie Bergmann is vendored for YAML parsing.

## License

BSD 2-Clause
