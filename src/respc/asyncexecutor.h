#pragma once

#include "connection.h"
#include "parser.h"
#include <chrono>
#include <deque>
#include <future>
#include <optional>

namespace wibens::resp
{
template <typename ResultType> class ResultFuture : public std::future<std::string>
{
  public:
    explicit ResultFuture(std::future<std::string> &&promise) : std::future<std::string>(std::move(promise))
    {
    }

    ResultType get()
    {
        std::string value = std::future<std::string>::get();
        std::string_view view = value;
        return parser::Parser<ResultType>::parse(view);
    }

    using std::future<std::string>::valid;
    using std::future<std::string>::wait;
    using std::future<std::string>::wait_for;
    using std::future<std::string>::wait_until;
};

class ASyncExecutor
{
  public:
    explicit ASyncExecutor(RedisConnection *conn) : connection(conn)
    {
    }

    ~ASyncExecutor()
    {
        // Wait for all promises to be fulfilled before releasing the connection
        while (listen(std::chrono::milliseconds(-1))) {
        }
    }

    template <typename T> ResultFuture<typename T::ResultType> operator()(const T &command)
    {
        auto &result = execute(command.getCommand());
        return ResultFuture<typename T::ResultType>(result.get_future());
    }

    bool listen(std::chrono::milliseconds timeout);

  private:
    std::promise<std::string> &execute(std::string_view command);
    std::deque<std::promise<std::string>> promises;

    RedisConnection *connection;
};
} // namespace wibens::resp
