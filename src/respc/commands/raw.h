#pragma once

#include "../command.h"
#include "../parser.h"
#include "../util.h"
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace wibens::resp::commands::raw
{
// struct ExpireBase : CommandBase {
//     struct ExpireFlags {
//         bool nx : 1 = false;
//         bool xx : 1 = false;
//         bool gt : 1 = false;
//         bool lt : 1 = false;
//     };
//     using ResultType = int64_t;

//     ExpireBase(std::string_view cmd, std::string_view key, int64_t timeout, ExpireFlags flags = {})
//     {
//         ast::Node node(std::make_shared<ast::StringNode>(cmd));
//     }
// };

struct Get : CommandBase {
    using ResultType = std::optional<std::string>;
    explicit Get(std::string_view key) : CommandBase("GET", key)
    {
    }
};

struct SetFlags {
    bool xx = false;
    bool nx = false;
    bool get = false;
};
struct Set : CommandBase {
    using ResultType = std::optional<std::string>;
    Set(std::string_view key, std::string_view value, SetFlags flags = SetFlags{})
    {
        ast::Node node("SET", key, value);
        if (flags.xx)
            node.addChild("XX");
        if (flags.nx)
            node.addChild("NX");
        if (flags.get)
            node.addChild("GET");
        node.rebuild();
        build(node);
    }
};

struct Del : CommandBase {
    using ResultType = int64_t;
    explicit Del(std::initializer_list<std::string_view> keys)
    {
        ast::Node node("DEL", keys);
        for (const auto &key : keys) {
            node.addChild(key);
        };
        node.rebuild();
        build(node);
    }
    template <typename... Args> explicit Del(Args... args) : CommandBase("DEL", args...)
    {
    }
};

struct CopyFlags {
    std::optional<std::string> database{};
    bool replace = false;
};
struct Copy : CommandBase {
    using ResultType = int64_t;
    Copy(std::string_view source, std::string_view destination, CopyFlags flags = {})
    {
        ast::Node node("COPY", source, destination);
        if (flags.database.has_value()) {
            node.addChild("DB");
            node.addChild(flags.database.value());
        }
        if (flags.replace) {
            node.addChild("REPLACE");
        }
    }
};

struct Dump : CommandBase {
    using ResultType = std::optional<std::string>;
    explicit Dump(std::string_view key) : CommandBase("DUMP", key)
    {
    }
};

struct Exists : CommandBase {
    using ResultType = int64_t;
    explicit Exists(std::initializer_list<std::string_view> keys)
    {
        ast::Node node("EXISTS", keys);
        for (const auto &key : keys) {
            node.addChild(key);
        };
        node.rebuild();
        build(node);
    }
    template <typename... Args> explicit Exists(Args... args) : CommandBase("EXISTS", args...)
    {
    }
};

// struct Expire : ExpireBase {
//     Expire(std::string_view key, int64_t timeout, ExpireBase::ExpireFlags flags)
//         : ExpireBase("EXPIRE", key, timeout, flags)
//     {
//     }
// };

// struct ExpireAt : ExpireBase {
//     ExpireAt(std::string_view key, int64_t timepoint, ExpireBase::ExpireFlags flags)
//         : ExpireBase("EXPIREAT", key, timepoint, flags)
//     {
//     }
// };

// struct ExpireTime : CommandBase {
//     using ResultType = int64_t;
//     explicit ExpireTime(std::string_view key) : CommandBase("EXPIRETIME {}", key)
//     {
//     }
// };

// template <typename Result = std::vector<std::string>> struct Keys : CommandBase {
//     using ResultType = Result;
//     explicit Keys(std::string_view pattern) : CommandBase("KEYS {}", pattern)
//     {
//     }
// };

// Skip migrate

// struct Move : CommandBase {
//     using ResultType = int64_t;
//     Move(std::string_view key, std::string_view db) : CommandBase("MOVE {} {}", key, db)
//     {
//     }
// };

struct Multi : CommandBase {
    using ResultType = parser::IgnoreAll;
    Multi() : CommandBase("MULTI"){};
};

struct Discard : CommandBase {
    using ResultType = parser::IgnoreAll;
    Discard() : CommandBase("DISCARD"){};
};

struct Exec : CommandBase {
    using ResultType = parser::IgnoreAll;
    Exec() : CommandBase("EXEC"){};
};

struct Publish : CommandBase {
    using ResultType = int64_t;
    Publish(std::string_view channel, std::string_view message) : CommandBase("PUBLISH", channel, message){};
};
} // namespace wibens::resp::commands::raw
