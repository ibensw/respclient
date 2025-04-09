#pragma once

#include "command.h"
#include "types.h"

template <typename T = Types<>> struct Generic {
    struct Get : CommandBase {
        using ResultType = typename T::template Variant<typename T::Null, typename T::String>;
        Get(std::string_view key) : CommandBase("GET {}", key)
        {
        }
    };

    struct Set : CommandBase {
        using ResultType = typename T::template Variant<typename T::Null, typename T::String>;
        Set(std::string_view key, T::String value, bool xx = false, bool nx = false, bool get = false)
            : CommandBase("SET {} {}{}{}{}", key, value, xx ? " XX" : "", nx ? " NX" : "", get ? " GET" : "")
        {
        }
    };

    struct Del : CommandBase {
        using ResultType = T::Integer;
        Del(std::initializer_list<std::string_view> keys) : CommandBase("DEL {}", fmt::join(keys, " "))
        {
        }
        template <typename... Args> Del(Args... args) : Del(std::initializer_list<std::string_view>{args...})
        {
        }
    };

    struct Copy : CommandBase {
        using ResultType = T::Integer;
        Copy(std::string_view source, std::string_view destination,
             std::optional<std::string_view> database = std::nullopt, bool replace = false)
            : CommandBase("COPY {} {}{}{}", source, destination, database ? fmt::format("DB {}", *database) : "",
                          replace ? " REPLACE" : "")
        {
        }
    };

    struct Dump : CommandBase {
        using ResultType = typename T::template Variant<typename T::Null, typename T::String>;
        Dump(std::string_view key) : CommandBase("DUMP {}", key)
        {
        }
    };

    struct Exists : CommandBase {
        using ResultType = T::Integer;
        Exists(std::initializer_list<std::string_view> keys) : CommandBase("EXISTS {}", fmt::join(keys, " "))
        {
        }
        template <typename... Args> Exists(Args... args) : Exists(std::initializer_list<std::string_view>{args...})
        {
        }
    };
};
