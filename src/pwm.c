/*
 * pwm.c — TIM4_CH1 PWM driver on PD12 (STM32F407, no interrupts).
 *
 * The timer works like this:
 *
 *     CNT:  0 1 2 3 ... ARR 0 1 2 ...  (wraps at ARR, i.e. period = ARR+1)
 *     CCR:  ^^^^^^^^^^^                   pin HIGH while CNT < CCR
 *                              vvvvvvvv    pin LOW  while CNT >= CCR
 *
 * "PWM mode 1" means active while CNT < CCR. "Active" = high by default.
 * The ratio CCR / (ARR+1) is the duty cycle.
 *
 * This file's trick is that all the PSC/ARR numbers are computed from
 * the requested frequency, so the caller never sees the timer's guts.
 */
#include "stm32f407_regs.h"
#include "pwm.h"

/* ---- Wiring, kept together so it's easy to port ------------------- */
#define PWM_GPIO        GPIOD
#define PWM_RCC_GPIOEN  RCC_AHB1ENR_GPIODEN
#define PWM_RCC_TIMEN   RCC_APB1ENR_TIM4EN
#define PWM_TIM         TIM4
#define PWM_PIN         12
#define PWM_AF          2       /* TIM4 is AF2 on the F407 */

/* Timer clock, in Hz. TIM4 is on APB1. After reset APB1 = HSI = 16 MHz
 * (the APB1 prescaler is 1, so the timer does not get doubled). If you
 * later configure the PLL to 168 MHz, APB1 becomes 42 MHz but the timer
 * clock becomes 84 MHz (APB1 × 2 when the APB1 prescaler > 1). Update
 * this constant accordingly, or every PWM frequency will be wrong. */
#define PWM_TIMER_CLK_HZ  16000000UL

/* Target resolution: how many counts per period. 1000 gives 0.1% duty
 * resolution, which is invisible flicker-free for an LED. For higher
 * frequencies this drops — the caller's frequency determines it. */
#define PWM_RESOLUTION    1000U

static uint16_t s_period;   /* value of ARR = PWM_RESOLUTION - 1 */

int pwm_init(uint32_t freq_hz) {
    if (freq_hz == 0) return -1;

    /* Compute the timer clock divisor for the requested frequency:
     *     f = fCK / ((PSC+1) * (ARR+1))
     *
     * We fix ARR+1 = PWM_RESOLUTION to lock the duty resolution, then
     * solve for PSC+1. Check the result is a whole number and fits a
     * 16-bit PSC register — if not, the frequency is unreachable. */
    uint32_t div = PWM_TIMER_CLK_HZ / (freq_hz * PWM_RESOLUTION);
    uint32_t real_f = PWM_TIMER_CLK_HZ / (div * PWM_RESOLUTION);
    if (div == 0 || div > 0x10000UL) return -1;
    /* Reject frequencies we can't hit within 1% — silent rounding is
     * exactly the kind of thing that bites later. */
    if (real_f < freq_hz * 99 / 100 || real_f > freq_hz * 101 / 100) return -1;

    /* ---- Step 1: clocks -------------------------------------------- */
    RCC->AHB1ENR |= PWM_RCC_GPIOEN;
    RCC->APB1ENR |= PWM_RCC_TIMEN;

    /* ---- Step 2: pin as alternate function -------------------------
     * MODER = 10 (alternate), AFR = 2 (TIM4). Same pattern as UART. */
    PWM_GPIO->MODER &= ~(3U << (PWM_PIN * 2));
    PWM_GPIO->MODER |=  (2U << (PWM_PIN * 2));
    /* Pin 12 lives in AFR[1]; field offset within that register is
     * (pin - 8) * 4 = 16 bits. */
    uint32_t afr_idx = (PWM_PIN >= 8) ? 1U : 0U;
    uint32_t afr_shift = (PWM_PIN - (afr_idx * 8)) * 4;
    PWM_GPIO->AFR[afr_idx] &= ~(0xFU << afr_shift);
    PWM_GPIO->AFR[afr_idx] |=  ((uint32_t)PWM_AF << afr_shift);

    /* ---- Step 3: timer base ---------------------------------------
     * PSC and ARR are "value minus one" because the counter wraps *at*
     * the value: writing PSC=15 divides the clock by 16, and ARR=999
     * gives 1000 counts per period. Forgetting the -1 is the classic
     * off-by-one that makes frequencies land on 999 Hz instead of 1 kHz.
     *
     * ARPE = 1 double-buffers ARR: writes to ARR take effect at the next
     * update event, not immediately. This avoids glitches when you
     * change frequency on a running timer. For our fixed frequency it's
     * unnecessary, but it's good discipline and costs nothing.
     */
    PWM_TIM->PSC = (uint16_t)(div - 1);
    PWM_TIM->ARR = (uint16_t)(PWM_RESOLUTION - 1);
    PWM_TIM->CR1 |= TIM_CR1_ARPE;

    /* Force the shadow registers to load right now. Without UG the very
     * first period uses the timer's power-on defaults (PSC=0, ARR=0xFFFF)
     * before the values above take effect — a visible glitch. */
    PWM_TIM->EGR = TIM_EGR_UG;

    /* ---- Step 4: configure channel 1 for PWM mode 1 ----------------
     * OC1M = 6 (PWM mode 1): pin is HIGH while CNT < CCR1, LOW otherwise.
     * OC1PE = 1 preloads CCR1: writes to CCR1 take effect at the next
     * update event, so mid-period changes can't produce a half-tick
     * glitch on the output. */
    PWM_TIM->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;

    /* CC1E = channel 1 output enable: hands the pin to the timer. */
    PWM_TIM->CCER = TIM_CCER_CC1E;

    /* Start at 0% so the LED is off until someone calls set_duty. */
    PWM_TIM->CCR1 = 0;

    /* ---- Step 5: start the counter -------------------------------- */
    PWM_TIM->CR1 |= TIM_CR1_CEN;

    s_period = (uint16_t)(PWM_RESOLUTION - 1);
    return 0;
}

void pwm_set_duty(uint8_t percent) {
    if (percent > 100) percent = 100;
    /* 100% duty is *not* CCR = period+1 (that's outside range) — it's
     * CCR > ARR. Setting CCR = ARR+1 would overflow for a 16-bit CCR.
     * Instead, treat 100% as "always above CCR" — but with PWM mode 1
     * the pin is HIGH while CNT < CCR, so 100% needs CCR > ARR. We clamp
     * CCR to ARR for 100% and rely on the timer's behavior: at CCR=ARR,
     * the pin is HIGH for CNT=0..ARR-1 and LOW only on the last tick,
     * which is 99.9% duty. Close enough for an LED. For a true 100%,
     * the correct fix is to disable the channel and drive the pin high
     * manually — which is exactly what you'd do for a motor driver. */
    uint32_t ccr = ((uint32_t)percent * s_period) / 100U;
    PWM_TIM->CCR1 = (uint16_t)ccr;
}

void pwm_set_raw(uint16_t value) {
    if (value > s_period) value = s_period;
    PWM_TIM->CCR1 = value;
}

uint16_t pwm_get_period(void) {
    return s_period;
}

/* ================================================================
 * Timer interrupt support.
 *
 * The chain that makes an ISR actually fire:
 *
 *   TIM4->DIER.UIE = 1     ← timer raises the IRQ line on update
 *   NVIC->ISER[0].30 = 1   ← NVIC forwards IRQ 30 to the CPU
 *   vector[46] = TIM4_IRQHandler  ← CPU jumps to our ISR
 *   ISR clears TIM4->SR.UIF ← stop the re-entry
 *
 * Any one of those missing and nothing happens. Debugging a "my
 * interrupt doesn't fire" problem is almost always a hunt for
 * which of the four steps was skipped.
 * ================================================================ */

static volatile pwm_update_cb_t s_callback    = 0;
static volatile uint32_t        s_update_count = 0;

void pwm_enable_update_irq(pwm_update_cb_t cb) {
    s_callback     = cb;
    s_update_count = 0;

    /* Clear any stale UIF before enabling. If the flag was set by an
     * earlier update (which it will be, since the timer has been
     * running), the ISR would fire the instant NVIC enable lands —
     * before the callback is installed. Belt-and-braces. */
    PWM_TIM->SR = ~TIM_SR_UIF;

    /* Step 1: peripheral-side enable. */
    PWM_TIM->DIER |= TIM_DIER_UIE;

    /* Step 2: NVIC-side enable. This is the step SysTick skipped. */
    NVIC_ISER0 = (1U << TIM4_IRQN);
}

void pwm_disable_update_irq(void) {
    NVIC_ICER0 = (1U << TIM4_IRQN);
    PWM_TIM->DIER &= ~TIM_DIER_UIE;
    s_callback = 0;
}

uint32_t pwm_get_update_count(void) {
    return s_update_count;
}

/*
 * TIM4_IRQHandler — the ISR that fires on every PWM update event.
 *
 * `startup.c` declares this as a weak alias to Default_Handler, so
 * the vector table has a valid pointer before this file is linked.
 * This strong definition overrides the alias at link time, and the
 * vector table's slot 46 automatically points here. That's the whole
 * trick of the weak-alias pattern: no manual editing of the vector
 * table when you add an ISR.
 *
 * Note the ISR is *four instructions* of actual work. This is not an
 * accident — the shorter the ISR, the less it delays other interrupts
 * and the main loop. Heavy work belongs in the callback, or better,
 * outside interrupt context entirely.
 */
void TIM4_IRQHandler(void) {
    /* Clear UIF. The `= ~TIM_SR_UIF` idiom writes 0 to bit 0 (clears
     * it) and 1 to all other bits (which are rc_w0, so writing 1 is a
     * no-op — they keep their current state). This is the standard
     * STM32 way to clear a single flag without a read-modify-write
     * race. */
    PWM_TIM->SR = ~TIM_SR_UIF;

    s_update_count++;

    /* Tail-call to the registered callback. Null check in case the
     * timer still has a pending IRQ after pwm_disable_update_irq
     * cleared s_callback but before ICER took effect. */
    if (s_callback) s_callback();
}
