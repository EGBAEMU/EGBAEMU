// Prepare for some makro madness
#define REP_CASE1(off, glbOff, ...)           \
    case off + 0: {                           \
        const uint32_t offset = (glbOff) - (off); \
        __VA_ARGS__;                          \
    } break
#define REP_CASE2(off, glbOff, ...) \
    case off + 1:                   \
        REP_CASE1(off, glbOff, __VA_ARGS__)
#define REP_CASE3(off, glbOff, ...) \
    case off + 2:                   \
        REP_CASE2(off, glbOff, __VA_ARGS__)
#define REP_CASE4(off, glbOff, ...) \
    case off + 3:                   \
        REP_CASE3(off, glbOff, __VA_ARGS__)
#define REP_CASE5(off, glbOff, ...) \
    case off + 4:                   \
        REP_CASE4(off, glbOff, __VA_ARGS__)
#define REP_CASE6(off, glbOff, ...) \
    case off + 5:                   \
        REP_CASE5(off, glbOff, __VA_ARGS__)
#define REP_CASE7(off, glbOff, ...) \
    case off + 6:                   \
        REP_CASE6(off, glbOff, __VA_ARGS__)
#define REP_CASE8(off, glbOff, ...) \
    case off + 7:                   \
        REP_CASE7(off, glbOff, __VA_ARGS__)
#define REP_CASE9(off, glbOff, ...) \
    case off + 8:                   \
        REP_CASE8(off, glbOff, __VA_ARGS__)
#define REP_CASE10(off, glbOff, ...) \
    case off + 9:                    \
        REP_CASE9(off, glbOff, __VA_ARGS__)
#define REP_CASE11(off, glbOff, ...) \
    case off + 10:                   \
        REP_CASE10(off, glbOff, __VA_ARGS__)
#define REP_CASE12(off, glbOff, ...) \
    case off + 11:                   \
        REP_CASE11(off, glbOff, __VA_ARGS__)
#define REP_CASE13(off, glbOff, ...) \
    case off + 12:                   \
        REP_CASE12(off, glbOff, __VA_ARGS__)
#define REP_CASE14(off, glbOff, ...) \
    case off + 13:                   \
        REP_CASE13(off, glbOff, __VA_ARGS__)
#define REP_CASE15(off, glbOff, ...) \
    case off + 14:                   \
        REP_CASE14(off, glbOff, __VA_ARGS__)
#define REP_CASE16(off, glbOff, ...) \
    case off + 15:                   \
        REP_CASE15(off, glbOff, __VA_ARGS__)
#define REP_CASE17(off, glbOff, ...) \
    case off + 16:                   \
        REP_CASE16(off, glbOff, __VA_ARGS__)
#define REP_CASE18(off, glbOff, ...) \
    case off + 17:                   \
        REP_CASE17(off, glbOff, __VA_ARGS__)
#define REP_CASE19(off, glbOff, ...) \
    case off + 18:                   \
        REP_CASE18(off, glbOff, __VA_ARGS__)
#define REP_CASE20(off, glbOff, ...) \
    case off + 19:                   \
        REP_CASE19(off, glbOff, __VA_ARGS__)
#define REP_CASE21(off, glbOff, ...) \
    case off + 20:                   \
        REP_CASE20(off, glbOff, __VA_ARGS__)
#define REP_CASE22(off, glbOff, ...) \
    case off + 21:                   \
        REP_CASE21(off, glbOff, __VA_ARGS__)
#define REP_CASE23(off, glbOff, ...) \
    case off + 22:                   \
        REP_CASE22(off, glbOff, __VA_ARGS__)
#define REP_CASE24(off, glbOff, ...) \
    case off + 23:                   \
        REP_CASE23(off, glbOff, __VA_ARGS__)
#define REP_CASE25(off, glbOff, ...) \
    case off + 24:                   \
        REP_CASE24(off, glbOff, __VA_ARGS__)
#define REP_CASE26(off, glbOff, ...) \
    case off + 25:                   \
        REP_CASE25(off, glbOff, __VA_ARGS__)
#define REP_CASE27(off, glbOff, ...) \
    case off + 26:                   \
        REP_CASE26(off, glbOff, __VA_ARGS__)
#define REP_CASE28(off, glbOff, ...) \
    case off + 27:                   \
        REP_CASE27(off, glbOff, __VA_ARGS__)
#define REP_CASE29(off, glbOff, ...) \
    case off + 28:                   \
        REP_CASE28(off, glbOff, __VA_ARGS__)
#define REP_CASE30(off, glbOff, ...) \
    case off + 29:                   \
        REP_CASE29(off, glbOff, __VA_ARGS__)
#define REP_CASE31(off, glbOff, ...) \
    case off + 30:                   \
        REP_CASE30(off, glbOff, __VA_ARGS__)
#define REP_CASE32(off, glbOff, ...) \
    case off + 31:                   \
        REP_CASE31(off, glbOff, __VA_ARGS__)
#define REP_CASE33(off, glbOff, ...) \
    case off + 32:                   \
        REP_CASE32(off, glbOff, __VA_ARGS__)
#define REP_CASE34(off, glbOff, ...) \
    case off + 33:                   \
        REP_CASE33(off, glbOff, __VA_ARGS__)
#define REP_CASE35(off, glbOff, ...) \
    case off + 34:                   \
        REP_CASE34(off, glbOff, __VA_ARGS__)
#define REP_CASE36(off, glbOff, ...) \
    case off + 35:                   \
        REP_CASE35(off, glbOff, __VA_ARGS__)
#define REP_CASE37(off, glbOff, ...) \
    case off + 36:                   \
        REP_CASE36(off, glbOff, __VA_ARGS__)
#define REP_CASE38(off, glbOff, ...) \
    case off + 37:                   \
        REP_CASE37(off, glbOff, __VA_ARGS__)
#define REP_CASE39(off, glbOff, ...) \
    case off + 38:                   \
        REP_CASE38(off, glbOff, __VA_ARGS__)
#define REP_CASE40(off, glbOff, ...) \
    case off + 39:                   \
        REP_CASE39(off, glbOff, __VA_ARGS__)
#define REP_CASE41(off, glbOff, ...) \
    case off + 40:                   \
        REP_CASE40(off, glbOff, __VA_ARGS__)
#define REP_CASE42(off, glbOff, ...) \
    case off + 41:                   \
        REP_CASE41(off, glbOff, __VA_ARGS__)
#define REP_CASE43(off, glbOff, ...) \
    case off + 42:                   \
        REP_CASE42(off, glbOff, __VA_ARGS__)
#define REP_CASE44(off, glbOff, ...) \
    case off + 43:                   \
        REP_CASE43(off, glbOff, __VA_ARGS__)
#define REP_CASE45(off, glbOff, ...) \
    case off + 44:                   \
        REP_CASE44(off, glbOff, __VA_ARGS__)
#define REP_CASE46(off, glbOff, ...) \
    case off + 45:                   \
        REP_CASE45(off, glbOff, __VA_ARGS__)
#define REP_CASE47(off, glbOff, ...) \
    case off + 46:                   \
        REP_CASE46(off, glbOff, __VA_ARGS__)
#define REP_CASE48(off, glbOff, ...) \
    case off + 47:                   \
        REP_CASE47(off, glbOff, __VA_ARGS__)
#define REP_CASE49(off, glbOff, ...) \
    case off + 48:                   \
        REP_CASE48(off, glbOff, __VA_ARGS__)
#define REP_CASE50(off, glbOff, ...) \
    case off + 49:                   \
        REP_CASE49(off, glbOff, __VA_ARGS__)
#define REP_CASE51(off, glbOff, ...) \
    case off + 50:                   \
        REP_CASE50(off, glbOff, __VA_ARGS__)
#define REP_CASE52(off, glbOff, ...) \
    case off + 51:                   \
        REP_CASE51(off, glbOff, __VA_ARGS__)
#define REP_CASE53(off, glbOff, ...) \
    case off + 52:                   \
        REP_CASE52(off, glbOff, __VA_ARGS__)
#define REP_CASE54(off, glbOff, ...) \
    case off + 53:                   \
        REP_CASE53(off, glbOff, __VA_ARGS__)
#define REP_CASE55(off, glbOff, ...) \
    case off + 54:                   \
        REP_CASE54(off, glbOff, __VA_ARGS__)
#define REP_CASE56(off, glbOff, ...) \
    case off + 55:                   \
        REP_CASE55(off, glbOff, __VA_ARGS__)
#define REP_CASE57(off, glbOff, ...) \
    case off + 56:                   \
        REP_CASE56(off, glbOff, __VA_ARGS__)
#define REP_CASE58(off, glbOff, ...) \
    case off + 57:                   \
        REP_CASE57(off, glbOff, __VA_ARGS__)
#define REP_CASE59(off, glbOff, ...) \
    case off + 58:                   \
        REP_CASE58(off, glbOff, __VA_ARGS__)
#define REP_CASE60(off, glbOff, ...) \
    case off + 59:                   \
        REP_CASE59(off, glbOff, __VA_ARGS__)
#define REP_CASE61(off, glbOff, ...) \
    case off + 60:                   \
        REP_CASE60(off, glbOff, __VA_ARGS__)
#define REP_CASE62(off, glbOff, ...) \
    case off + 61:                   \
        REP_CASE61(off, glbOff, __VA_ARGS__)
#define REP_CASE63(off, glbOff, ...) \
    case off + 62:                   \
        REP_CASE62(off, glbOff, __VA_ARGS__)
#define REP_CASE64(off, glbOff, ...) \
    case off + 63:                   \
        REP_CASE63(off, glbOff, __VA_ARGS__)
#define REP_CASE65(off, glbOff, ...) \
    case off + 64:                   \
        REP_CASE64(off, glbOff, __VA_ARGS__)
#define REP_CASE66(off, glbOff, ...) \
    case off + 65:                   \
        REP_CASE65(off, glbOff, __VA_ARGS__)
#define REP_CASE67(off, glbOff, ...) \
    case off + 66:                   \
        REP_CASE66(off, glbOff, __VA_ARGS__)
#define REP_CASE68(off, glbOff, ...) \
    case off + 67:                   \
        REP_CASE67(off, glbOff, __VA_ARGS__)
#define REP_CASE69(off, glbOff, ...) \
    case off + 68:                   \
        REP_CASE68(off, glbOff, __VA_ARGS__)
#define REP_CASE70(off, glbOff, ...) \
    case off + 69:                   \
        REP_CASE69(off, glbOff, __VA_ARGS__)
#define REP_CASE71(off, glbOff, ...) \
    case off + 70:                   \
        REP_CASE70(off, glbOff, __VA_ARGS__)
#define REP_CASE72(off, glbOff, ...) \
    case off + 71:                   \
        REP_CASE71(off, glbOff, __VA_ARGS__)
#define REP_CASE73(off, glbOff, ...) \
    case off + 72:                   \
        REP_CASE72(off, glbOff, __VA_ARGS__)
#define REP_CASE74(off, glbOff, ...) \
    case off + 73:                   \
        REP_CASE73(off, glbOff, __VA_ARGS__)
#define REP_CASE75(off, glbOff, ...) \
    case off + 74:                   \
        REP_CASE74(off, glbOff, __VA_ARGS__)
#define REP_CASE76(off, glbOff, ...) \
    case off + 75:                   \
        REP_CASE75(off, glbOff, __VA_ARGS__)
#define REP_CASE77(off, glbOff, ...) \
    case off + 76:                   \
        REP_CASE76(off, glbOff, __VA_ARGS__)
#define REP_CASE78(off, glbOff, ...) \
    case off + 77:                   \
        REP_CASE77(off, glbOff, __VA_ARGS__)
#define REP_CASE79(off, glbOff, ...) \
    case off + 78:                   \
        REP_CASE78(off, glbOff, __VA_ARGS__)
#define REP_CASE80(off, glbOff, ...) \
    case off + 79:                   \
        REP_CASE79(off, glbOff, __VA_ARGS__)
#define REP_CASE81(off, glbOff, ...) \
    case off + 80:                   \
        REP_CASE80(off, glbOff, __VA_ARGS__)
#define REP_CASE82(off, glbOff, ...) \
    case off + 81:                   \
        REP_CASE81(off, glbOff, __VA_ARGS__)
#define REP_CASE83(off, glbOff, ...) \
    case off + 82:                   \
        REP_CASE82(off, glbOff, __VA_ARGS__)
#define REP_CASE84(off, glbOff, ...) \
    case off + 83:                   \
        REP_CASE83(off, glbOff, __VA_ARGS__)
#define REP_CASE85(off, glbOff, ...) \
    case off + 84:                   \
        REP_CASE84(off, glbOff, __VA_ARGS__)
#define REP_CASE86(off, glbOff, ...) \
    case off + 85:                   \
        REP_CASE85(off, glbOff, __VA_ARGS__)
#define REP_CASE87(off, glbOff, ...) \
    case off + 86:                   \
        REP_CASE86(off, glbOff, __VA_ARGS__)
#define REP_CASE88(off, glbOff, ...) \
    case off + 87:                   \
        REP_CASE87(off, glbOff, __VA_ARGS__)
#define REP_CASE89(off, glbOff, ...) \
    case off + 88:                   \
        REP_CASE88(off, glbOff, __VA_ARGS__)
#define REP_CASE90(off, glbOff, ...) \
    case off + 89:                   \
        REP_CASE89(off, glbOff, __VA_ARGS__)
#define REP_CASE91(off, glbOff, ...) \
    case off + 90:                   \
        REP_CASE90(off, glbOff, __VA_ARGS__)
#define REP_CASE92(off, glbOff, ...) \
    case off + 91:                   \
        REP_CASE91(off, glbOff, __VA_ARGS__)
#define REP_CASE93(off, glbOff, ...) \
    case off + 92:                   \
        REP_CASE92(off, glbOff, __VA_ARGS__)
#define REP_CASE94(off, glbOff, ...) \
    case off + 93:                   \
        REP_CASE93(off, glbOff, __VA_ARGS__)
#define REP_CASE95(off, glbOff, ...) \
    case off + 94:                   \
        REP_CASE94(off, glbOff, __VA_ARGS__)
#define REP_CASE96(off, glbOff, ...) \
    case off + 95:                   \
        REP_CASE95(off, glbOff, __VA_ARGS__)
#define REP_CASE97(off, glbOff, ...) \
    case off + 96:                   \
        REP_CASE96(off, glbOff, __VA_ARGS__)
#define REP_CASE98(off, glbOff, ...) \
    case off + 97:                   \
        REP_CASE97(off, glbOff, __VA_ARGS__)
#define REP_CASE99(off, glbOff, ...) \
    case off + 98:                   \
        REP_CASE98(off, glbOff, __VA_ARGS__)
#define REP_CASE100(off, glbOff, ...) \
    case off + 99:                    \
        REP_CASE99(off, glbOff, __VA_ARGS__)

#define REP_CASE(n, glbOff, off, ...) REP_CASE##n(off, glbOff, __VA_ARGS__)