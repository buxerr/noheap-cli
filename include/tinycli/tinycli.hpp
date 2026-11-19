#ifndef TINYCLI_TINYCLI_HPP
#define TINYCLI_TINYCLI_HPP

#include <cstddef>
#include <cstring>

namespace tinycli {

enum class Status {
    Ok,
    EmptyLine,
    NullInput,
    LineTooLong,
    TooManyArguments,
    TokenTooLong,
    UnclosedQuote,
    InvalidEscape,
    UnknownCommand,
    InvalidCommandName,
    CommandTableFull,
    DuplicateCommand,
    InvalidHandler,
    OutputFull,
    HandlerError,
};

struct Argument {
    const char* data;
    std::size_t length;

    bool equals(const char* literal) const {
        if (literal == nullptr) return false;
        std::size_t literalLength = 0;
        while (literal[literalLength] != '\0') ++literalLength;
        return literalLength == length && std::memcmp(data, literal, length) == 0;
    }
};

template <std::size_t OutputCapacity>
struct Response {
    static_assert(OutputCapacity > 0, "OutputCapacity must reserve a NUL byte");

    char output[OutputCapacity];
    std::size_t outputLength;
    Status status;

    Response() : output(), outputLength(0), status(Status::Ok) {}

    void reset() {
        outputLength = 0;
        output[0] = '\0';
        status = Status::Ok;
    }

    bool append(const char* value) {
        if (value == nullptr) {
            status = Status::HandlerError;
            return false;
        }
        std::size_t length = 0;
        const std::size_t remaining = OutputCapacity - 1 - outputLength;
        while (length <= remaining && value[length] != '\0') ++length;
        if (length > remaining) {
            status = Status::OutputFull;
            return false;
        }
        return append(value, length);
    }

    bool append(const char* value, std::size_t length) {
        if (value == nullptr && length != 0) {
            status = Status::HandlerError;
            return false;
        }
        if (length > OutputCapacity - 1 - outputLength) {
            status = Status::OutputFull;
            return false;
        }
        if (length != 0) std::memmove(output + outputLength, value, length);
        outputLength += length;
        output[outputLength] = '\0';
        return true;
    }

    bool append(char value) {
        return append(&value, 1);
    }
};

template <
    std::size_t MaxCommands,
    std::size_t LineCapacity,
    std::size_t MaxTokens,
    std::size_t TokenCapacity,
    std::size_t OutputCapacity>
class Cli {
public:
    typedef Response<OutputCapacity> ResponseType;
    typedef Status (*Handler)(std::size_t argc, const Argument* argv, ResponseType& response, void* context);

    static_assert(MaxCommands > 0, "MaxCommands must be positive");
    static_assert(LineCapacity > 0, "LineCapacity must be positive");
    static_assert(MaxTokens > 0, "MaxTokens must be positive");
    static_assert(TokenCapacity > 0, "TokenCapacity must be positive");

    Cli() : commandCount_(0), tokenCount_(0) {}

    Status addCommand(const char* name, Handler handler, void* context = nullptr) {
        if (name == nullptr || name[0] == '\0') return Status::InvalidCommandName;
        if (handler == nullptr) return Status::InvalidHandler;
        std::size_t nameLength = 0;
        while (nameLength <= TokenCapacity && name[nameLength] != '\0') ++nameLength;
        if (nameLength == 0 || nameLength > TokenCapacity) return Status::InvalidCommandName;
        for (std::size_t i = 0; i < nameLength; ++i) {
            if (!isCommandCharacter(name[i])) return Status::InvalidCommandName;
        }
        for (std::size_t i = 0; i < commandCount_; ++i) {
            if (commands_[i].nameLength == nameLength
                && std::memcmp(commands_[i].name, name, nameLength) == 0) {
                return Status::DuplicateCommand;
            }
        }
        if (commandCount_ >= MaxCommands) return Status::CommandTableFull;
        Command& command = commands_[commandCount_];
        std::memcpy(command.name, name, nameLength);
        command.name[nameLength] = '\0';
        command.nameLength = nameLength;
        command.handler = handler;
        command.context = context;
        ++commandCount_;
        return Status::Ok;
    }

    Status executeLine(const char* line, ResponseType& response) {
        response.reset();
        if (line == nullptr) return setStatus(response, Status::NullInput);

        std::size_t lineLength = 0;
        while (lineLength <= LineCapacity && line[lineLength] != '\0') ++lineLength;
        if (lineLength > LineCapacity) return setStatus(response, Status::LineTooLong);

        const Status parseStatus = tokenize(line, lineLength);
        if (parseStatus != Status::Ok) return setStatus(response, parseStatus);
        if (tokenCount_ == 0) return setStatus(response, Status::EmptyLine);

        std::size_t commandIndex = commandCount_;
        for (std::size_t i = 0; i < commandCount_; ++i) {
            if (commands_[i].nameLength == tokens_[0].length
                && std::memcmp(commands_[i].name, tokens_[0].text, tokens_[0].length) == 0) {
                commandIndex = i;
                break;
            }
        }
        if (commandIndex == commandCount_) return setStatus(response, Status::UnknownCommand);

        for (std::size_t i = 1; i < tokenCount_; ++i) {
            arguments_[i - 1].data = tokens_[i].text;
            arguments_[i - 1].length = tokens_[i].length;
        }
        const Status handlerStatus = commands_[commandIndex].handler(
            tokenCount_ - 1,
            arguments_,
            response,
            commands_[commandIndex].context);
        if (response.status == Status::OutputFull || response.status == Status::HandlerError) {
            return response.status;
        }
        return setStatus(response, handlerStatus);
    }

    std::size_t commandCount() const {
        return commandCount_;
    }

private:
    struct Command {
        char name[TokenCapacity + 1];
        std::size_t nameLength;
        Handler handler;
        void* context;
    };

    struct ParsedToken {
        char text[TokenCapacity + 1];
        std::size_t length;
    };

    Command commands_[MaxCommands];
    ParsedToken tokens_[MaxTokens];
    Argument arguments_[MaxTokens];
    std::size_t commandCount_;
    std::size_t tokenCount_;

    static Status setStatus(ResponseType& response, Status status) {
        response.status = status;
        return status;
    }

    static bool isCommandCharacter(char value) {
        return (value >= 'a' && value <= 'z')
            || (value >= 'A' && value <= 'Z')
            || (value >= '0' && value <= '9')
            || value == '_'
            || value == '-';
    }

    static bool isWhitespace(char value) {
        return value == ' ' || value == '\t' || value == '\r' || value == '\n';
    }

    Status beginToken(bool& active) {
        if (!active) {
            if (tokenCount_ >= MaxTokens) return Status::TooManyArguments;
            tokens_[tokenCount_].length = 0;
            tokens_[tokenCount_].text[0] = '\0';
            active = true;
        }
        return Status::Ok;
    }

    Status appendTokenCharacter(char value, bool& active) {
        const Status startStatus = beginToken(active);
        if (startStatus != Status::Ok) return startStatus;
        ParsedToken& token = tokens_[tokenCount_];
        if (token.length >= TokenCapacity) return Status::TokenTooLong;
        token.text[token.length++] = value;
        token.text[token.length] = '\0';
        return Status::Ok;
    }

    void finishToken(bool& active) {
        if (active) {
            ++tokenCount_;
            active = false;
        }
    }

    Status tokenize(const char* line, std::size_t lineLength) {
        tokenCount_ = 0;
        bool active = false;
        bool quoted = false;
        bool escaped = false;

        for (std::size_t i = 0; i < lineLength; ++i) {
            const char value = line[i];
            if (escaped) {
                if (value != '\\' && value != '"' && value != ' ') return Status::InvalidEscape;
                const Status appendStatus = appendTokenCharacter(value, active);
                if (appendStatus != Status::Ok) return appendStatus;
                escaped = false;
                continue;
            }
            if (value == '\\') {
                const Status startStatus = beginToken(active);
                if (startStatus != Status::Ok) return startStatus;
                escaped = true;
                continue;
            }
            if (quoted) {
                if (value == '"') {
                    quoted = false;
                } else {
                    const Status appendStatus = appendTokenCharacter(value, active);
                    if (appendStatus != Status::Ok) return appendStatus;
                }
                continue;
            }
            if (value == '"') {
                const Status startStatus = beginToken(active);
                if (startStatus != Status::Ok) return startStatus;
                quoted = true;
            } else if (isWhitespace(value)) {
                finishToken(active);
            } else {
                const Status appendStatus = appendTokenCharacter(value, active);
                if (appendStatus != Status::Ok) return appendStatus;
            }
        }

        if (escaped) return Status::InvalidEscape;
        if (quoted) return Status::UnclosedQuote;
        finishToken(active);
        return Status::Ok;
    }
};

}  // namespace tinycli

#endif
