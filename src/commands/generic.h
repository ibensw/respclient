#pragma once

#include "command.h"
#include "types.h"

namespace wibens::resp
{

template <typename T = Types<>> struct Generic {
    struct ExpireBase : CommandBase {
        struct ExpireFlags {
            bool nx = false;
            bool xx = false;
            bool gt = false;
            bool lt = false;
        };
        using ResultType = typename T::Integer;

        ExpireBase(std::string_view cmd, std::string_view key, typename T::Integer timeout, ExpireFlags flags)
            : CommandBase("{} {} {}{}{}{}{}", cmd, key, timeout, flags.nx ? " NX" : "", flags.xx ? " XX" : "",
                          flags.gt ? " GT" : "", flags.lt ? " LT" : "")
        {
        }
    };

    struct Get : CommandBase {
        using ResultType = typename T::template Variant<typename T::Null, typename T::String>;
        explicit Get(std::string_view key) : CommandBase("GET {}", key)
        {
        }
    };

    struct Set : CommandBase {
        using ResultType = typename T::template Variant<typename T::Null, typename T::String>;
        Set(std::string_view key, typename T::String value, bool xx = false, bool nx = false, bool get = false)
            : CommandBase("SET {} {}{}{}{}", key, value, xx ? " XX" : "", nx ? " NX" : "", get ? " GET" : "")
        {
        }
    };

    struct Del : CommandBase {
        using ResultType = typename T::Integer;
        explicit Del(std::initializer_list<std::string_view> keys) : CommandBase("DEL {}", fmt::join(keys, " "))
        {
        }
        template <typename... Args> explicit Del(Args... args) : Del(std::initializer_list<std::string_view>{args...})
        {
        }
    };

    struct Copy : CommandBase {
        using ResultType = typename T::Integer;
        Copy(std::string_view source, std::string_view destination,
             std::optional<std::string_view> database = std::nullopt, bool replace = false)
            : CommandBase("COPY {} {}{}{}", source, destination, database ? fmt::format("DB {}", *database) : "",
                          replace ? " REPLACE" : "")
        {
        }
    };

    struct Dump : CommandBase {
        using ResultType = typename T::template Variant<typename T::Null, typename T::String>;
        explicit Dump(std::string_view key) : CommandBase("DUMP {}", key)
        {
        }
    };

    struct Exists : CommandBase {
        using ResultType = typename T::Integer;
        explicit Exists(std::initializer_list<std::string_view> keys) : CommandBase("EXISTS {}", fmt::join(keys, " "))
        {
        }
        template <typename... Args>
        explicit Exists(Args... args) : Exists(std::initializer_list<std::string_view>{args...})
        {
        }
    };

    struct Expire : ExpireBase {
        Expire(std::string_view key, std::chrono::seconds timeout, typename ExpireBase::ExpireFlags flags)
            : ExpireBase("EXPIRE", key, timeout.count(), flags)
        {
        }
    };

    struct ExpireAt : ExpireBase {
        ExpireAt(std::string_view key, std::chrono::system_clock::time_point timepoint,
                 typename ExpireBase::ExpireFlags flags)
            : ExpireBase("EXPIREAT", key, timepoint.time_since_epoch().count(), flags)
        {
        }
    };

    struct ExpireTime : CommandBase {
        using ResultType = std::chrono::system_clock::time_point;
        explicit ExpireTime(std::string_view key) : CommandBase("EXPIRETIME {}", key)
        {
        }
    };

    struct Keys : CommandBase {
        using ResultType = typename T::template Array<T::String>;
        explicit Keys(std::string_view pattern) : CommandBase("KEYS {}", pattern)
        {
        }
    };

    // Skip migrate

    struct Move : CommandBase {
        using ResultType = typename T::Integer;
        Move(std::string_view key, std::string_view db) : CommandBase("MOVE {} {}", key, db)
        {
        }
    };
};
} // namespace wibens::resp
