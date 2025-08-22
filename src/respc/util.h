#pragma once

#include <format>
#include <ranges>
#include <string>
#include <string_view>

namespace wibens::resp::util
{
template <typename Range> std::string join(const Range &r, std::string_view sep)
{
    std::string out;
    bool first = true;
    for (const auto &e : r) {
        if (!first) {
            out += sep;
            first = false;
        }
        out += std::format("{}", e);
    }
    return out;
}

template <auto... VALUES> bool oneOf(const auto &value)
{
    return ((value == VALUES) || ...);
}
} // namespace wibens::resp::util
