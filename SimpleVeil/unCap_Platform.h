#pragma once
#pragma once
#include <cstdint> //uint8_t
#include <string> //unfortunate

typedef uint8_t u8; //prepping for jai
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef wchar_t utf16;
typedef char32_t utf32;

#ifdef UNICODE
typedef std::wstring str;
typedef std::wstring_view str_view;
typedef wchar_t cstr;
#else
typedef std::string str;
typedef std::string_view str_view;
typedef char cstr;
#endif