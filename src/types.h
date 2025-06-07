#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace wibens::resp
{
template <typename T> using DefaultVector = std::vector<T>;
template <typename K, typename V> using DefaultMap = std::unordered_map<K, V>;
template <typename StringType = std::string, typename IntegerType = int64_t, typename NullType = std::monostate,
          typename BooleanType = bool, typename DoubleType = double, typename BigIntType = std::string,
          template <typename> typename ArrayType = DefaultVector,
          template <typename, typename> typename MapType = DefaultMap,
          template <typename...> typename VariantType = std::variant>
struct Types {
    using BigInt = BigIntType;
    using Boolean = BooleanType;
    using Double = DoubleType;
    using Integer = IntegerType;
    using Null = NullType;
    using String = StringType;
    template <typename T> using Array = ArrayType<T>;
    template <typename K, typename V> using Map = MapType<K, V>;
    template <typename... T> using Variant = VariantType<T...>;
};
} // namespace wibens::resp
