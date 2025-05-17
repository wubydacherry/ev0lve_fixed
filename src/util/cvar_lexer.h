
#ifndef UTIL_CVAR_LEXER_H
#define UTIL_CVAR_LEXER_H

#include <map>
#include <string>
#include <vector>

namespace util
{
std::vector<std::string> parse_cvars(std::string cmd);
}

#endif // UTIL_CVAR_LEXER_H
