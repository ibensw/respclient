#pragma once

#include "ast.h"
#include "error.h"
#include "types.h"
#include <charconv>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

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
    static std::string parse(ast::Node *input);
};

template <> struct Parser<int64_t> {
    static constexpr inline std::string_view prefixes = ":";
    static int64_t parse(ast::Node *input);
};

// template <> struct Parser<std::chrono::system_clock::time_point> : Parser<int64_t> {
//     static auto parse(ast::Node* input)
//     {
//         return std::chrono::system_clock::time_point{std::chrono::seconds{Parser<int64_t>::parse(input)}};
//     }
// };

template <> struct Parser<std::monostate> {
    static constexpr inline std::string_view prefixes = "_";
    static inline std::monostate parse(ast::Node *)
    {
        return {};
    }
};

template <typename... Args> struct Parser<std::variant<Args...>> {
    static std::variant<Args...> parse(ast::Node *input)
    {
        return parseInner<Args...>(input);
    }

  private:
    template <typename T, typename... Ts> static std::variant<Args...> parseInner(ast::Node *input)
    {
        if (Parser<T>::prefixes.find(input->type()) != std::string::npos) {
            return Parser<T>::parse(input);
        }
        if constexpr (sizeof...(Ts) > 0) {
            return parseInner<Ts...>(input);
        }
        throw error::ParseError("Invalid response format");
    }
};

template <typename T> struct Parser<std::optional<T>> : Parser<std::variant<std::monostate, T>> {
    static std::optional<T> parse(ast::Node *input)
    {
        if (auto result = Parser<std::variant<std::monostate, T>>::parse(input);
            std::holds_alternative<std::monostate>(result)) {
            return std::nullopt;
        } else {
            return std::get<T>(result);
        }
    }
};

template <typename K, typename V, typename... Args> struct Parser<std::unordered_map<K, V, Args...>> {
    static constexpr inline std::string_view prefixes = "%";
    static std::unordered_map<K, V, Args...> parse(ast::Node *input)
    {
        std::unordered_map<K, V, Args...> result;
        auto count = input->children.size() / 2;
        for (size_t i = 0; i < count; ++i) {
            result.emplace(Parser<K>::parse(input->children[i * 2].get()),
                           Parser<V>::parse(input->children[i * 2 + 1].get()));
        }
        return result;
    }
};

template <typename T, typename... Args> struct Parser<std::vector<T, Args...>> {
    static constexpr inline std::string_view prefixes = "*";
    static std::vector<T> parse(ast::Node *input)
    {
        std::vector<T, Args...> result;
        for (const auto &child : input->children) {
            result.emplace_back(Parser<T>::parse(child.get()));
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
