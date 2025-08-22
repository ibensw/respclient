#include "connection.h"

#include "ast.h"
#include "command.h"
#include "syncexecutor.h"
#include "types.h"
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

struct HelloCommand : public CommandBase {
    using T = Types<>;
    using ResultType =
        T::Map<T::String,
               T::Variant<T::String, T::Integer, T::Array<T::Map<T::String, T::Variant<T::String, T::Integer>>>>>;
    // using ResultType = Ignore;
    explicit HelloCommand(std::string_view version) : CommandBase("HELLO", version)
    {
    }
};

// static std::string_view parse(std::string_view buffer)
// {
//     if (buffer.empty()) {
//         return {};
//     }
//     switch (buffer[0]) {
//         case '+':
//         case '-':
//         case ':':
//         case '_':
//         case '#':
//         case ',':
//         case '(': {
//             auto end = buffer.find("\r\n");
//             if (end == std::string_view::npos) {
//                 return {};
//             }
//             return buffer.substr(0, end + 2);
//         }
//         case '!':
//         case '=':
//         case '$': {
//             auto endSize = buffer.find("\r\n");
//             if (endSize == std::string_view::npos) {
//                 return {};
//             }
//             auto size = parser::readnum<size_t>(buffer.substr(1, endSize));
//             if (auto innerData = buffer.substr(endSize + 2); innerData.size() < size + 2) {
//                 return {};
//             }
//             return buffer.substr(0, endSize + 2 + size + 2);
//         }
//         case '*':
//         case '%':
//         case '>': {
//             bool isMap = buffer[0] == '%';
//             auto endCount = buffer.find("\r\n");
//             if (endCount == std::string_view::npos) {
//                 return {};
//             }
//             auto count = parser::readnum<size_t>(buffer.substr(1, endCount));
//             if (isMap) {
//                 count *= 2;
//             }
//             auto start = buffer.substr(endCount + 2);
//             std::size_t totalSize = 0;
//             for (size_t i = 0; i < count; ++i) {
//                 std::string_view v = parse(start);
//                 if (v.empty()) {
//                     return {};
//                 }
//                 start.remove_prefix(v.size());
//                 totalSize += v.size();
//             }
//             return buffer.substr(0, endCount + 2 + totalSize);
//         }
//         default:
//             throw error::ParseError("Unexpected prefix in response: "s + std::string{buffer[0]});
//     }
// }

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
        CommandBase subscribeCommand("SUBSCRIBE", it->first);
        send(subscribeCommand.getCommand());
    }
}

void RedisConnection::unsubscribe(const std::string &channel)
{
    CommandBase unsubscribeCommand("UNSUBSCRIBE", channel);
    send(unsubscribeCommand.getCommand());
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
