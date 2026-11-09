/*
 * stm32f407_regs.h — Register map for the STM32F407.
 *
 * Same technique as every prior project: cast a base address to a struct
 * of volatile uint32_t fields. This version adds TIM4 (the PWM timer)
 * to the set of peripherals from the ADC project. Everything else is
 * identical — which is exactly the point of this file: as you add
 * peripherals, you add structs, not rewrite anything.
 */
#ifndef STM32F407_REGS_H
#define STM32F407_REGS_H

#include <stdint.h>

#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)

#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)
#define GPIOD_BASE          (AHB1PERIPH_BASE + 0x0C00UL)   /* 0x40020C00 */
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)
#define TIM4_BASE           (APB1PERIPH_BASE + 0x0800UL)   /* 0x40000800 */

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

/*
 * General-purpose timer (TIM2..TIM5 layout, all identical).
 *
 * The three registers that define the timing:
 *   PSC — prescaler: divides the input clock before it reaches CNT
 *   ARR — auto-reload: CNT counts from 0 to ARR, then wraps to 0
 *   CNT — the live counter value
 *
 * The four capture/compare registers (CCR1..CCR4) each control one
 * output channel. In PWM mode, the timer compares CNT against CCRx on
 * every tick and toggles the corresponding pin accordingly.
 */
typedef struct {
    volatile uint32_t CR1;    /* 0x00 control 1: CEN, ARPE, ... */
    volatile uint32_t CR2;    /* 0x04 control 2 */
    volatile uint32_t SMCR;   /* 0x08 slave mode */
    volatile uint32_t DIER;   /* 0x0C DMA/interrupt enable */
    volatile uint32_t SR;     /* 0x10 status (UIF flag) */
    volatile uint32_t EGR;    /* 0x14 event generation (UG) */
    volatile uint32_t CCMR1;  /* 0x18 capture/compare mode, channels 1-2 */
    volatile uint32_t CCMR2;  /* 0x1C capture/compare mode, channels 3-4 */
    volatile uint32_t CCER;   /* 0x20 capture/compare enable */
    volatile uint32_t CNT;    /* 0x24 counter */
    volatile uint32_t PSC;    /* 0x28 prescaler */
    volatile uint32_t ARR;    /* 0x2C auto-reload */
    volatile uint32_t RCR;    /* 0x30 repetition counter */
    volatile uint32_t CCR1;   /* 0x34 capture/compare 1 */
    volatile uint32_t CCR2;   /* 0x38 */
    volatile uint32_t CCR3;   /* 0x3C */
    volatile uint32_t CCR4;   /* 0x40 */
    volatile uint32_t BDTR;   /* 0x44 break/dead-time (only TIM1/8 use it) */
    volatile uint32_t DCR;    /* 0x48 DMA control */
    volatile uint32_t DMAR;   /* 0x4C DMA address */
} TIM_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t          RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t          RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t          RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

#define RCC     ((RCC_TypeDef *)RCC_BASE)
#define GPIOD   ((GPIO_TypeDef *)GPIOD_BASE)
#define USART2  ((USART_TypeDef *)USART2_BASE)
#define TIM4    ((TIM_TypeDef *)TIM4_BASE)

/* RCC enables */
#define RCC_AHB1ENR_GPIODEN   (1U << 3)
#define RCC_APB1ENR_USART2EN  (1U << 17)
#define RCC_APB1ENR_TIM4EN    (1U << 2)      /* TIM4 is on APB1 */

/* TIM CR1 */
#define TIM_CR1_CEN           (1U << 0)      /* counter enable */
#define TIM_CR1_ARPE          (1U << 7)      /* auto-reload preload enable */

/* TIM CCMR1 (channels 1 and 2) */
#define TIM_CCMR1_OC1PE       (1U << 3)      /* CCR1 preload enable */
#define TIM_CCMR1_OC1M_PWM1   (6U << 4)      /* PWM mode 1 (active while CNT < CCR1) */
#define TIM_CCMR1_OC1M_PWM2   (7U << 4)      /* PWM mode 2 (inverted) */

/* TIM CCER */
#define TIM_CCER_CC1E         (1U << 0)      /* channel 1 output enable */

/* TIM EGR */
#define TIM_EGR_UG            (1U << 0)      /* update generation (force load) */

/* USART (for the demo console) */
#define USART_SR_TXE          (1U << 7)
#define USART_CR1_UE          (1U << 13)
#define USART_CR1_TE          (1U << 3)

#endif /* STM32F407_REGS_H */
