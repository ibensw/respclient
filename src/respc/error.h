#pragma once

#include <stdexcept>

namespace wibens::resp::error
{
struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};
} // namespace wibens::resp::error
