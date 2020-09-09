#ifdef __clang__
/*code specific to clang compiler*/
#define PACKED __attribute__((__packed__))
#elif __GNUC__
/*code for GNU C compiler */
#define PACKED __attribute__((__packed__))
#elif _MSC_VER
/*usually has the version number in _MSC_VER*/
/*code specific to MSVC compiler*/
#define PACKED
#pragma pack(push, 1)
#elif __MINGW32__
/*code specific to mingw compilers*/
#define PACKED __attribute__((__packed__))
#else
#error "unsupported compiler!"
#endif