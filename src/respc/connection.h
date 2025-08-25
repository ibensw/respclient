#pragma once

#include "ast.h"
#include "error.h"
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace wibens::resp
{

namespace error
{
struct ConnectionError : std::runtime_error {
    using std::runtime_error::runtime_error;
};
}; // namespace error

struct TcpConnectionParams {
    std::string host;
    int port;
    int version = 3;
    bool fallback = false;
};

class RedisConnection
{
  public:
    RedisConnection() = default;
    virtual ~RedisConnection() = default;
    RedisConnection(const RedisConnection &) = delete;
    RedisConnection &operator=(const RedisConnection &) = delete;
    RedisConnection(RedisConnection &&) noexcept = default;
    RedisConnection &operator=(RedisConnection &&) noexcept = default;

    using Callback = std::function<void(std::string_view, std::string_view)>;

    virtual void send(std::string_view data) = 0;
    virtual bool receive(std::chrono::milliseconds timeout) = 0;

    virtual std::size_t getResponseCount() const
    {
        return responses.size();
    }
    virtual ast::Node::Ptr popResponse()
    {
        auto front = std::move(responses.front());
        responses.pop_front();
        return front;
    }

    void subscribe(std::string channel, Callback callback);
    void unsubscribe(const std::string &channel);

  protected:
    void appendBuffer(std::string_view data)
    {
        printf("Received data: %.*s\n", static_cast<int>(data.size()), data.data());
        receiveBuffer.append(data);
    }
    bool parseResponses();

  private:
    void pushMsg(ast::Node::Ptr msg);

    std::deque<ast::Node::Ptr> responses;
    std::string receiveBuffer;
    std::unordered_map<std::string, Callback> subscriptions;
};

template <typename T> class RedisConnectionCreator
{
  public:
    template <typename... Args> static std::unique_ptr<T> create(Args &&...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
};

class RedisTcpConnection : public RedisConnection, public RedisConnectionCreator<RedisTcpConnection>
{
  public:
    explicit RedisTcpConnection(TcpConnectionParams params);
    ~RedisTcpConnection() override;

    void send(std::string_view data) override;
    bool receive(std::chrono::milliseconds timeout) override;

  private:
    int fd{-1};
    TcpConnectionParams connParams;
};
} // namespace wibens::resp
