#ifndef LOGGING_HPP
#define LOGGING_HPP

//#define DEBUG_CLI
#ifdef DEBUG_CLI
//#define DUMP_CPU_STATE
#endif
// #define DEBUG_ALL

// #define LEGACY_RENDERING

#ifdef DEBUG_ALL
#define DEBUG_DMA
#define DEBUG_IRQ
#define DEBUG_TIM
#define DEBUG_LCD
#define DEBUG_MEM
#define DEBUG_IO
#define DEBUG_SAVE
#define DEBUG_SWI
#endif

#ifdef DEBUG_DMA
#define LOG_DMA(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_DMA(body) \
    do {              \
    } while (0)
#endif

#ifdef DEBUG_IRQ
#define LOG_IRQ(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_IRQ(body) \
    do {              \
    } while (0)
#endif

#ifdef DEBUG_TIM
#define LOG_TIM(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_TIM(body) \
    do {              \
    } while (0)
#endif

#ifdef DEBUG_LCD
#define LOG_LCD(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_LCD(body) \
    do {              \
    } while (0)
#endif

#ifdef DEBUG_MEM
#define LOG_MEM(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_MEM(body) \
    do {              \
    } while (0)
#endif

#ifdef DEBUG_IO
#define LOG_IO(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_IO(body) \
    do {              \
    } while (0)
#endif

#ifdef DEBUG_SAVE
#define LOG_SAVE(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_SAVE(body) \
    do {              \
    } while (0)
#endif

#ifdef DEBUG_SWI
#define LOG_SWI(body) \
    do {              \
        body          \
    } while (0)
#else
#define LOG_SWI(body) \
    do {              \
    } while (0)
#endif

#endif
