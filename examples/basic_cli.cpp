#include "tinycli/tinycli.hpp"

#include <cstdio>
#include <cstring>

typedef tinycli::Cli<8, 96, 8, 32, 128> DeviceCli;

static tinycli::Status echoCommand(
    std::size_t argc,
    const tinycli::Argument* argv,
    DeviceCli::ResponseType& response,
    void*) {
    for (std::size_t i = 0; i < argc; ++i) {
        if (i != 0 && !response.append(' ')) return tinycli::Status::OutputFull;
        if (!response.append(argv[i].data, argv[i].length)) return tinycli::Status::OutputFull;
    }
    return tinycli::Status::Ok;
}

int main() {
    DeviceCli cli;
    if (cli.addCommand("echo", &echoCommand) != tinycli::Status::Ok) return 1;

    DeviceCli::ResponseType response;
    const tinycli::Status status = cli.executeLine("echo sensor ready", response);
    if (status != tinycli::Status::Ok || std::strcmp(response.output, "sensor ready") != 0) return 2;
    std::fputs(response.output, stdout);
    std::fputc('\n', stdout);
    return 0;
}
