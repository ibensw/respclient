#include "syncexecutor.h"
#include <stdexcept>
#include <string>
#include <string_view>

namespace wibens::resp
{
ast::Node::Ptr SyncExecutor::execute(std::string_view command, std::chrono::milliseconds timeout)
{
    connection->send(command);
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (timeout.count() > 0) {
        if (connection->receive(timeout)) {
            return connection->popResponse();
        }
        timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
    }
    throw error::TimeoutError("Timeout while waiting for response");
}
} // namespace wibens::resp
