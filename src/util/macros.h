
#ifndef UTIL_MACROS_H
#define UTIL_MACROS_H

#include <util/value_obfuscation.h>

#define RUNTIME_ERROR(msg) throw std::runtime_error(XOR_STR(msg))
#define INVALID_ARGUMENT(msg) throw std::invalid_argument(XOR_STR(msg))

#endif // UTIL_MACROS_H
