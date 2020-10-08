// Prepare for some makro madness
#define REP_CASE1(off, ...)                  \
    case off + 0: {                          \
        const constexpr uint32_t offset = 0; \
        __VA_ARGS__;                         \
    } break
#define REP_CASE2(off, ...)                  \
    case off + 1: {                          \
        const constexpr uint32_t offset = 1; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE1(off, __VA_ARGS__)
#define REP_CASE3(off, ...)                  \
    case off + 2: {                          \
        const constexpr uint32_t offset = 2; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE2(off, __VA_ARGS__)
#define REP_CASE4(off, ...)                  \
    case off + 3: {                          \
        const constexpr uint32_t offset = 3; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE3(off, __VA_ARGS__)
#define REP_CASE5(off, ...)                  \
    case off + 4: {                          \
        const constexpr uint32_t offset = 4; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE4(off, __VA_ARGS__)
#define REP_CASE6(off, ...)                  \
    case off + 5: {                          \
        const constexpr uint32_t offset = 5; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE5(off, __VA_ARGS__)
#define REP_CASE7(off, ...)                  \
    case off + 6: {                          \
        const constexpr uint32_t offset = 6; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE6(off, __VA_ARGS__)
#define REP_CASE8(off, ...)                  \
    case off + 7: {                          \
        const constexpr uint32_t offset = 7; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE7(off, __VA_ARGS__)
#define REP_CASE9(off, ...)                  \
    case off + 8: {                          \
        const constexpr uint32_t offset = 8; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE8(off, __VA_ARGS__)
#define REP_CASE10(off, ...)                 \
    case off + 9: {                          \
        const constexpr uint32_t offset = 9; \
        __VA_ARGS__;                         \
    } break;                                 \
        REP_CASE9(off, __VA_ARGS__)
#define REP_CASE11(off, ...)                  \
    case off + 10: {                          \
        const constexpr uint32_t offset = 10; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE10(off, __VA_ARGS__)
#define REP_CASE12(off, ...)                  \
    case off + 11: {                          \
        const constexpr uint32_t offset = 11; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE11(off, __VA_ARGS__)
#define REP_CASE13(off, ...)                  \
    case off + 12: {                          \
        const constexpr uint32_t offset = 12; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE12(off, __VA_ARGS__)
#define REP_CASE14(off, ...)                  \
    case off + 13: {                          \
        const constexpr uint32_t offset = 13; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE13(off, __VA_ARGS__)
#define REP_CASE15(off, ...)                  \
    case off + 14: {                          \
        const constexpr uint32_t offset = 14; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE14(off, __VA_ARGS__)
#define REP_CASE16(off, ...)                  \
    case off + 15: {                          \
        const constexpr uint32_t offset = 15; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE15(off, __VA_ARGS__)
#define REP_CASE17(off, ...)                  \
    case off + 16: {                          \
        const constexpr uint32_t offset = 16; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE16(off, __VA_ARGS__)
#define REP_CASE18(off, ...)                  \
    case off + 17: {                          \
        const constexpr uint32_t offset = 17; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE17(off, __VA_ARGS__)
#define REP_CASE19(off, ...)                  \
    case off + 18: {                          \
        const constexpr uint32_t offset = 18; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE18(off, __VA_ARGS__)
#define REP_CASE20(off, ...)                  \
    case off + 19: {                          \
        const constexpr uint32_t offset = 19; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE19(off, __VA_ARGS__)
#define REP_CASE21(off, ...)                  \
    case off + 20: {                          \
        const constexpr uint32_t offset = 20; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE20(off, __VA_ARGS__)
#define REP_CASE22(off, ...)                  \
    case off + 21: {                          \
        const constexpr uint32_t offset = 21; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE21(off, __VA_ARGS__)
#define REP_CASE23(off, ...)                  \
    case off + 22: {                          \
        const constexpr uint32_t offset = 22; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE22(off, __VA_ARGS__)
#define REP_CASE24(off, ...)                  \
    case off + 23: {                          \
        const constexpr uint32_t offset = 23; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE23(off, __VA_ARGS__)
#define REP_CASE25(off, ...)                  \
    case off + 24: {                          \
        const constexpr uint32_t offset = 24; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE24(off, __VA_ARGS__)
#define REP_CASE26(off, ...)                  \
    case off + 25: {                          \
        const constexpr uint32_t offset = 25; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE25(off, __VA_ARGS__)
#define REP_CASE27(off, ...)                  \
    case off + 26: {                          \
        const constexpr uint32_t offset = 26; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE26(off, __VA_ARGS__)
#define REP_CASE28(off, ...)                  \
    case off + 27: {                          \
        const constexpr uint32_t offset = 27; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE27(off, __VA_ARGS__)
#define REP_CASE29(off, ...)                  \
    case off + 28: {                          \
        const constexpr uint32_t offset = 28; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE28(off, __VA_ARGS__)
#define REP_CASE30(off, ...)                  \
    case off + 29: {                          \
        const constexpr uint32_t offset = 29; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE29(off, __VA_ARGS__)
#define REP_CASE31(off, ...)                  \
    case off + 30: {                          \
        const constexpr uint32_t offset = 30; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE30(off, __VA_ARGS__)
#define REP_CASE32(off, ...)                  \
    case off + 31: {                          \
        const constexpr uint32_t offset = 31; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE31(off, __VA_ARGS__)
#define REP_CASE33(off, ...)                  \
    case off + 32: {                          \
        const constexpr uint32_t offset = 32; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE32(off, __VA_ARGS__)
#define REP_CASE34(off, ...)                  \
    case off + 33: {                          \
        const constexpr uint32_t offset = 33; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE33(off, __VA_ARGS__)
#define REP_CASE35(off, ...)                  \
    case off + 34: {                          \
        const constexpr uint32_t offset = 34; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE34(off, __VA_ARGS__)
#define REP_CASE36(off, ...)                  \
    case off + 35: {                          \
        const constexpr uint32_t offset = 35; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE35(off, __VA_ARGS__)
#define REP_CASE37(off, ...)                  \
    case off + 36: {                          \
        const constexpr uint32_t offset = 36; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE36(off, __VA_ARGS__)
#define REP_CASE38(off, ...)                  \
    case off + 37: {                          \
        const constexpr uint32_t offset = 37; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE37(off, __VA_ARGS__)
#define REP_CASE39(off, ...)                  \
    case off + 38: {                          \
        const constexpr uint32_t offset = 38; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE38(off, __VA_ARGS__)
#define REP_CASE40(off, ...)                  \
    case off + 39: {                          \
        const constexpr uint32_t offset = 39; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE39(off, __VA_ARGS__)
#define REP_CASE41(off, ...)                  \
    case off + 40: {                          \
        const constexpr uint32_t offset = 40; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE40(off, __VA_ARGS__)
#define REP_CASE42(off, ...)                  \
    case off + 41: {                          \
        const constexpr uint32_t offset = 41; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE41(off, __VA_ARGS__)
#define REP_CASE43(off, ...)                  \
    case off + 42: {                          \
        const constexpr uint32_t offset = 42; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE42(off, __VA_ARGS__)
#define REP_CASE44(off, ...)                  \
    case off + 43: {                          \
        const constexpr uint32_t offset = 43; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE43(off, __VA_ARGS__)
#define REP_CASE45(off, ...)                  \
    case off + 44: {                          \
        const constexpr uint32_t offset = 44; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE44(off, __VA_ARGS__)
#define REP_CASE46(off, ...)                  \
    case off + 45: {                          \
        const constexpr uint32_t offset = 45; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE45(off, __VA_ARGS__)
#define REP_CASE47(off, ...)                  \
    case off + 46: {                          \
        const constexpr uint32_t offset = 46; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE46(off, __VA_ARGS__)
#define REP_CASE48(off, ...)                  \
    case off + 47: {                          \
        const constexpr uint32_t offset = 47; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE47(off, __VA_ARGS__)
#define REP_CASE49(off, ...)                  \
    case off + 48: {                          \
        const constexpr uint32_t offset = 48; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE48(off, __VA_ARGS__)
#define REP_CASE50(off, ...)                  \
    case off + 49: {                          \
        const constexpr uint32_t offset = 49; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE49(off, __VA_ARGS__)
#define REP_CASE51(off, ...)                  \
    case off + 50: {                          \
        const constexpr uint32_t offset = 50; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE50(off, __VA_ARGS__)
#define REP_CASE52(off, ...)                  \
    case off + 51: {                          \
        const constexpr uint32_t offset = 51; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE51(off, __VA_ARGS__)
#define REP_CASE53(off, ...)                  \
    case off + 52: {                          \
        const constexpr uint32_t offset = 52; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE52(off, __VA_ARGS__)
#define REP_CASE54(off, ...)                  \
    case off + 53: {                          \
        const constexpr uint32_t offset = 53; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE53(off, __VA_ARGS__)
#define REP_CASE55(off, ...)                  \
    case off + 54: {                          \
        const constexpr uint32_t offset = 54; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE54(off, __VA_ARGS__)
#define REP_CASE56(off, ...)                  \
    case off + 55: {                          \
        const constexpr uint32_t offset = 55; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE55(off, __VA_ARGS__)
#define REP_CASE57(off, ...)                  \
    case off + 56: {                          \
        const constexpr uint32_t offset = 56; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE56(off, __VA_ARGS__)
#define REP_CASE58(off, ...)                  \
    case off + 57: {                          \
        const constexpr uint32_t offset = 57; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE57(off, __VA_ARGS__)
#define REP_CASE59(off, ...)                  \
    case off + 58: {                          \
        const constexpr uint32_t offset = 58; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE58(off, __VA_ARGS__)
#define REP_CASE60(off, ...)                  \
    case off + 59: {                          \
        const constexpr uint32_t offset = 59; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE59(off, __VA_ARGS__)
#define REP_CASE61(off, ...)                  \
    case off + 60: {                          \
        const constexpr uint32_t offset = 60; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE60(off, __VA_ARGS__)
#define REP_CASE62(off, ...)                  \
    case off + 61: {                          \
        const constexpr uint32_t offset = 61; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE61(off, __VA_ARGS__)
#define REP_CASE63(off, ...)                  \
    case off + 62: {                          \
        const constexpr uint32_t offset = 62; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE62(off, __VA_ARGS__)
#define REP_CASE64(off, ...)                  \
    case off + 63: {                          \
        const constexpr uint32_t offset = 63; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE63(off, __VA_ARGS__)
#define REP_CASE65(off, ...)                  \
    case off + 64: {                          \
        const constexpr uint32_t offset = 64; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE64(off, __VA_ARGS__)
#define REP_CASE66(off, ...)                  \
    case off + 65: {                          \
        const constexpr uint32_t offset = 65; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE65(off, __VA_ARGS__)
#define REP_CASE67(off, ...)                  \
    case off + 66: {                          \
        const constexpr uint32_t offset = 66; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE66(off, __VA_ARGS__)
#define REP_CASE68(off, ...)                  \
    case off + 67: {                          \
        const constexpr uint32_t offset = 67; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE67(off, __VA_ARGS__)
#define REP_CASE69(off, ...)                  \
    case off + 68: {                          \
        const constexpr uint32_t offset = 68; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE68(off, __VA_ARGS__)
#define REP_CASE70(off, ...)                  \
    case off + 69: {                          \
        const constexpr uint32_t offset = 69; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE69(off, __VA_ARGS__)
#define REP_CASE71(off, ...)                  \
    case off + 70: {                          \
        const constexpr uint32_t offset = 70; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE70(off, __VA_ARGS__)
#define REP_CASE72(off, ...)                  \
    case off + 71: {                          \
        const constexpr uint32_t offset = 71; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE71(off, __VA_ARGS__)
#define REP_CASE73(off, ...)                  \
    case off + 72: {                          \
        const constexpr uint32_t offset = 72; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE72(off, __VA_ARGS__)
#define REP_CASE74(off, ...)                  \
    case off + 73: {                          \
        const constexpr uint32_t offset = 73; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE73(off, __VA_ARGS__)
#define REP_CASE75(off, ...)                  \
    case off + 74: {                          \
        const constexpr uint32_t offset = 74; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE74(off, __VA_ARGS__)
#define REP_CASE76(off, ...)                  \
    case off + 75: {                          \
        const constexpr uint32_t offset = 75; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE75(off, __VA_ARGS__)
#define REP_CASE77(off, ...)                  \
    case off + 76: {                          \
        const constexpr uint32_t offset = 76; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE76(off, __VA_ARGS__)
#define REP_CASE78(off, ...)                  \
    case off + 77: {                          \
        const constexpr uint32_t offset = 77; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE77(off, __VA_ARGS__)
#define REP_CASE79(off, ...)                  \
    case off + 78: {                          \
        const constexpr uint32_t offset = 78; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE78(off, __VA_ARGS__)
#define REP_CASE80(off, ...)                  \
    case off + 79: {                          \
        const constexpr uint32_t offset = 79; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE79(off, __VA_ARGS__)
#define REP_CASE81(off, ...)                  \
    case off + 80: {                          \
        const constexpr uint32_t offset = 80; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE80(off, __VA_ARGS__)
#define REP_CASE82(off, ...)                  \
    case off + 81: {                          \
        const constexpr uint32_t offset = 81; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE81(off, __VA_ARGS__)
#define REP_CASE83(off, ...)                  \
    case off + 82: {                          \
        const constexpr uint32_t offset = 82; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE82(off, __VA_ARGS__)
#define REP_CASE84(off, ...)                  \
    case off + 83: {                          \
        const constexpr uint32_t offset = 83; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE83(off, __VA_ARGS__)
#define REP_CASE85(off, ...)                  \
    case off + 84: {                          \
        const constexpr uint32_t offset = 84; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE84(off, __VA_ARGS__)
#define REP_CASE86(off, ...)                  \
    case off + 85: {                          \
        const constexpr uint32_t offset = 85; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE85(off, __VA_ARGS__)
#define REP_CASE87(off, ...)                  \
    case off + 86: {                          \
        const constexpr uint32_t offset = 86; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE86(off, __VA_ARGS__)
#define REP_CASE88(off, ...)                  \
    case off + 87: {                          \
        const constexpr uint32_t offset = 87; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE87(off, __VA_ARGS__)
#define REP_CASE89(off, ...)                  \
    case off + 88: {                          \
        const constexpr uint32_t offset = 88; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE88(off, __VA_ARGS__)
#define REP_CASE90(off, ...)                  \
    case off + 89: {                          \
        const constexpr uint32_t offset = 89; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE89(off, __VA_ARGS__)
#define REP_CASE91(off, ...)                  \
    case off + 90: {                          \
        const constexpr uint32_t offset = 90; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE90(off, __VA_ARGS__)
#define REP_CASE92(off, ...)                  \
    case off + 91: {                          \
        const constexpr uint32_t offset = 91; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE91(off, __VA_ARGS__)
#define REP_CASE93(off, ...)                  \
    case off + 92: {                          \
        const constexpr uint32_t offset = 92; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE92(off, __VA_ARGS__)
#define REP_CASE94(off, ...)                  \
    case off + 93: {                          \
        const constexpr uint32_t offset = 93; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE93(off, __VA_ARGS__)
#define REP_CASE95(off, ...)                  \
    case off + 94: {                          \
        const constexpr uint32_t offset = 94; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE94(off, __VA_ARGS__)
#define REP_CASE96(off, ...)                  \
    case off + 95: {                          \
        const constexpr uint32_t offset = 95; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE95(off, __VA_ARGS__)
#define REP_CASE97(off, ...)                  \
    case off + 96: {                          \
        const constexpr uint32_t offset = 96; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE96(off, __VA_ARGS__)
#define REP_CASE98(off, ...)                  \
    case off + 97: {                          \
        const constexpr uint32_t offset = 97; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE97(off, __VA_ARGS__)
#define REP_CASE99(off, ...)                  \
    case off + 98: {                          \
        const constexpr uint32_t offset = 98; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE98(off, __VA_ARGS__)
#define REP_CASE100(off, ...)                 \
    case off + 99: {                          \
        const constexpr uint32_t offset = 99; \
        __VA_ARGS__;                          \
    } break;                                  \
        REP_CASE99(off, __VA_ARGS__)

#define REP_CASE(n, off, ...) REP_CASE##n(off, __VA_ARGS__)