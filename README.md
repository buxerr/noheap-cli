# No-Heap CLI

A small header-only C++11 command parser for firmware. It uses compile-time fixed buffers for the command table, line tokens, arguments, and response output. The library has no allocator, transport, logging, or operating-system dependency.

## Build and test

Use CMake 3.16 or newer and a C++11 compiler:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

CI builds with warnings treated as errors on Linux and Windows. The test executable overrides global new and verifies command registration and parsing do not allocate.

## Configure capacities

Instantiate tinycli::Cli with maximum command count, input-line bytes, token count, token bytes, and response bytes. MaxTokens includes the command name. Capacities are compile-time constants, so applications can size memory to their transport and command set.

Register handlers with addCommand. Command names are copied into the fixed-size registry; handler context remains caller-owned. Handlers receive argument views and a bounded Response buffer; append returns false when the output capacity is exhausted. The same parser can sit behind UART, TCP, USB, or a scripted host test.

The parser supports whitespace-separated tokens, double-quoted spans, and backslash escapes for spaces, quotes, and backslashes. It reports overlong lines/tokens, too many arguments, unclosed quotes, invalid escapes, unknown commands, and table overflow as explicit statuses.

## Scope

This is a parser and dispatch core, not a terminal driver or shell framework. It does not provide history, editing, authorization, transport framing, command help generation, or thread safety. A handler can still allocate; the zero-heap guarantee covers this library's registration and parsing paths.

MIT licensed. See LICENSE.
