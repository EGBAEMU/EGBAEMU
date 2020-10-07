// Prepare for some makro madness
#define REP_CASE1(off, body)                           \
    case off + 0: {                                    \
        const constexpr uint32_t offset = 0;           \
        body;                                          \
    } break #define REP_CASE2(off, body) case off + 1: \
    {                                                  \
        const constexpr uint32_t offset = 1;           \
        body;                                          \
    }                                                  \
        break;                                         \
        REP_CASE1(off, body)
#define REP_CASE3(off, body)                 \
    case off + 2: {                          \
        const constexpr uint32_t offset = 2; \
        body;                                \
    } break;                                 \
        REP_CASE2(off, body)
#define REP_CASE4(off, body)                 \
    case off + 3: {                          \
        const constexpr uint32_t offset = 3; \
        body;                                \
    } break;                                 \
        REP_CASE3(off, body)
#define REP_CASE5(off, body)                 \
    case off + 4: {                          \
        const constexpr uint32_t offset = 4; \
        body;                                \
    } break;                                 \
        REP_CASE4(off, body)
#define REP_CASE6(off, body)                 \
    case off + 5: {                          \
        const constexpr uint32_t offset = 5; \
        body;                                \
    } break;                                 \
        REP_CASE5(off, body)
#define REP_CASE7(off, body)                 \
    case off + 6: {                          \
        const constexpr uint32_t offset = 6; \
        body;                                \
    } break;                                 \
        REP_CASE6(off, body)
#define REP_CASE8(off, body)                 \
    case off + 7: {                          \
        const constexpr uint32_t offset = 7; \
        body;                                \
    } break;                                 \
        REP_CASE7(off, body)
#define REP_CASE9(off, body)                 \
    case off + 8: {                          \
        const constexpr uint32_t offset = 8; \
        body;                                \
    } break;                                 \
        REP_CASE8(off, body)
#define REP_CASE10(off, body)                \
    case off + 9: {                          \
        const constexpr uint32_t offset = 9; \
        body;                                \
    } break;                                 \
        REP_CASE9(off, body)
#define REP_CASE11(off, body)                 \
    case off + 10: {                          \
        const constexpr uint32_t offset = 10; \
        body;                                 \
    } break;                                  \
        REP_CASE10(off, body)
#define REP_CASE12(off, body)                 \
    case off + 11: {                          \
        const constexpr uint32_t offset = 11; \
        body;                                 \
    } break;                                  \
        REP_CASE11(off, body)
#define REP_CASE13(off, body)                 \
    case off + 12: {                          \
        const constexpr uint32_t offset = 12; \
        body;                                 \
    } break;                                  \
        REP_CASE12(off, body)
#define REP_CASE14(off, body)                 \
    case off + 13: {                          \
        const constexpr uint32_t offset = 13; \
        body;                                 \
    } break;                                  \
        REP_CASE13(off, body)
#define REP_CASE15(off, body)                 \
    case off + 14: {                          \
        const constexpr uint32_t offset = 14; \
        body;                                 \
    } break;                                  \
        REP_CASE14(off, body)
#define REP_CASE16(off, body)                 \
    case off + 15: {                          \
        const constexpr uint32_t offset = 15; \
        body;                                 \
    } break;                                  \
        REP_CASE15(off, body)
#define REP_CASE17(off, body)                 \
    case off + 16: {                          \
        const constexpr uint32_t offset = 16; \
        body;                                 \
    } break;                                  \
        REP_CASE16(off, body)
#define REP_CASE18(off, body)                 \
    case off + 17: {                          \
        const constexpr uint32_t offset = 17; \
        body;                                 \
    } break;                                  \
        REP_CASE17(off, body)
#define REP_CASE19(off, body)                 \
    case off + 18: {                          \
        const constexpr uint32_t offset = 18; \
        body;                                 \
    } break;                                  \
        REP_CASE18(off, body)
#define REP_CASE20(off, body)                 \
    case off + 19: {                          \
        const constexpr uint32_t offset = 19; \
        body;                                 \
    } break;                                  \
        REP_CASE19(off, body)
#define REP_CASE21(off, body)                 \
    case off + 20: {                          \
        const constexpr uint32_t offset = 20; \
        body;                                 \
    } break;                                  \
        REP_CASE20(off, body)
#define REP_CASE22(off, body)                 \
    case off + 21: {                          \
        const constexpr uint32_t offset = 21; \
        body;                                 \
    } break;                                  \
        REP_CASE21(off, body)
#define REP_CASE23(off, body)                 \
    case off + 22: {                          \
        const constexpr uint32_t offset = 22; \
        body;                                 \
    } break;                                  \
        REP_CASE22(off, body)
#define REP_CASE24(off, body)                 \
    case off + 23: {                          \
        const constexpr uint32_t offset = 23; \
        body;                                 \
    } break;                                  \
        REP_CASE23(off, body)
#define REP_CASE25(off, body)                 \
    case off + 24: {                          \
        const constexpr uint32_t offset = 24; \
        body;                                 \
    } break;                                  \
        REP_CASE24(off, body)
#define REP_CASE26(off, body)                 \
    case off + 25: {                          \
        const constexpr uint32_t offset = 25; \
        body;                                 \
    } break;                                  \
        REP_CASE25(off, body)
#define REP_CASE27(off, body)                 \
    case off + 26: {                          \
        const constexpr uint32_t offset = 26; \
        body;                                 \
    } break;                                  \
        REP_CASE26(off, body)
#define REP_CASE28(off, body)                 \
    case off + 27: {                          \
        const constexpr uint32_t offset = 27; \
        body;                                 \
    } break;                                  \
        REP_CASE27(off, body)
#define REP_CASE29(off, body)                 \
    case off + 28: {                          \
        const constexpr uint32_t offset = 28; \
        body;                                 \
    } break;                                  \
        REP_CASE28(off, body)
#define REP_CASE30(off, body)                 \
    case off + 29: {                          \
        const constexpr uint32_t offset = 29; \
        body;                                 \
    } break;                                  \
        REP_CASE29(off, body)
#define REP_CASE31(off, body)                 \
    case off + 30: {                          \
        const constexpr uint32_t offset = 30; \
        body;                                 \
    } break;                                  \
        REP_CASE30(off, body)
#define REP_CASE32(off, body)                 \
    case off + 31: {                          \
        const constexpr uint32_t offset = 31; \
        body;                                 \
    } break;                                  \
        REP_CASE31(off, body)
#define REP_CASE33(off, body)                 \
    case off + 32: {                          \
        const constexpr uint32_t offset = 32; \
        body;                                 \
    } break;                                  \
        REP_CASE32(off, body)
#define REP_CASE34(off, body)                 \
    case off + 33: {                          \
        const constexpr uint32_t offset = 33; \
        body;                                 \
    } break;                                  \
        REP_CASE33(off, body)
#define REP_CASE35(off, body)                 \
    case off + 34: {                          \
        const constexpr uint32_t offset = 34; \
        body;                                 \
    } break;                                  \
        REP_CASE34(off, body)
#define REP_CASE36(off, body)                 \
    case off + 35: {                          \
        const constexpr uint32_t offset = 35; \
        body;                                 \
    } break;                                  \
        REP_CASE35(off, body)
#define REP_CASE37(off, body)                 \
    case off + 36: {                          \
        const constexpr uint32_t offset = 36; \
        body;                                 \
    } break;                                  \
        REP_CASE36(off, body)
#define REP_CASE38(off, body)                 \
    case off + 37: {                          \
        const constexpr uint32_t offset = 37; \
        body;                                 \
    } break;                                  \
        REP_CASE37(off, body)
#define REP_CASE39(off, body)                 \
    case off + 38: {                          \
        const constexpr uint32_t offset = 38; \
        body;                                 \
    } break;                                  \
        REP_CASE38(off, body)
#define REP_CASE40(off, body)                 \
    case off + 39: {                          \
        const constexpr uint32_t offset = 39; \
        body;                                 \
    } break;                                  \
        REP_CASE39(off, body)
#define REP_CASE41(off, body)                 \
    case off + 40: {                          \
        const constexpr uint32_t offset = 40; \
        body;                                 \
    } break;                                  \
        REP_CASE40(off, body)
#define REP_CASE42(off, body)                 \
    case off + 41: {                          \
        const constexpr uint32_t offset = 41; \
        body;                                 \
    } break;                                  \
        REP_CASE41(off, body)
#define REP_CASE43(off, body)                 \
    case off + 42: {                          \
        const constexpr uint32_t offset = 42; \
        body;                                 \
    } break;                                  \
        REP_CASE42(off, body)
#define REP_CASE44(off, body)                 \
    case off + 43: {                          \
        const constexpr uint32_t offset = 43; \
        body;                                 \
    } break;                                  \
        REP_CASE43(off, body)
#define REP_CASE45(off, body)                 \
    case off + 44: {                          \
        const constexpr uint32_t offset = 44; \
        body;                                 \
    } break;                                  \
        REP_CASE44(off, body)
#define REP_CASE46(off, body)                 \
    case off + 45: {                          \
        const constexpr uint32_t offset = 45; \
        body;                                 \
    } break;                                  \
        REP_CASE45(off, body)
#define REP_CASE47(off, body)                 \
    case off + 46: {                          \
        const constexpr uint32_t offset = 46; \
        body;                                 \
    } break;                                  \
        REP_CASE46(off, body)
#define REP_CASE48(off, body)                 \
    case off + 47: {                          \
        const constexpr uint32_t offset = 47; \
        body;                                 \
    } break;                                  \
        REP_CASE47(off, body)
#define REP_CASE49(off, body)                 \
    case off + 48: {                          \
        const constexpr uint32_t offset = 48; \
        body;                                 \
    } break;                                  \
        REP_CASE48(off, body)
#define REP_CASE50(off, body)                 \
    case off + 49: {                          \
        const constexpr uint32_t offset = 49; \
        body;                                 \
    } break;                                  \
        REP_CASE49(off, body)
#define REP_CASE51(off, body)                 \
    case off + 50: {                          \
        const constexpr uint32_t offset = 50; \
        body;                                 \
    } break;                                  \
        REP_CASE50(off, body)
#define REP_CASE52(off, body)                 \
    case off + 51: {                          \
        const constexpr uint32_t offset = 51; \
        body;                                 \
    } break;                                  \
        REP_CASE51(off, body)
#define REP_CASE53(off, body)                 \
    case off + 52: {                          \
        const constexpr uint32_t offset = 52; \
        body;                                 \
    } break;                                  \
        REP_CASE52(off, body)
#define REP_CASE54(off, body)                 \
    case off + 53: {                          \
        const constexpr uint32_t offset = 53; \
        body;                                 \
    } break;                                  \
        REP_CASE53(off, body)
#define REP_CASE55(off, body)                 \
    case off + 54: {                          \
        const constexpr uint32_t offset = 54; \
        body;                                 \
    } break;                                  \
        REP_CASE54(off, body)
#define REP_CASE56(off, body)                 \
    case off + 55: {                          \
        const constexpr uint32_t offset = 55; \
        body;                                 \
    } break;                                  \
        REP_CASE55(off, body)
#define REP_CASE57(off, body)                 \
    case off + 56: {                          \
        const constexpr uint32_t offset = 56; \
        body;                                 \
    } break;                                  \
        REP_CASE56(off, body)
#define REP_CASE58(off, body)                 \
    case off + 57: {                          \
        const constexpr uint32_t offset = 57; \
        body;                                 \
    } break;                                  \
        REP_CASE57(off, body)
#define REP_CASE59(off, body)                 \
    case off + 58: {                          \
        const constexpr uint32_t offset = 58; \
        body;                                 \
    } break;                                  \
        REP_CASE58(off, body)
#define REP_CASE60(off, body)                 \
    case off + 59: {                          \
        const constexpr uint32_t offset = 59; \
        body;                                 \
    } break;                                  \
        REP_CASE59(off, body)
#define REP_CASE61(off, body)                 \
    case off + 60: {                          \
        const constexpr uint32_t offset = 60; \
        body;                                 \
    } break;                                  \
        REP_CASE60(off, body)
#define REP_CASE62(off, body)                 \
    case off + 61: {                          \
        const constexpr uint32_t offset = 61; \
        body;                                 \
    } break;                                  \
        REP_CASE61(off, body)
#define REP_CASE63(off, body)                 \
    case off + 62: {                          \
        const constexpr uint32_t offset = 62; \
        body;                                 \
    } break;                                  \
        REP_CASE62(off, body)
#define REP_CASE64(off, body)                 \
    case off + 63: {                          \
        const constexpr uint32_t offset = 63; \
        body;                                 \
    } break;                                  \
        REP_CASE63(off, body)
#define REP_CASE65(off, body)                 \
    case off + 64: {                          \
        const constexpr uint32_t offset = 64; \
        body;                                 \
    } break;                                  \
        REP_CASE64(off, body)
#define REP_CASE66(off, body)                 \
    case off + 65: {                          \
        const constexpr uint32_t offset = 65; \
        body;                                 \
    } break;                                  \
        REP_CASE65(off, body)
#define REP_CASE67(off, body)                 \
    case off + 66: {                          \
        const constexpr uint32_t offset = 66; \
        body;                                 \
    } break;                                  \
        REP_CASE66(off, body)
#define REP_CASE68(off, body)                 \
    case off + 67: {                          \
        const constexpr uint32_t offset = 67; \
        body;                                 \
    } break;                                  \
        REP_CASE67(off, body)
#define REP_CASE69(off, body)                 \
    case off + 68: {                          \
        const constexpr uint32_t offset = 68; \
        body;                                 \
    } break;                                  \
        REP_CASE68(off, body)
#define REP_CASE70(off, body)                 \
    case off + 69: {                          \
        const constexpr uint32_t offset = 69; \
        body;                                 \
    } break;                                  \
        REP_CASE69(off, body)
#define REP_CASE71(off, body)                 \
    case off + 70: {                          \
        const constexpr uint32_t offset = 70; \
        body;                                 \
    } break;                                  \
        REP_CASE70(off, body)
#define REP_CASE72(off, body)                 \
    case off + 71: {                          \
        const constexpr uint32_t offset = 71; \
        body;                                 \
    } break;                                  \
        REP_CASE71(off, body)
#define REP_CASE73(off, body)                 \
    case off + 72: {                          \
        const constexpr uint32_t offset = 72; \
        body;                                 \
    } break;                                  \
        REP_CASE72(off, body)
#define REP_CASE74(off, body)                 \
    case off + 73: {                          \
        const constexpr uint32_t offset = 73; \
        body;                                 \
    } break;                                  \
        REP_CASE73(off, body)
#define REP_CASE75(off, body)                 \
    case off + 74: {                          \
        const constexpr uint32_t offset = 74; \
        body;                                 \
    } break;                                  \
        REP_CASE74(off, body)
#define REP_CASE76(off, body)                 \
    case off + 75: {                          \
        const constexpr uint32_t offset = 75; \
        body;                                 \
    } break;                                  \
        REP_CASE75(off, body)
#define REP_CASE77(off, body)                 \
    case off + 76: {                          \
        const constexpr uint32_t offset = 76; \
        body;                                 \
    } break;                                  \
        REP_CASE76(off, body)
#define REP_CASE78(off, body)                 \
    case off + 77: {                          \
        const constexpr uint32_t offset = 77; \
        body;                                 \
    } break;                                  \
        REP_CASE77(off, body)
#define REP_CASE79(off, body)                 \
    case off + 78: {                          \
        const constexpr uint32_t offset = 78; \
        body;                                 \
    } break;                                  \
        REP_CASE78(off, body)
#define REP_CASE80(off, body)                 \
    case off + 79: {                          \
        const constexpr uint32_t offset = 79; \
        body;                                 \
    } break;                                  \
        REP_CASE79(off, body)
#define REP_CASE81(off, body)                 \
    case off + 80: {                          \
        const constexpr uint32_t offset = 80; \
        body;                                 \
    } break;                                  \
        REP_CASE80(off, body)
#define REP_CASE82(off, body)                 \
    case off + 81: {                          \
        const constexpr uint32_t offset = 81; \
        body;                                 \
    } break;                                  \
        REP_CASE81(off, body)
#define REP_CASE83(off, body)                 \
    case off + 82: {                          \
        const constexpr uint32_t offset = 82; \
        body;                                 \
    } break;                                  \
        REP_CASE82(off, body)
#define REP_CASE84(off, body)                 \
    case off + 83: {                          \
        const constexpr uint32_t offset = 83; \
        body;                                 \
    } break;                                  \
        REP_CASE83(off, body)
#define REP_CASE85(off, body)                 \
    case off + 84: {                          \
        const constexpr uint32_t offset = 84; \
        body;                                 \
    } break;                                  \
        REP_CASE84(off, body)
#define REP_CASE86(off, body)                 \
    case off + 85: {                          \
        const constexpr uint32_t offset = 85; \
        body;                                 \
    } break;                                  \
        REP_CASE85(off, body)
#define REP_CASE87(off, body)                 \
    case off + 86: {                          \
        const constexpr uint32_t offset = 86; \
        body;                                 \
    } break;                                  \
        REP_CASE86(off, body)
#define REP_CASE88(off, body)                 \
    case off + 87: {                          \
        const constexpr uint32_t offset = 87; \
        body;                                 \
    } break;                                  \
        REP_CASE87(off, body)
#define REP_CASE89(off, body)                 \
    case off + 88: {                          \
        const constexpr uint32_t offset = 88; \
        body;                                 \
    } break;                                  \
        REP_CASE88(off, body)
#define REP_CASE90(off, body)                 \
    case off + 89: {                          \
        const constexpr uint32_t offset = 89; \
        body;                                 \
    } break;                                  \
        REP_CASE89(off, body)
#define REP_CASE91(off, body)                 \
    case off + 90: {                          \
        const constexpr uint32_t offset = 90; \
        body;                                 \
    } break;                                  \
        REP_CASE90(off, body)
#define REP_CASE92(off, body)                 \
    case off + 91: {                          \
        const constexpr uint32_t offset = 91; \
        body;                                 \
    } break;                                  \
        REP_CASE91(off, body)
#define REP_CASE93(off, body)                 \
    case off + 92: {                          \
        const constexpr uint32_t offset = 92; \
        body;                                 \
    } break;                                  \
        REP_CASE92(off, body)
#define REP_CASE94(off, body)                 \
    case off + 93: {                          \
        const constexpr uint32_t offset = 93; \
        body;                                 \
    } break;                                  \
        REP_CASE93(off, body)
#define REP_CASE95(off, body)                 \
    case off + 94: {                          \
        const constexpr uint32_t offset = 94; \
        body;                                 \
    } break;                                  \
        REP_CASE94(off, body)
#define REP_CASE96(off, body)                 \
    case off + 95: {                          \
        const constexpr uint32_t offset = 95; \
        body;                                 \
    } break;                                  \
        REP_CASE95(off, body)
#define REP_CASE97(off, body)                 \
    case off + 96: {                          \
        const constexpr uint32_t offset = 96; \
        body;                                 \
    } break;                                  \
        REP_CASE96(off, body)
#define REP_CASE98(off, body)                 \
    case off + 97: {                          \
        const constexpr uint32_t offset = 97; \
        body;                                 \
    } break;                                  \
        REP_CASE97(off, body)
#define REP_CASE99(off, body)                 \
    case off + 98: {                          \
        const constexpr uint32_t offset = 98; \
        body;                                 \
    } break;                                  \
        REP_CASE98(off, body)
#define REP_CASE100(off, body)                \
    case off + 99: {                          \
        const constexpr uint32_t offset = 99; \
        body;                                 \
    } break;                                  \
        REP_CASE99(off, body)

#define REP_CASE(n, off, body) REP_CASE##n(off, n - 1, body)