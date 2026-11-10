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

/* ================================================================
 * Timer interrupt API (added in this project).
 * ================================================================ */

/*
 * Callback signature for the update interrupt.
 *
 * The driver owns TIM4_IRQHandler (it's defined in pwm.c). The ISR
 * does the minimum possible work — clear UIF, call this callback —
 * and your code runs inside the callback, still in interrupt context.
 *
 * The callback runs at the PWM update frequency (1 kHz by default),
 * so it must be *fast*. No blocking, no serial printing, no long
 * loops. The whole point of this pattern is that the ISR stays tiny.
 */
typedef void (*pwm_update_cb_t)(void);

/*
 * Enable the update interrupt. Sets DIER.UIE in TIM4 and enables
 * IRQ 30 in the NVIC. The callback fires on every update event.
 *
 * Calling this again replaces the previous callback.
 */
void pwm_enable_update_irq(pwm_update_cb_t cb);

/* Disable both the peripheral source and the NVIC enable. */
void pwm_disable_update_irq(void);

/* Returns the number of update events that have fired since the last
 * call to pwm_enable_update_irq. Useful for verification: at 1 kHz,
 * this should advance ~1000 per second. */
uint32_t pwm_get_update_count(void);
