#pragma once

#include "ast.h"
#include <string>

namespace wibens::resp
{

struct CommandBase {
    CommandBase() = default;
    template <typename... ARGS> CommandBase(ARGS &&...args)
    {
        buffer = ast::Node(std::forward<ARGS>(args)...).toString();
    }
    void build(const ast::Node &node)
    {
        buffer = node.toString();
    }
    const std::string &getCommand() const
    {
        return buffer;
    }

  private:
    std::string buffer;
};
} // namespace wibens::resp
