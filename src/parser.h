#pragma once

#include "error.h"
#include "types.h"
#include <charconv>
#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace wibens::resp::parser
{
template <typename T> T readnum(std::string_view input)
{
    if (T value{}; std::from_chars(input.data(), input.data() + input.size(), value).ec == std::errc()) {
        return value;
    }
    throw error::ParseError("Invalid number format");
}

template <typename T> struct Parser {
};

template <> struct Parser<std::string> {
    static constexpr inline std::string_view prefixes = "+$";
    static std::string parse(std::string_view &input);
};

template <> struct Parser<int64_t> {
    static constexpr inline std::string_view prefixes = ":";
    static int64_t parse(std::string_view &input);
};

template <> struct Parser<std::chrono::system_clock::time_point> : Parser<int64_t> {
    static auto parse(std::string_view &input)
    {
        return std::chrono::system_clock::time_point{std::chrono::seconds{Parser<int64_t>::parse(input)}};
    }
};

template <> struct Parser<std::monostate> {
    static constexpr inline std::string_view prefixes = "_";
    static std::monostate parse(std::string_view &input)
    {
        input.remove_prefix(3);
        return {};
    }
};

template <typename... Args> struct Parser<std::variant<Args...>> {
    static std::variant<Args...> parse(std::string_view &input)
    {
        return parseInner<Args...>(input);
    }

  private:
    template <typename T, typename... Ts> static std::variant<Args...> parseInner(std::string_view &input)
    {
        if (Parser<T>::prefixes.find(input[0]) != std::string::npos) {
            return Parser<T>::parse(input);
        }
        if constexpr (sizeof...(Ts) > 0) {
            return parseInner<Ts...>(input);
        }
        throw error::ParseError("Invalid response format");
    }
};

template <typename K, typename V, typename... Args> struct Parser<std::unordered_map<K, V, Args...>> {
    static constexpr inline std::string_view prefixes = "%";
    static std::unordered_map<K, V, Args...> parse(std::string_view &input)
    {
        std::unordered_map<K, V, Args...> result;
        input.remove_prefix(1);
        auto endSize = input.find("\r\n");
        auto count = readnum<size_t>(input.substr(0, endSize));
        input.remove_prefix(endSize + 2);
        for (size_t i = 0; i < count; ++i) {
            auto key = Parser<K>::parse(input);
            auto value = Parser<V>::parse(input);
            result.emplace(key, value);
        }
        return result;
    }
};

template <typename T, typename... Args> struct Parser<std::vector<T, Args...>> {
    static constexpr inline std::string_view prefixes = "*";
    static std::vector<T> parse(std::string_view &input)
    {
        std::vector<T, Args...> result;
        input.remove_prefix(1);
        auto endSize = input.find("\r\n");
        auto count = readnum<size_t>(input.substr(0, endSize));
        input.remove_prefix(endSize + 2);
        result.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            auto value = Parser<T>::parse(input);
            result.emplace_back(value);
        }
        return result;
    }
};

struct IgnoreAll {
};

template <> struct Parser<IgnoreAll> {
    static constexpr inline std::string_view prefixes = "+-:$*_#,(!=%`~>";
    static IgnoreAll parse(const std::string_view &)
    {
        return {};
    }
};
} // namespace wibens::resp::parser
