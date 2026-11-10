/*
 * app/main.c — Fade driven by TIM4 update ISR at 1 kHz.
 *
 * The whole fade now lives in the ISR (via the callback), not in the
 * main loop. main is free to do other work; in this demo it just
 * prints status changes and spins, but the printing is *reactive* —
 * it only fires when the ISR has advanced the fade by a step.
 *
 * Contrast with the previous PWM demo: that one had a busy-wait
 * `delay_()` that blocked the CPU between every duty change. Here
 * main never blocks, never calls a delay, and never sets the duty
 * directly — it just observes.
 */
#include "stm32f407_regs.h"
#include "pwm.h"

/* ============= Demo console (unchanged from prior projects) ======== */

static void console_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIOA->MODER &= ~((3U << 4) | (3U << 6));
    GPIOA->MODER |=  ((2U << 4) | (2U << 6));
    GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA->AFR[0] |=  ((0x7U << 8) | (0x7U << 12));

    USART2->BRR = (16000000UL + 115200U / 2) / 115200U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void putc_(char c) {
    while (!(USART2->SR & USART_SR_TXE)) { }
    USART2->DR = (uint8_t)c;
}
static void puts_(const char *s) { while (*s) putc_(*s++); }

static void print_u32(uint32_t v) {
    char buf[10];
    int i = 0;
    if (v == 0) { putc_('0'); return; }
    while (v > 0) { buf[i++] = (char)('0' + (v % 10U)); v /= 10U; }
    while (i--) putc_(buf[i]);
}

/* ============= Fade state, shared with the ISR ======================
 *
 * Every variable the ISR writes and main reads must be `volatile`.
 * Without it, the compiler is allowed to cache the value in a
 * register and never reload from memory — main would see the initial
 * value forever while the ISR happily updated it in memory.
 *
 * On a 32-bit CPU, reads and writes of uint8_t/int8_t are atomic, so
 * no critical sections are needed for single-byte access. If you
 * added a multi-word shared struct, you'd need to disable interrupts
 * briefly around the read (or use a seqlock). */

/* How many update events between fade steps. At 1 kHz, 40 events =
 * 40 ms per step, so a full 0→100 sweep takes ~4 seconds. */
#define TICKS_PER_STEP  40

static volatile uint8_t  s_duty_pct     = 0;
static volatile int8_t   s_direction    = 1;   /* +1 up, -1 down */
static volatile uint16_t s_tick_counter = 0;
static volatile uint32_t s_step_count   = 0;   /* increments on each fade step */

/*
 * fade_step — called from TIM4_IRQHandler at 1 kHz.
 *
 * Runs *inside interrupt context*. Must be fast: no serial, no
 * busy-waits, no long loops. The body here is under a dozen
 * instructions — that's the right budget for an ISR callback.
 *
 * Uses pwm_set_duty rather than pwm_set_raw so the fade is expressed
 * in whole percent. The preload bit (OC1PE) means the CCR1 write
 * takes effect at the next update event — no glitch even though we're
 * modifying the register mid-period.
 */
static void fade_step(void) {
    s_tick_counter++;
    if (s_tick_counter < TICKS_PER_STEP) return;
    s_tick_counter = 0;

    int16_t next = (int16_t)s_duty_pct + s_direction;
    if (next >= 100) { next = 100; s_direction = -1; }
    if (next <= 0)   { next = 0;   s_direction = 1; }

    s_duty_pct = (uint8_t)next;
    pwm_set_duty(s_duty_pct);
    s_step_count++;
}

/* ============================ Main ============================ */

int main(void) {
    console_init();

    if (pwm_init(1000) != 0) {                  /* 1 kHz PWM */
        puts_("pwm_init failed\r\n");
        while (1) { }
    }

    puts_("\r\n=== TIM4 update interrupt demo ===\r\n");
    puts_("Fade is driven by the ISR at 1 kHz.\r\n");
    puts_("main does nothing but react.\r\n\r\n");

    pwm_enable_update_irq(fade_step);

    /* main loop: purely reactive. It prints when s_step_count changes,
     * and does "other work" in between. Nothing here drives the fade —
     * the ISR does. Main is completely decoupled from the timing. */
    uint32_t last_step = 0;
    uint32_t main_work = 0;
    while (1) {
        uint32_t current = s_step_count;
        if (current != last_step) {
            last_step = current;
            puts_("duty = ");
            print_u32(s_duty_pct);
            puts_("%   updates = ");
            print_u32(pwm_get_update_count());
            puts_("   main_work = ");
            print_u32(main_work);
            puts_("\r\n");
            main_work = 0;   /* reset per print so you can see progress */
        }
        main_work++;
    }
}
