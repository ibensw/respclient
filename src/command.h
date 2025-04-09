#pragma once

#include <fmt/format.h>
#include <string_view>

struct CommandBase {
    template <typename... Args> CommandBase(fmt::format_string<Args...> command, Args... args)
    {
        fmt::format_to(std::back_inserter(buffer), command, std::forward<Args>(args)...);
        buffer.append(std::string_view{"\r\n"});
    }
    std::string_view getCommand() const
    {
        return {buffer.data(), buffer.size()};
    }

  private:
    fmt::basic_memory_buffer<char, 32> buffer;
};
