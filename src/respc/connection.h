#pragma once

#include "error.h"
#include "types.h"
#include <chrono>
#include <deque>
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
    virtual ~RedisConnection() = default;

    virtual void send(std::string_view data) = 0;
    virtual bool receive(std::chrono::milliseconds timeout) = 0;

    virtual std::size_t getResponseCount() const
    {
        return responses.size();
    }
    virtual std::string popResponse()
    {
        std::string result = std::move(responses.front());
        responses.pop_front();
        return result;
    }

  protected:
    void appendBuffer(std::string_view data)
    {
        receiveBuffer.append(data);
    }
    bool parseResponses();

  private:
    std::deque<std::string> responses;
    std::string receiveBuffer;
};

class RedisTcpConnection : public RedisConnection
{
  public:
    explicit RedisTcpConnection(TcpConnectionParams params);
    ~RedisTcpConnection() override;

    RedisTcpConnection(const RedisTcpConnection &) = delete;
    RedisTcpConnection &operator=(const RedisTcpConnection &) = delete;
    RedisTcpConnection(RedisTcpConnection &&) noexcept;
    RedisTcpConnection &operator=(RedisTcpConnection &&) noexcept;

    void send(std::string_view data) override;
    bool receive(std::chrono::milliseconds timeout) override;

  private:
    int fd{-1};
    TcpConnectionParams connParams;
};
} // namespace wibens::resp
