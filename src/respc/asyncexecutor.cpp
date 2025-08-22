#include "asyncexecutor.h"
#include <stdexcept>
#include <string>
#include <string_view>

namespace wibens::resp
{

std::promise<ast::Node::Ptr> &ASyncExecutor::execute(std::string_view command)
{
    connection->send(command);
    return promises.emplace_back();
}

bool ASyncExecutor::listen(std::chrono::milliseconds timeout)
{
    if (promises.empty()) {
        return false;
    }
    connection->receive(timeout);
    while (connection->getResponseCount()) {
        auto &promise = promises.front();
        promise.set_value(connection->popResponse());
        promises.pop_front();
    }
    return !promises.empty();
}
} // namespace wibens::resp
