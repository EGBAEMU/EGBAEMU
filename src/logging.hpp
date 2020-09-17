#ifndef LOGGING_HPP
#define LOGGING_HPP

#ifdef DEBUG_ALL
#define DEBUG_DMA
#define DEBUG_IRQ
#define DEBUG_TIM
#define DEBUG_LCD
#define DEBUG_MEM
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

#define DEBUG_SWI
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