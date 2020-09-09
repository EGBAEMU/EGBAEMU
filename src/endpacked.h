#ifdef __clang__
/*code specific to clang compiler*/
#undef PACKED
#elif __GNUC__
/*code for GNU C compiler */
#undef PACKED
#elif _MSC_VER
/*usually has the version number in _MSC_VER*/
/*code specific to MSVC compiler*/
#pragma pack(pop)
#undef PACKED
#elif __MINGW32__
/*code specific to mingw compilers*/
#undef PACKED
#else
#error "unsupported compiler!"
#endif