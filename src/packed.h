#ifndef PACKED_H
#define PACKED_H

#ifdef __clang__
/*code specific to clang compiler*/
#define PACK_STRUCT(Name, InsName, ...) \
    struct Name {                       \
        __VA_ARGS__                     \
    } __attribute__((__packed__)) InsName

#define PACK_STRUCT_DEF(Name, ...) \
    struct Name {                  \
        __VA_ARGS__                \
    } __attribute__((__packed__))
#elif __GNUC__
/*code for GNU C compiler */
#define PACK_STRUCT(Name, InsName, ...) \
    struct Name {                       \
        __VA_ARGS__                     \
    } __attribute__((__packed__)) InsName

#define PACK_STRUCT_DEF(Name, ...) \
    struct Name {                  \
        __VA_ARGS__                \
    } __attribute__((__packed__))
#elif _MSC_VER
/*usually has the version number in _MSC_VER*/
/*code specific to MSVC compiler*/
#define PACK_STRUCT(Name, InsName, ...)    \
    _Pragma("pack(push, 1)") struct Name { \
        __VA_ARGS__                        \
    } _Pragma("pack(pop)")                 \
        InsName

#define PACK_STRUCT_DEF(Name, ...)         \
    _Pragma("pack(push, 1)") struct Name { \
        __VA_ARGS__                        \
    } _Pragma("pack(pop)")
#elif __MINGW32__
/*code specific to mingw compilers*/
#define PACK_STRUCT(Name, InsName, ...) \
    struct Name {                       \
        __VA_ARGS__                     \
    } __attribute__((__packed__)) InsName

#define PACK_STRUCT_DEF(Name, ...) \
    struct Name {                  \
        __VA_ARGS__                \
    } __attribute__((__packed__))
#else
#error "unsupported compiler!"
#endif

#endif