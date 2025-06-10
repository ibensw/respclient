#include <fmt/format.h>
#include <fmt/std.h>
#include <respc/connection.h>
#include <respc/respc.h>

using namespace wibens::resp;

int main() // NOLINT
{
    RedisTcpConnection connection({"localhost", 6379});

    Commands::Get getCmd("mykey");
    Commands::Set setCmd("mykey", "test");
    Commands::Del delCmd("mykey");

    fmt::print("Sync...\n");
    {
        SyncExecutor executor(&connection, std::chrono::milliseconds(1000));
        fmt::print("Command: {}Result: {}\n", getCmd.getCommand(), executor(getCmd));
        fmt::print("Command: {}Result: {}\n", setCmd.getCommand(), executor(setCmd));
        fmt::print("Command: {}Result: {}\n", getCmd.getCommand(), executor(getCmd));
        fmt::print("Command: {}Result: {}\n", delCmd.getCommand(), executor(delCmd));
    }
    fmt::print("Async...\n");
    {
        ASyncExecutor executor(&connection);

        auto get1 = executor(getCmd);
        auto set1 = executor(setCmd);
        auto get2 = executor(getCmd);
        auto del1 = executor(delCmd);

        while (executor.listen(std::chrono::milliseconds(1000))) {
        }

        fmt::print("Command: {}Result: {}\n", getCmd.getCommand(), get1.get());
        fmt::print("Command: {}Result: {}\n", setCmd.getCommand(), set1.get());
        fmt::print("Command: {}Result: {}\n", getCmd.getCommand(), get2.get());
        fmt::print("Command: {}Result: {}\n", delCmd.getCommand(), del1.get());
    }
}
