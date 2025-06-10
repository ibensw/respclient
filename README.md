# C++ RESP Client

RESP is the [Redis serialization protocol specification](https://redis.io/docs/latest/develop/reference/protocol-spec/), the protocol to communicate with a redis server or similar.

The goal of this project is to provide a very customizable way to use redis in C++ projects, leveraging on modern C++ principles.

## Basic usage

```
RedisTcpConnection connection({"localhost", 6379});

SyncExecutor executor(&connection, std::chrono::milliseconds(100));

Commands::Get getCmd("mykey");
fmt::print("Command: {}Result: {}\n", getCmd.getCommand(), executor(getCmd));
Commands::Set setCmd("mykey", "test");
fmt::print("Command: {}Result: {}\n", setCmd.getCommand(), executor(setCmd));
fmt::print("Command: {}Result: {}\n", getCmd.getCommand(), executor(getCmd));
Commands::Del delCmd("mykey");
fmt::print("Command: {}Result: {}\n", delCmd.getCommand(), executor(delCmd));
```

## Requirements
Required [libfmt](https://github.com/fmtlib/fmt)

## What works
* Sync and async connections
* Commands from the generic category

## Todo
* Build as library
* Verify the install path of the header files
* Convert main.cpp to proper unit tests
* Add custom other command categories
* Add other connection types (unix domain socket) and connection wrappers (connection pool, auto reconnect, ...)
* Investigate subscribers
