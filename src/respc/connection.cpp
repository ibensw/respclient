#include "connection.h"
#include "ast.h"
#include "syncexecutor.h"
#include <arpa/inet.h>
#include <array>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace wibens::resp
{

using namespace std::string_literals;
using namespace std::chrono_literals;
using ast::Node;

struct HelloCommand : public Node {
    using ResultType = std::unordered_map<
        std::string, std::variant<std::string, int64_t,
                                  std::vector<std::unordered_map<std::string, std::variant<std::string, int64_t>>>>>;
    // using ResultType = Ignore;
    explicit HelloCommand(std::string_view version) : Node("HELLO", version)
    {
    }
};

bool RedisConnection::parseResponses()
{
    while (true) {
        ast::Node::Ptr node = nullptr;
        std::tie(node, receiveBuffer) = ast::Node::parse(receiveBuffer);
        if (node == nullptr) {
            break;
        }
        if (node->type() == '>') {
            pushMsg(std::move(node));
        } else {
            responses.push_back(std::move(node));
        }
    }

    return !responses.empty();
}

void RedisConnection::subscribe(std::string channel, Callback callback)
{
    auto [it, inserted] = subscriptions.emplace(std::move(channel), std::move(callback));
    if (inserted) {
        Node subscribeCommand("SUBSCRIBE", it->first);
        send(subscribeCommand.toString());
    }
}

void RedisConnection::unsubscribe(const std::string &channel)
{
    Node unsubscribeCommand("UNSUBSCRIBE", channel);
    send(unsubscribeCommand.toString());
    subscriptions.erase(channel);
}

void RedisConnection::pushMsg(ast::Node::Ptr msg)
{
    auto parsed = parser::Parser<std::vector<std::variant<std::string, int64_t>>>::parse(msg.get());
    if (parsed.empty()) {
        return;
    }
    if (std::get<std::string>(parsed[0]) == "message") {
        if (auto it = subscriptions.find(std::get<std::string>(parsed[1])); it != subscriptions.end()) {
            it->second(it->first, std::get<std::string>(parsed[2]));
        }
    }
}

RedisTcpConnection::RedisTcpConnection(TcpConnectionParams params)
    : fd(socket(AF_INET, SOCK_STREAM, 0)), connParams(std::move(params))
{
    if (fd < 0) {
        throw error::ConnectionError("Failed to create socket"s + strerror(errno));
    }

    addrinfo hints{};
    addrinfo *res = nullptr;
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(connParams.port);
    if (int err = getaddrinfo(connParams.host.c_str(), portStr.c_str(), &hints, &res); err != 0) {
        close(fd);
        throw error::ConnectionError("Failed to get address info: "s + gai_strerror(err));
    }

    // Connect to the first valid address returned by getaddrinfo
    addrinfo *p = res;
    while (p != nullptr && connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
        p = p->ai_next;
    }

    freeaddrinfo(res);
    if (p == nullptr) {
        close(fd);
        throw error::ConnectionError("No valid addresses found"s);
    }

    // Make the socket non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    SyncExecutor executor(this, 100ms);
    executor(HelloCommand(std::to_string(connParams.version)));
}

RedisTcpConnection::~RedisTcpConnection()
{
    // Can be moved from
    if (fd >= 0) {
        close(fd);
    }
}

void RedisTcpConnection::send(std::string_view data)
{
    printf("Sending: %.*s\n", static_cast<int>(data.size()), data.data());
    std::size_t sent = 0;
    while (sent < data.size()) {
        sent += ::send(fd, data.data() + sent, data.size() - sent, 0);
    }
}

bool RedisTcpConnection::receive(std::chrono::milliseconds timeout)
{
    pollfd fdSet{};
    fdSet.fd = fd;
    fdSet.events = POLLIN;

    int pollResult = poll(&fdSet, 1, static_cast<int>(timeout.count()));

    if (pollResult < 0) {
        throw error::ConnectionError("Poll failed: "s + strerror(errno));
    }
    if (pollResult == 0) {
        return false; // Timeout
    }
    if (pollResult > 0 && (fdSet.revents & POLLIN)) {
        ssize_t received = 0;
        while (received >= 0) {
            std::array<char, 1024> buffer{};
            received = ::recv(fd, buffer.data(), buffer.size(), 0);
            if (received > 0) {
                appendBuffer({buffer.data(), static_cast<std::size_t>(received)});
            }
        }
    }

    return parseResponses();
}
} // namespace wibens::resp
