#include "ast.h"
#include "util.h"
#include <format>

namespace wibens::resp::ast
{

Node::Node() : data("_\r\n")
{
}

Node::Node(const SimpleString &value) : data(std::format("+{}\r\n", static_cast<std::string_view>(value)))
{
}

Node::Node(std::string_view value) : data(std::format("${}\r\n{}\r\n", value.size(), value))
{
}

Node::Node(int64_t value) : data(std::format(":{}\r\n", value))
{
}

Node::Node(double value) : data(std::format(",{:.{}g}\r\n", value, std::numeric_limits<double>::max_digits10))
{
}

Node::Node(bool value) : data(value ? "#t\r\n" : "#f\r\n")
{
}

Node::Node(const Type &value)
{
    std::visit([this](auto &v) { *this = Node(v); }, value);
}

void Node::makeAggregate(char type)
{
    auto size = children.size();
    if (type == '%') {
        size /= 2;
    }
    data = std::format("{}{}\r\n", type, size);
    for (const auto &child : children) {
        data += child->data;
    }
}

Node::Node(Raw, std::string_view value) : data(value)
{
}

template <typename T> T readnum(std::string_view input)
{
    if (T value{}; std::from_chars(input.data(), input.data() + input.size(), value).ec == std::errc()) {
        return value;
    }
    throw std::runtime_error("Invalid number format");
}

std::tuple<Node::Ptr, std::string_view> Node::parse(std::string_view input)
{
    struct FirstLineResult {
        char type = 0;
        std::string_view raw;
        std::string_view trimmed;
        std::string_view rest;
    };
    auto firstLine = [](std::string_view s) -> FirstLineResult {
        auto end = s.find("\r\n");
        if (end == std::string_view::npos) {
            return {0, {}, {}, s};
        }
        return {s[0], s.substr(0, end + 2), s.substr(1, end - 1), s.substr(end + 2)};
    };

    auto [type, raw, trimmed, rest] = firstLine(input);
    if (type == 0) {
        return {nullptr, input};
    }

    switch (type) {
        case '+':
        case '-':
        case '(':
        case ':':
        case '_':
        case '#':
        case ',':
            return {Ptr{new Node(Raw{}, raw)}, rest};
        case '!':
        case '=':
        case '$': {
            auto size = readnum<size_t>(trimmed);
            rest = rest.substr(size + 2);
            return {Ptr{new Node(Raw{}, input.substr(0, raw.size() + size + 2))}, rest};
        }
        case '*':
        case '%':
        case '>': {
            Ptr node = std::make_unique<Node>();
            bool isMap = (type == '%');
            auto size = readnum<size_t>(trimmed);
            if (isMap) {
                size *= 2;
            }
            for (size_t i = 0; i < size; ++i) {
                Ptr child;
                std::tie(child, rest) = parse(rest);
                if (!child) {
                    return {nullptr, input};
                }
                node->children.push_back(std::move(child));
            }
            auto totalSize = input.size() - rest.size();
            node->data = input.substr(0, totalSize);
            return {std::move(node), rest};
        }
        default:
            throw std::runtime_error("Unexpected prefix in response: " + std::string{input[0]});
    }
}

void Node::rebuild()
{
    if (util::oneOf<'*', '%'>(data[0])) {
        makeAggregate(data[0]);
    }
}

} // namespace wibens::resp::ast
