#pragma once

#include <cstdint>
#include <memory>
#include <ranges>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace wibens::resp::ast
{

template <typename T>
concept SequenceContainer =
    std::ranges::range<T> && requires { typename T::value_type; } && !std::convertible_to<T, std::string_view>;

template <typename T>
concept AssociativeContainer = std::ranges::range<T> && requires(T a) {
    typename T::key_type;
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;

    { a.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
    { a.count(std::declval<typename T::key_type>()) } -> std::convertible_to<typename T::size_type>;
};

class SimpleString : public std::string_view
{
  public:
    using std::string_view::string_view;
};

class Node
{
  public:
    using Ptr = std::unique_ptr<Node>;
    using Type = std::variant<SimpleString, std::string_view, uint64_t, double, bool>;

    static std::tuple<Ptr, std::string_view> parse(std::string_view input);
    std::vector<Ptr> children;

    explicit Node();
    explicit Node(const SimpleString &value);
    explicit Node(std::string_view value);
    explicit Node(const std::string &value) : Node(std::string_view(value))
    {
    }
    explicit Node(const char *value) : Node(std::string_view(value))
    {
    }
    explicit Node(int64_t value);
    template <std::integral T> explicit Node(T value) : Node(static_cast<int64_t>(value))
    {
    }
    explicit Node(double value);
    explicit Node(bool value);
    explicit Node(const Type &value);
    explicit Node(const SequenceContainer auto &values)
    {
        for (const auto &value : values) {
            children.emplace_back(std::make_unique<Node>(value));
        }
        makeAggregate('*');
    }
    template <typename First, typename Second, typename... ARGS> explicit Node(First first, Second second, ARGS... args)
    {
        addChild(first);
        addChild(second);
        (addChild(args), ...);
        makeAggregate('*');
    }

    explicit Node(const AssociativeContainer auto &values)
    {
        for (const auto &pair : values) {
            children.emplace_back(std::make_unique<Node>(pair.first));
            children.emplace_back(std::make_unique<Node>(pair.second));
        }
        makeAggregate('%');
    }

    const std::string &toString() const
    {
        return data;
    }

    char type() const
    {
        return data[0];
    }

    void rebuild();

    template <typename T> void addChild(const T &child)
    {
        children.emplace_back(std::make_unique<Node>(child));
    }

  private:
    struct Raw {
    };
    Node(Raw, std::string_view value);
    void makeAggregate(char type);

    std::string data;
};
} // namespace wibens::resp::ast
