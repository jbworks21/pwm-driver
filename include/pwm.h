/*
 * pwm.h — Public API of the TIM4 PWM driver (STM32F407).
 *
 * Scope: one channel (TIM4_CH1 on PD12), one frequency, software-updated
 * duty cycle. No complementary outputs, no dead-time insertion, no
 * input capture, no interrupts. Those come later, once this works.
 *
 * Design choice: the driver takes a frequency in Hz and computes PSC
 * and ARR for you. The alternative — exposing raw PSC/ARR/CCR values —
 * would be more flexible but forces every caller to redo the same
 * arithmetic. This API matches 90% of real use.
 */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/*
 * Configure PD12 as a PWM output at the requested frequency.
 *
 * freq_hz: target frequency. The achievable value depends on the
 *          timer clock (16 MHz here) and the resolution you need.
 *          For an LED, 1 kHz is ideal — 1000 steps of duty.
 *          For a motor, 10-20 kHz keeps it above audible range.
 *          For a hobby servo, use 50 Hz (see README).
 *
 * Returns 0 on success, -1 if the frequency is not achievable with
 * the timer clock.
 */
int pwm_init(uint32_t freq_hz);

/*
 * Set the duty cycle, 0..100.
 * Values above 100 are clamped to 100.
 * The change takes effect at the next timer update — no glitches.
 */
void pwm_set_duty(uint8_t percent);

/*
 * Set the duty cycle in raw counts, 0..pwm_get_period().
 * Useful when you want finer resolution than whole percent, or when
 * you're computing the value from some other quantity.
 */
void pwm_set_raw(uint16_t value);

/* Returns the value of ARR, i.e. the number of counts in one period. */
uint16_t pwm_get_period(void);

#endif /* PWM_H */
