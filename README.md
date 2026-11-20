# Embedded C++ No-Heap Command-Line Parser

A header-only C++11 command parser for embedded firmware CLIs. It uses compile-time fixed buffers for command registration, input lines, tokens, arguments, and response output, with no allocator, transport, logging, or operating-system dependency.

Use it when a firmware command interface needs predictable storage and should not pull in a full terminal or shell framework.

## Build and test

Requires CMake 3.16 or newer and a C++11 compiler:

```sh
git clone https://github.com/buxerr/noheap-cli.git
cd noheap-cli
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The library is an interface target with headers in `include/`. The example is in [examples/basic_cli.cpp](examples/basic_cli.cpp). CI builds on Linux and Windows with warnings treated as errors. The test executable overrides global `new` and verifies registration and parsing do not allocate.

## Install in PlatformIO

After the `v0.1.0` GitHub release is available, a PlatformIO project can consume the header-only parser directly from the tagged repository:

```ini
[env:my-board]
lib_deps = https://github.com/buxerr/noheap-cli.git#v0.1.0
```

The package manifest is also ready for the PlatformIO Registry so users can find it by search and install it with a version-pinned registry dependency.

## Set fixed capacities

Instantiate `tinycli::Cli` with maximum command count, input-line bytes, token count, token bytes, and response bytes. `MaxTokens` includes the command name. Capacities are compile-time constants, so you can size memory to your command set and transport.

Register handlers with `addCommand`. The command name is copied into the fixed-size registry; handler context remains caller-owned. Handlers receive argument views and a bounded response buffer. `append` returns false when the output capacity is exhausted. The parser can sit behind UART, TCP, USB, or a scripted host test.

The parser supports whitespace-separated tokens, double-quoted spans, and backslash escapes for spaces, quotes, and backslashes. It reports overlong lines or tokens, excess arguments, unclosed quotes, invalid escapes, unknown commands, and table overflow as explicit statuses.

## Scope and limitations

This is a parser and dispatcher core, not a terminal driver or shell framework. It does not provide line editing, history, authorization, transport framing, command help generation, or thread safety. User-supplied handlers can allocate; the no-heap guarantee covers this library's registration and parsing paths.

## License

MIT. See [LICENSE](LICENSE).
