#include "syncexecutor.h"
#include <stdexcept>
#include <string>
#include <string_view>

std::string SyncExecutor::execute(std::string_view command, std::chrono::milliseconds timeout)
{
    connection->send(command);
    if (connection->receive(timeout)) {
        return connection->popResponse();
    }
    throw std::runtime_error("Timeout while waiting for response");
}
