#include "parser.h"
#include "error.h"
#include <charconv>

namespace wibens::resp::parser
{
std::string Parser<std::string>::parse(ast::Node *input)
{
    std::string_view inputView = input->toString();
    std::string result;
    if (input->type() == '+') {
        result = inputView.substr(1, inputView.size() - 3);
    } else if (input->type() == '$') {
        inputView.remove_prefix(1);
        auto endSize = inputView.find("\r\n");
        auto size = readnum<size_t>(inputView.substr(0, endSize));
        inputView.remove_prefix(endSize + 2);
        result = std::string{inputView.substr(0, size)};
    } else {
        throw error::ParseError("Invalid response format");
    }
    return result;
}

int64_t Parser<int64_t>::parse(ast::Node *input)
{
    int64_t value{};
    std::string_view inputView = input->toString();
    inputView.remove_prefix(1);
    if (auto end = inputView.find("\r\n");
        std::from_chars(inputView.data(), inputView.data() + end, value).ec == std::errc()) {
        return value;
    }
    throw error::ParseError("Invalid number format");
}
} // namespace wibens::resp::parser
