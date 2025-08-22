#pragma once

#include "ast.h"
#include "connection.h"
#include "parser.h"
#include <chrono>
#include <deque>
#include <future>
#include <optional>

namespace wibens::resp
{
template <typename ResultType> class ResultFuture : public std::future<ast::Node::Ptr>
{
  public:
    explicit ResultFuture(std::future<ast::Node::Ptr> &&promise) : std::future<ast::Node::Ptr>(std::move(promise))
    {
    }

    ResultType get()
    {
        auto value = std::future<ast::Node::Ptr>::get();
        return parser::Parser<ResultType>::parse(value.get());
    }

    using std::future<ast::Node::Ptr>::valid;
    using std::future<ast::Node::Ptr>::wait;
    using std::future<ast::Node::Ptr>::wait_for;
    using std::future<ast::Node::Ptr>::wait_until;
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
    std::promise<ast::Node::Ptr> &execute(std::string_view command);
    std::deque<std::promise<ast::Node::Ptr>> promises;

    RedisConnection *connection;
};
} // namespace wibens::resp
