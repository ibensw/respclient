#pragma once

#include "../ast.h"
#include "../parser.h"
#include "../util.h"
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace wibens::resp::commands::raw
{
struct ExpireFlags {
    bool nx = false;
    bool xx = false;
    bool gt = false;
    bool lt = false;
};
struct ExpireBase : ast::Node {
    using ResultType = int64_t;

    ExpireBase(std::string_view cmd, std::string_view key, int64_t timeout, ExpireFlags flags = {})
        : ast::Node(cmd, key, timeout)
    {
        if (flags.nx)
            addChild("NX");
        if (flags.xx)
            addChild("XX");
        if (flags.gt)
            addChild("GT");
        if (flags.lt)
            addChild("LT");
        rebuild();
    }
};

struct Get : ast::Node {
    using ResultType = std::optional<std::string>;
    explicit Get(std::string_view key) : ast::Node("GET", key)
    {
    }
};

struct SetFlags {
    bool xx = false;
    bool nx = false;
    bool get = false;
};
struct Set : ast::Node {
    using ResultType = std::optional<std::string>;
    Set(std::string_view key, std::string_view value, SetFlags flags = {}) : ast::Node("SET", key, value)
    {
        if (flags.xx)
            addChild("XX");
        if (flags.nx)
            addChild("NX");
        if (flags.get)
            addChild("GET");
        rebuild();
    }
};

struct Del : ast::Node {
    using ResultType = int64_t;
    explicit Del(std::initializer_list<std::string_view> keys) : ast::Node("DEL", keys)
    {
        for (const auto &key : keys) {
            addChild(key);
        };
        rebuild();
    }
    template <typename... Args> explicit Del(Args... args) : ast::Node("DEL", args...)
    {
    }
};

struct CopyFlags {
    std::optional<std::string> database{};
    bool replace = false;
};
struct Copy : ast::Node {
    using ResultType = int64_t;
    Copy(std::string_view source, std::string_view destination, CopyFlags flags = {})
        : ast::Node("COPY", source, destination)
    {
        if (flags.database.has_value()) {
            addChild("DB");
            addChild(flags.database.value());
        }
        if (flags.replace) {
            addChild("REPLACE");
        }
        rebuild();
    }
};

struct Dump : ast::Node {
    using ResultType = std::optional<std::string>;
    explicit Dump(std::string_view key) : ast::Node("DUMP", key)
    {
    }
};

struct Exists : ast::Node {
    using ResultType = int64_t;
    explicit Exists(std::initializer_list<std::string_view> keys) : ast::Node("EXISTS", keys)
    {
        for (const auto &key : keys) {
            addChild(key);
        };
        rebuild();
    }
    template <typename... Args> explicit Exists(Args... args) : ast::Node("EXISTS", args...)
    {
    }
};

struct Expire : ExpireBase {
    Expire(std::string_view key, int64_t timeout, ExpireFlags flags) : ExpireBase("EXPIRE", key, timeout, flags)
    {
    }
};

struct ExpireAt : ExpireBase {
    ExpireAt(std::string_view key, int64_t timepoint, ExpireFlags flags) : ExpireBase("EXPIREAT", key, timepoint, flags)
    {
    }
};

struct ExpireTime : ast::Node {
    using ResultType = int64_t;
    explicit ExpireTime(std::string_view key) : ast::Node("EXPIRETIME", key)
    {
    }
};

template <typename Result = std::vector<std::string>> struct Keys : ast::Node {
    using ResultType = Result;
    explicit Keys(std::string_view pattern) : ast::Node("KEYS", pattern)
    {
    }
};

struct MigrateOptions {
    bool copy = false;
    bool replace = false;
    std::optional<std::string> password;
    std::optional<std::string> username;
};
struct Migrate : ast::Node {
    using ResultType = std::string;
    Migrate(std::string_view host, int port, int db, int64_t timeout, std::span<std::string_view> keys,
            MigrateOptions options = {})
        : ast::Node("MIGRATE", host, port, "", db, timeout)
    {
        if (options.copy) {
            addChild("COPY");
        }
        if (options.replace) {
            addChild("REPLACE");
        }
        if (options.username.has_value()) {
            addChild("AUTH2");
            addChild(options.username.value());
            addChild(options.password.value());
        } else if (options.password.has_value()) {
            addChild("AUTH");
            addChild(options.password.value());
        }
        addChild("KEYS");
        for (const auto &key : keys) {
            addChild(key);
        }
        rebuild();
    }
};

struct Move : ast::Node {
    using ResultType = int64_t;
    Move(std::string_view key, std::string_view db) : ast::Node("MOVE", key, db)
    {
    }
};

struct Multi : ast::Node {
    using ResultType = parser::IgnoreAll;
    Multi() : ast::Node("MULTI"){};
};

struct Discard : ast::Node {
    using ResultType = parser::IgnoreAll;
    Discard() : ast::Node("DISCARD"){};
};

struct Exec : ast::Node {
    using ResultType = parser::IgnoreAll;
    Exec() : ast::Node("EXEC"){};
};

struct Publish : ast::Node {
    using ResultType = int64_t;
    Publish(std::string_view channel, std::string_view message) : ast::Node("PUBLISH", channel, message){};
};
} // namespace wibens::resp::commands::raw
