/*
 * startup.c — Vector table and Reset_Handler for STM32F407.
 *
 * See the previous projects for the full explanation of the vector
 * table's first 16 entries. This version extends the table to include
 * peripheral IRQs, so that TIM4 (IRQ 30, vector slot 46) can fire.
 *
 * Index arithmetic: peripheral IRQ N is at vector slot (16 + N).
 * TIM4_IRQn = 30, so 16 + 30 = 46. That's the whole rule. The F407
 * has 82 IRQs total, so a fully populated table would be 98 entries;
 * we only need up to slot 46 here, because no IRQ above TIM4 is
 * enabled. Extending further is trivial — add more Default_Handler
 * lines.
 *
 * The weak-alias pattern:
 *   - Every handler symbol is defined as a weak alias to Default_Handler
 *   - A strong definition elsewhere (e.g. pwm.c's TIM4_IRQHandler)
 *     overrides the alias at link time
 *   - The vector table then points at the strong version automatically
 *   - If you never write a strong version, the alias fires and loops
 *     in Default_Handler — obvious in a debugger, harmless otherwise
 */
#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;
extern int main(void);

void Reset_Handler(void);

void Default_Handler(void) {
    while (1) { }
}

/* ---- Weak aliases: system exceptions ------------------------------ */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* ---- Weak alias: TIM4 peripheral IRQ ------------------------------
 * Overridden by pwm.c's strong definition. If pwm.c ever removes
 * that function, the vector table silently reverts to this alias,
 * and an enabled TIM4 IRQ would hang in Default_Handler — which is
 * exactly the bug you want to be loud, so keep pwm.c and its ISR
 * together. */
void TIM4_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));

/*
 * The vector table. `__attribute__((section(".isr_vector")))` places
 * this array in the section the linker script reserves for it, mapped
 * to the very start of Flash (0x08000000).
 */
__attribute__((section(".isr_vector")))
void (*const g_pfnVectors[])(void) = {

    /* ---- 0-15: ARM system exceptions ---------------------------- */
    (void (*)(void))(&_estack),   /*  0: initial stack pointer */
    Reset_Handler,                /*  1: reset */
    NMI_Handler,                  /*  2 */
    HardFault_Handler,            /*  3 */
    MemManage_Handler,            /*  4 */
    BusFault_Handler,             /*  5 */
    UsageFault_Handler,           /*  6 */
    0,                            /*  7: reserved */
    0,                            /*  8: reserved */
    0,                            /*  9: reserved */
    0,                            /* 10: reserved */
    SVC_Handler,                  /* 11 */
    DebugMon_Handler,             /* 12 */
    0,                            /* 13: reserved */
    PendSV_Handler,               /* 14 */
    SysTick_Handler,              /* 15 */

    /* ---- 16-46: peripheral IRQs, index = 16 + IRQn --------------
     * Only TIM4 (IRQ 30) is used in this project. Every other entry
     * points at Default_Handler so that if a stray IRQ ever fires
     * (e.g. from a misconfigured peripheral), we loop in a known
     * place instead of jumping into random Flash. */
    Default_Handler,   /* 16: IRQ0  WWDG */
    Default_Handler,   /* 17: IRQ1  PVD */
    Default_Handler,   /* 18: IRQ2  TAMP_STAMP */
    Default_Handler,   /* 19: IRQ3  RTC_WKUP */
    Default_Handler,   /* 20: IRQ4  FLASH */
    Default_Handler,   /* 21: IRQ5  RCC */
    Default_Handler,   /* 22: IRQ6  EXTI0 */
    Default_Handler,   /* 23: IRQ7  EXTI1 */
    Default_Handler,   /* 24: IRQ8  EXTI2 */
    Default_Handler,   /* 25: IRQ9  EXTI3 */
    Default_Handler,   /* 26: IRQ10 EXTI4 */
    Default_Handler,   /* 27: IRQ11 DMA1_Stream0 */
    Default_Handler,   /* 28: IRQ12 DMA1_Stream1 */
    Default_Handler,   /* 29: IRQ13 DMA1_Stream2 */
    Default_Handler,   /* 30: IRQ14 DMA1_Stream3 */
    Default_Handler,   /* 31: IRQ15 DMA1_Stream4 */
    Default_Handler,   /* 32: IRQ16 DMA1_Stream5 */
    Default_Handler,   /* 33: IRQ17 DMA1_Stream6 */
    Default_Handler,   /* 34: IRQ18 ADC */
    Default_Handler,   /* 35: IRQ19 CAN1_TX */
    Default_Handler,   /* 36: IRQ20 CAN1_RX0 */
    Default_Handler,   /* 37: IRQ21 CAN1_RX1 */
    Default_Handler,   /* 38: IRQ22 CAN1_SCE */
    Default_Handler,   /* 39: IRQ23 EXTI9_5 */
    Default_Handler,   /* 40: IRQ24 TIM1_BRK_TIM9 */
    Default_Handler,   /* 41: IRQ25 TIM1_UP_TIM10 */
    Default_Handler,   /* 42: IRQ26 TIM1_TRG_COM_TIM11 */
    Default_Handler,   /* 43: IRQ27 TIM1_CC */
    Default_Handler,   /* 44: IRQ28 TIM2 */
    Default_Handler,   /* 45: IRQ29 TIM3 */
    TIM4_IRQHandler,   /* 46: IRQ30 TIM4 ← the one we use */
};

void Reset_Handler(void) {
    uint32_t *pSrc  = &_sidata;
    uint32_t *pDest = &_sdata;
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }

    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }

    main();
    while (1) { }
}
