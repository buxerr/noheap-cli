#include "tinycli/tinycli.hpp"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

static std::size_t allocationCount = 0;

void* operator new(std::size_t size) {
    ++allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    ++allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

typedef tinycli::Cli<3, 40, 6, 16, 64> TestCli;
typedef TestCli::ResponseType TestResponse;
typedef tinycli::Cli<1, 16, 2, 8, 4> SmallOutputCli;

static tinycli::Status echo(
    std::size_t argc,
    const tinycli::Argument* argv,
    TestResponse& response,
    void*) {
    for (std::size_t i = 0; i < argc; ++i) {
        if (i != 0 && !response.append(' ')) return tinycli::Status::OutputFull;
        if (!response.append(argv[i].data, argv[i].length)) return tinycli::Status::OutputFull;
    }
    return tinycli::Status::Ok;
}

static tinycli::Status failHandler(std::size_t, const tinycli::Argument*, TestResponse&, void*) {
    return tinycli::Status::HandlerError;
}

static tinycli::Status smallEcho(
    std::size_t argc,
    const tinycli::Argument* argv,
    SmallOutputCli::ResponseType& response,
    void*) {
    for (std::size_t i = 0; i < argc; ++i) {
        if (i != 0 && !response.append(' ')) return tinycli::Status::OutputFull;
        if (!response.append(argv[i].data, argv[i].length)) return tinycli::Status::OutputFull;
    }
    return tinycli::Status::Ok;
}

#define CHECK_STATUS(actual, expected) \
    do { \
        if ((actual) != (expected)) return __LINE__; \
    } while (false)

int main() {
    const std::size_t allocationsBeforeSetup = allocationCount;
    TestCli cli;
    CHECK_STATUS(cli.addCommand("echo", &echo), tinycli::Status::Ok);
    CHECK_STATUS(cli.addCommand("error", &failHandler), tinycli::Status::Ok);
    CHECK_STATUS(cli.addCommand("echo", &echo), tinycli::Status::DuplicateCommand);
    CHECK_STATUS(cli.addCommand("bad name", &echo), tinycli::Status::InvalidCommandName);
    CHECK_STATUS(cli.addCommand("null-handler", nullptr), tinycli::Status::InvalidHandler);
    CHECK_STATUS(cli.addCommand("full", &echo), tinycli::Status::Ok);
    CHECK_STATUS(cli.addCommand("overflow", &echo), tinycli::Status::CommandTableFull);
    char temporaryName[] = "temporary";
    TestCli temporaryCli;
    CHECK_STATUS(temporaryCli.addCommand(temporaryName, &echo), tinycli::Status::Ok);
    temporaryName[0] = 'X';
    TestResponse temporaryResponse;
    CHECK_STATUS(temporaryCli.executeLine("temporary stable", temporaryResponse), tinycli::Status::Ok);
    if (std::strcmp(temporaryResponse.output, "stable") != 0) return __LINE__;

    TestResponse response;
    CHECK_STATUS(cli.executeLine("echo one \"two words\" \"\" escaped\\ space", response), tinycli::Status::Ok);
    if (std::strcmp(response.output, "one two words  escaped space") != 0) return __LINE__;
    const std::size_t originalLength = response.outputLength;
    if (!response.append(response.output, originalLength)) return __LINE__;
    if (response.outputLength != originalLength * 2) return __LINE__;
    if (std::memcmp(response.output, response.output + originalLength, originalLength) != 0) return __LINE__;
    CHECK_STATUS(cli.executeLine(" \t ", response), tinycli::Status::EmptyLine);
    CHECK_STATUS(cli.executeLine(nullptr, response), tinycli::Status::NullInput);
    CHECK_STATUS(cli.executeLine("unknown", response), tinycli::Status::UnknownCommand);
    CHECK_STATUS(cli.executeLine("echo \"unterminated", response), tinycli::Status::UnclosedQuote);
    CHECK_STATUS(cli.executeLine("echo bad\\q", response), tinycli::Status::InvalidEscape);
    CHECK_STATUS(cli.executeLine("echo 12345678901234567", response), tinycli::Status::TokenTooLong);
    CHECK_STATUS(cli.executeLine("echo a b c d e f", response), tinycli::Status::TooManyArguments);
    CHECK_STATUS(cli.executeLine(
        "echo 12345678901234567890123456789012345678901234567890",
        response), tinycli::Status::LineTooLong);
    CHECK_STATUS(cli.executeLine("error", response), tinycli::Status::HandlerError);
    if (response.output[0] != '\0') return __LINE__;
    if (allocationCount != allocationsBeforeSetup) return __LINE__;

    SmallOutputCli small;
    if (small.addCommand("echo", &smallEcho) != tinycli::Status::Ok) return __LINE__;
    SmallOutputCli::ResponseType smallResponse;
    if (small.executeLine("echo 12345", smallResponse) != tinycli::Status::OutputFull) return __LINE__;

    std::puts("tinycli: fixed-buffer parser tests passed without heap allocations");
    return 0;
}
