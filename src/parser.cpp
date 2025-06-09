#include "parser.h"
#include "error.h"
#include <charconv>

namespace wibens::resp::parser
{
std::string Parser<std::string>::parse(std::string_view &input)
{
    std::string result;
    if (input[0] == '+') {
        input.remove_prefix(1);
        auto end = input.find("\r\n");
        result = std::string{input.substr(0, end)};
        input.remove_prefix(end + 2);
    } else if (input[0] == '$') {
        input.remove_prefix(1);
        auto endSize = input.find("\r\n");
        auto size = readnum<size_t>(input.substr(0, endSize));
        input.remove_prefix(endSize + 2);
        result = std::string{input.substr(0, size)};
        input.remove_prefix(size + 2);
    } else {
        throw error::ParseError("Invalid response format");
    }
    return result;
}

int64_t Parser<int64_t>::parse(std::string_view &input)
{
    int64_t value{};
    input.remove_prefix(1);
    if (auto end = input.find("\r\n"); std::from_chars(input.data(), input.data() + end, value).ec == std::errc()) {
        input.remove_prefix(end + 2);
        return value;
    }
    throw error::ParseError("Invalid number format");
}
} // namespace wibens::resp::parser
