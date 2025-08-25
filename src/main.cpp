#include <format>
#include <iostream>
#include <optional>
#include <respc/ast.h>
#include <respc/asyncexecutor.h>
#include <respc/commands/raw.h>
#include <respc/connection.h>
#include <respc/syncexecutor.h>

using namespace wibens::resp;
using namespace wibens::resp::commands::raw;

template <typename... ARGS> void print(std::format_string<ARGS...> fmt, ARGS &&...args) // NOLINT
{
    std::cout << std::format(fmt, std::forward<ARGS>(args)...);
}

template <typename T> struct std::formatter<std::optional<T>> : std::formatter<T> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return std::formatter<T>::parse(ctx);
    }

    template <typename FormatContext> auto format(const std::optional<T> &opt, FormatContext &ctx) const
    {
        if (!opt) {
            return std::format_to(ctx.out(), "<None>");
        }
        return std::formatter<T>::format(*opt, ctx);
    }
};

template <> struct std::formatter<ast::Node> : std::formatter<std::string> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return std::formatter<std::string>::parse(ctx);
    }

    template <typename FormatContext> auto format(const ast::Node &node, FormatContext &ctx) const
    {
        std::string command;
        for (const auto &part : node.children) {
            command += "\"" + part->toString() + "\" ";
        }
        return std::formatter<std::string>::format(command, ctx);
    }
};

int main() // NOLINT
{
    auto connection = RedisTcpConnection::create(TcpConnectionParams{"localhost", 6379});

    wibens::resp::commands::raw::Get getCmd("mykey");
    Set setCmd("mykey", "test");
    Del delCmd("mykey");

    print("Sync...\n");
    {
        SyncExecutor executor(connection.get(), std::chrono::milliseconds(1000));
        print("Command: {}Result: {}\n", getCmd.toString(), executor(getCmd));
        print("Command: {}Result: {}\n", setCmd.toString(), executor(setCmd));
        print("Command: {}Result: {}\n", getCmd.toString(), executor(getCmd));
        print("Command: {}Result: {}\n", delCmd.toString(), executor(delCmd));
    }
    print("Async...\n");
    {
        ASyncExecutor executor(connection.get());

        auto get1 = executor(getCmd);
        auto set1 = executor(setCmd);
        auto get2 = executor(getCmd);
        auto del1 = executor(delCmd);

        while (executor.listen(std::chrono::milliseconds(1000))) {
            // do nothing
        }

        print("Command: {}Result: {}\n", getCmd.toString(), get1.get());
        print("Command: {}Result: {}\n", setCmd.toString(), set1.get());
        print("Command: {}Result: {}\n", getCmd.toString(), get2.get());
        print("Command: {}Result: {}\n", delCmd.toString(), del1.get());
    }
    print("Subscriptions...\n");
    {
        connection->subscribe("pub sub", [](std::string_view channel, std::string_view message) {
            print("Got message on channel {}: {}\n", channel, message);
        });

        for (auto i : {1, 2, 3}) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            SyncExecutor executor(connection.get(), std::chrono::milliseconds(1000));
            executor(Publish("pub sub", std::format("test message {}", i)));
        }
    }
}
