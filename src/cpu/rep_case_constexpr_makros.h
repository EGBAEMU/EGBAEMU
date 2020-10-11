#ifndef REP_CASE_CONSTEXPR_MAKROS_H
#define REP_CASE_CONSTEXPR_MAKROS_H

#define REP_CASE_CONSTEXPR_1(type, off, ...)   \
    case off + 0: {                            \
        const constexpr type offset = off + 0; \
        __VA_ARGS__;                           \
    } break
#define REP_CASE_CONSTEXPR_2(type, off, ...)   \
    case off + 1: {                            \
        const constexpr type offset = off + 1; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_1(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_3(type, off, ...)   \
    case off + 2: {                            \
        const constexpr type offset = off + 2; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_2(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_4(type, off, ...)   \
    case off + 3: {                            \
        const constexpr type offset = off + 3; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_3(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_5(type, off, ...)   \
    case off + 4: {                            \
        const constexpr type offset = off + 4; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_4(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_6(type, off, ...)   \
    case off + 5: {                            \
        const constexpr type offset = off + 5; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_5(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_7(type, off, ...)   \
    case off + 6: {                            \
        const constexpr type offset = off + 6; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_6(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_8(type, off, ...)   \
    case off + 7: {                            \
        const constexpr type offset = off + 7; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_7(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_9(type, off, ...)   \
    case off + 8: {                            \
        const constexpr type offset = off + 8; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_8(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_10(type, off, ...)  \
    case off + 9: {                            \
        const constexpr type offset = off + 9; \
        __VA_ARGS__;                           \
    } break;                                   \
        REP_CASE_CONSTEXPR_9(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_11(type, off, ...)   \
    case off + 10: {                            \
        const constexpr type offset = off + 10; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_10(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_12(type, off, ...)   \
    case off + 11: {                            \
        const constexpr type offset = off + 11; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_11(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_13(type, off, ...)   \
    case off + 12: {                            \
        const constexpr type offset = off + 12; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_12(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_14(type, off, ...)   \
    case off + 13: {                            \
        const constexpr type offset = off + 13; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_13(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_15(type, off, ...)   \
    case off + 14: {                            \
        const constexpr type offset = off + 14; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_14(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_16(type, off, ...)   \
    case off + 15: {                            \
        const constexpr type offset = off + 15; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_15(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_17(type, off, ...)   \
    case off + 16: {                            \
        const constexpr type offset = off + 16; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_16(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_18(type, off, ...)   \
    case off + 17: {                            \
        const constexpr type offset = off + 17; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_17(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_19(type, off, ...)   \
    case off + 18: {                            \
        const constexpr type offset = off + 18; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_18(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_20(type, off, ...)   \
    case off + 19: {                            \
        const constexpr type offset = off + 19; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_19(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_21(type, off, ...)   \
    case off + 20: {                            \
        const constexpr type offset = off + 20; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_20(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_22(type, off, ...)   \
    case off + 21: {                            \
        const constexpr type offset = off + 21; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_21(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_23(type, off, ...)   \
    case off + 22: {                            \
        const constexpr type offset = off + 22; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_22(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_24(type, off, ...)   \
    case off + 23: {                            \
        const constexpr type offset = off + 23; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_23(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_25(type, off, ...)   \
    case off + 24: {                            \
        const constexpr type offset = off + 24; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_24(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_26(type, off, ...)   \
    case off + 25: {                            \
        const constexpr type offset = off + 25; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_25(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_27(type, off, ...)   \
    case off + 26: {                            \
        const constexpr type offset = off + 26; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_26(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_28(type, off, ...)   \
    case off + 27: {                            \
        const constexpr type offset = off + 27; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_27(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_29(type, off, ...)   \
    case off + 28: {                            \
        const constexpr type offset = off + 28; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_28(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_30(type, off, ...)   \
    case off + 29: {                            \
        const constexpr type offset = off + 29; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_29(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_31(type, off, ...)   \
    case off + 30: {                            \
        const constexpr type offset = off + 30; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_30(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_32(type, off, ...)   \
    case off + 31: {                            \
        const constexpr type offset = off + 31; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_31(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_33(type, off, ...)   \
    case off + 32: {                            \
        const constexpr type offset = off + 32; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_32(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_34(type, off, ...)   \
    case off + 33: {                            \
        const constexpr type offset = off + 33; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_33(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_35(type, off, ...)   \
    case off + 34: {                            \
        const constexpr type offset = off + 34; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_34(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_36(type, off, ...)   \
    case off + 35: {                            \
        const constexpr type offset = off + 35; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_35(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_37(type, off, ...)   \
    case off + 36: {                            \
        const constexpr type offset = off + 36; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_36(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_38(type, off, ...)   \
    case off + 37: {                            \
        const constexpr type offset = off + 37; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_37(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_39(type, off, ...)   \
    case off + 38: {                            \
        const constexpr type offset = off + 38; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_38(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_40(type, off, ...)   \
    case off + 39: {                            \
        const constexpr type offset = off + 39; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_39(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_41(type, off, ...)   \
    case off + 40: {                            \
        const constexpr type offset = off + 40; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_40(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_42(type, off, ...)   \
    case off + 41: {                            \
        const constexpr type offset = off + 41; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_41(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_43(type, off, ...)   \
    case off + 42: {                            \
        const constexpr type offset = off + 42; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_42(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_44(type, off, ...)   \
    case off + 43: {                            \
        const constexpr type offset = off + 43; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_43(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_45(type, off, ...)   \
    case off + 44: {                            \
        const constexpr type offset = off + 44; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_44(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_46(type, off, ...)   \
    case off + 45: {                            \
        const constexpr type offset = off + 45; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_45(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_47(type, off, ...)   \
    case off + 46: {                            \
        const constexpr type offset = off + 46; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_46(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_48(type, off, ...)   \
    case off + 47: {                            \
        const constexpr type offset = off + 47; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_47(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_49(type, off, ...)   \
    case off + 48: {                            \
        const constexpr type offset = off + 48; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_48(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_50(type, off, ...)   \
    case off + 49: {                            \
        const constexpr type offset = off + 49; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_49(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_51(type, off, ...)   \
    case off + 50: {                            \
        const constexpr type offset = off + 50; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_50(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_52(type, off, ...)   \
    case off + 51: {                            \
        const constexpr type offset = off + 51; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_51(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_53(type, off, ...)   \
    case off + 52: {                            \
        const constexpr type offset = off + 52; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_52(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_54(type, off, ...)   \
    case off + 53: {                            \
        const constexpr type offset = off + 53; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_53(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_55(type, off, ...)   \
    case off + 54: {                            \
        const constexpr type offset = off + 54; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_54(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_56(type, off, ...)   \
    case off + 55: {                            \
        const constexpr type offset = off + 55; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_55(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_57(type, off, ...)   \
    case off + 56: {                            \
        const constexpr type offset = off + 56; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_56(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_58(type, off, ...)   \
    case off + 57: {                            \
        const constexpr type offset = off + 57; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_57(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_59(type, off, ...)   \
    case off + 58: {                            \
        const constexpr type offset = off + 58; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_58(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_60(type, off, ...)   \
    case off + 59: {                            \
        const constexpr type offset = off + 59; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_59(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_61(type, off, ...)   \
    case off + 60: {                            \
        const constexpr type offset = off + 60; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_60(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_62(type, off, ...)   \
    case off + 61: {                            \
        const constexpr type offset = off + 61; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_61(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_63(type, off, ...)   \
    case off + 62: {                            \
        const constexpr type offset = off + 62; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_62(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_64(type, off, ...)   \
    case off + 63: {                            \
        const constexpr type offset = off + 63; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_63(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_65(type, off, ...)   \
    case off + 64: {                            \
        const constexpr type offset = off + 64; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_64(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_66(type, off, ...)   \
    case off + 65: {                            \
        const constexpr type offset = off + 65; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_65(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_67(type, off, ...)   \
    case off + 66: {                            \
        const constexpr type offset = off + 66; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_66(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_68(type, off, ...)   \
    case off + 67: {                            \
        const constexpr type offset = off + 67; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_67(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_69(type, off, ...)   \
    case off + 68: {                            \
        const constexpr type offset = off + 68; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_68(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_70(type, off, ...)   \
    case off + 69: {                            \
        const constexpr type offset = off + 69; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_69(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_71(type, off, ...)   \
    case off + 70: {                            \
        const constexpr type offset = off + 70; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_70(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_72(type, off, ...)   \
    case off + 71: {                            \
        const constexpr type offset = off + 71; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_71(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_73(type, off, ...)   \
    case off + 72: {                            \
        const constexpr type offset = off + 72; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_72(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_74(type, off, ...)   \
    case off + 73: {                            \
        const constexpr type offset = off + 73; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_73(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_75(type, off, ...)   \
    case off + 74: {                            \
        const constexpr type offset = off + 74; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_74(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_76(type, off, ...)   \
    case off + 75: {                            \
        const constexpr type offset = off + 75; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_75(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_77(type, off, ...)   \
    case off + 76: {                            \
        const constexpr type offset = off + 76; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_76(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_78(type, off, ...)   \
    case off + 77: {                            \
        const constexpr type offset = off + 77; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_77(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_79(type, off, ...)   \
    case off + 78: {                            \
        const constexpr type offset = off + 78; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_78(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_80(type, off, ...)   \
    case off + 79: {                            \
        const constexpr type offset = off + 79; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_79(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_81(type, off, ...)   \
    case off + 80: {                            \
        const constexpr type offset = off + 80; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_80(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_82(type, off, ...)   \
    case off + 81: {                            \
        const constexpr type offset = off + 81; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_81(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_83(type, off, ...)   \
    case off + 82: {                            \
        const constexpr type offset = off + 82; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_82(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_84(type, off, ...)   \
    case off + 83: {                            \
        const constexpr type offset = off + 83; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_83(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_85(type, off, ...)   \
    case off + 84: {                            \
        const constexpr type offset = off + 84; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_84(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_86(type, off, ...)   \
    case off + 85: {                            \
        const constexpr type offset = off + 85; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_85(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_87(type, off, ...)   \
    case off + 86: {                            \
        const constexpr type offset = off + 86; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_86(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_88(type, off, ...)   \
    case off + 87: {                            \
        const constexpr type offset = off + 87; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_87(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_89(type, off, ...)   \
    case off + 88: {                            \
        const constexpr type offset = off + 88; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_88(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_90(type, off, ...)   \
    case off + 89: {                            \
        const constexpr type offset = off + 89; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_89(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_91(type, off, ...)   \
    case off + 90: {                            \
        const constexpr type offset = off + 90; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_90(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_92(type, off, ...)   \
    case off + 91: {                            \
        const constexpr type offset = off + 91; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_91(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_93(type, off, ...)   \
    case off + 92: {                            \
        const constexpr type offset = off + 92; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_92(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_94(type, off, ...)   \
    case off + 93: {                            \
        const constexpr type offset = off + 93; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_93(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_95(type, off, ...)   \
    case off + 94: {                            \
        const constexpr type offset = off + 94; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_94(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_96(type, off, ...)   \
    case off + 95: {                            \
        const constexpr type offset = off + 95; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_95(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_97(type, off, ...)   \
    case off + 96: {                            \
        const constexpr type offset = off + 96; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_96(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_98(type, off, ...)   \
    case off + 97: {                            \
        const constexpr type offset = off + 97; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_97(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_99(type, off, ...)   \
    case off + 98: {                            \
        const constexpr type offset = off + 98; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_98(type, off, __VA_ARGS__)
#define REP_CASE_CONSTEXPR_100(type, off, ...)  \
    case off + 99: {                            \
        const constexpr type offset = off + 99; \
        __VA_ARGS__;                            \
    } break;                                    \
        REP_CASE_CONSTEXPR_99(type, off, __VA_ARGS__)

#define REP_CASE_CONSTEXPR(n, type, off, ...) REP_CASE_CONSTEXPR_##n(type, off, __VA_ARGS__)

#endif