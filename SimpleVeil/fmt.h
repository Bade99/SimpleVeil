#pragma once

#define FMT_HEADER_ONLY
#include "fmt/format-inl.h" //Thanks to https://github.com/fmtlib/fmt
#define _f(...) fmt::format(__VA_ARGS__)