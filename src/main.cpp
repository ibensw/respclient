#include "connection.h"
#include "resp.h"
#include <fmt/format.h>
#include <fmt/std.h>

int main() // NOLINT
{
    RedisTcpConnection connection({"localhost", 6379});
    SyncExecutor executor(&connection, std::chrono::milliseconds(1000));
    Commands::Get getCmd("mykey");
    fmt::print("Command: {}\nResult: {}\n", getCmd.getCommand(), executor(getCmd));
    Commands::Set setCmd("mykey", "test");
    fmt::print("Command: {}\nResult: {}\n", setCmd.getCommand(), executor(setCmd));
    Commands::Del delCmd("key1", "key2");
    fmt::print("Command: {}\nResult: {}\n", delCmd.getCommand(), executor(delCmd));
}
