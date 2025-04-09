#include "connection.h"

#include "command.h"
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

using namespace std::string_literals;
using namespace std::chrono_literals;

struct HelloCommand : public CommandBase {
    using ResultType = std::unordered_map<std::string, std::variant<std::string, int64_t, std::vector<std::string>>>;
    HelloCommand(int version) : CommandBase("HELLO {}", version)
    {
    }
};

static std::string_view parse(std::string_view buffer)
{
    if (buffer.empty()) {
        return {};
    }
    switch (buffer[0]) {
        case '+':
        case '-':
        case ':':
        case '_':
        case '#':
        case ',':
        case '(': {
            auto end = buffer.find("\r\n");
            if (end == std::string_view::npos) {
                return {};
            }
            return buffer.substr(0, end + 2);
        }
        case '!':
        case '=':
        case '$': {
            auto endSize = buffer.find("\r\n");
            if (endSize == std::string_view::npos) {
                return {};
            }
            auto size = readnum<size_t>(buffer.substr(1, endSize));
            std::string_view innerData = buffer.substr(endSize + 2);
            if (innerData.size() < size + 2) {
                return {};
            }
            return buffer.substr(0, endSize + 2 + size + 2);
        }
        case '*':
        case '%': {
            bool isMap = buffer[0] == '%';
            auto endCount = buffer.find("\r\n");
            if (endCount == std::string_view::npos) {
                return {};
            }
            auto count = readnum<size_t>(buffer.substr(1, endCount));
            if (isMap) {
                count *= 2;
            }
            auto start = buffer.substr(endCount + 2);
            std::size_t totalSize = 0;
            for (size_t i = 0; i < count; ++i) {
                std::string_view v = parse(start);
                if (v.empty()) {
                    return {};
                }
                start.remove_prefix(v.size());
                totalSize += v.size();
            }
            return buffer.substr(0, endCount + 2 + totalSize);
        }
        default:
            throw std::runtime_error("Unexpected prefix in response: "s + std::string{buffer[0]});
    }
}

bool RedisConnection::parseResponses()
{
    std::string_view buffer = receiveBuffer;
    std::string_view response;
    while (true) {
        response = parse(buffer);
        if (response.empty()) {
            break;
        }
        responses.emplace_back(response);
        buffer.remove_prefix(response.size());
    }
    receiveBuffer = buffer;

    return !responses.empty();
}

RedisTcpConnection::RedisTcpConnection(TcpConnectionParams params)
    : fd(socket(AF_INET, SOCK_STREAM, 0)), connParams(std::move(params))
{
    if (fd < 0) {
        throw std::runtime_error("Failed to create socket"s + strerror(errno));
    }

    addrinfo hints{};
    addrinfo *res = nullptr;
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(connParams.port);
    int err = getaddrinfo(connParams.host.c_str(), portStr.c_str(), &hints, &res);
    if (err != 0) {
        close(fd);
        throw std::runtime_error("Failed to get address info: "s + gai_strerror(err));
    }

    // Connect to the first valid address returned by getaddrinfo
    addrinfo *p = res;
    while (p != nullptr && connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
        p = p->ai_next;
    }

    freeaddrinfo(res);
    if (p == nullptr) {
        close(fd);
        throw std::runtime_error("No valid addresses found"s);
    }

    // Make the socket non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    SyncExecutor executor(this, 100ms);
    executor(HelloCommand(connParams.version));
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
        throw std::runtime_error("Poll failed"s + strerror(errno));
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
