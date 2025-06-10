#pragma once

#include "connection.h"
#include "parser.h"
#include <chrono>
#include <optional>

namespace wibens::resp
{
class SyncExecutor
{
  public:
    SyncExecutor(RedisConnection *conn, std::chrono::milliseconds timeout) : connection(conn), defaultTimeout(timeout)
    {
    }

    template <typename T>
    typename T::ResultType operator()(const T &command, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
    {
        auto result = execute(command.getCommand(), timeout.value_or(defaultTimeout));
        std::string_view view = result;
        return parser::Parser<typename T::ResultType>::parse(view);
    }

  private:
    std::string execute(std::string_view command, std::chrono::milliseconds timeout);

    RedisConnection *connection;
    std::chrono::milliseconds defaultTimeout;
};
} // namespace wibens::resp
