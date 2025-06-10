#include "syncexecutor.h"
#include <stdexcept>
#include <string>
#include <string_view>

namespace wibens::resp
{
std::string SyncExecutor::execute(std::string_view command, std::chrono::milliseconds timeout)
{
    connection->send(command);
    if (connection->receive(timeout)) {
        return connection->popResponse();
    }
    throw error::TimeoutError("Timeout while waiting for response");
}
} // namespace wibens::resp
