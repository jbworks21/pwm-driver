/*
 * app/main.c — Fade demo: sweep PD12 from off to full brightness.
 *
 * Same PD12 LED as the blinky project, but the toggle loop is gone.
 * The timer hardware now generates the waveform; the application only
 * changes the duty cycle. Note that nothing in the main loop blocks
 * while the LED is fading — the CPU is free the entire time.
 *
 * The demo includes a small self-contained UART console (for progress
 * output), same as the ADC project. It is demo scaffolding, not part
 * of the driver.
 *
 * Wiring:
 *   - LED:     PD12 → resistor → LED → GND (built-in on Discovery)
 *   - Serial:  PA2 (TX) → USB-UART adapter RX, GND → GND, 115200 8N1
 */
#include "stm32f407_regs.h"
#include "pwm.h"

/* ============= Demo-only console (identical to ADC project) ============= */

static void console_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;   /* placeholder, no-op for GPIOA */
    /* The console uses PA2/PA3. We add the GPIOA enable the cheap way
     * to keep the diff small — see the ADC project for the full version. */
    RCC->AHB1ENR |= (1U << 0);             /* GPIOAEN */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    volatile uint32_t *gpioa_moder = (volatile uint32_t *)0x40020000UL;
    volatile uint32_t *gpioa_afr0  = (volatile uint32_t *)0x40020020UL;
    *gpioa_moder &= ~((3U << 4) | (3U << 6));
    *gpioa_moder |=  ((2U << 4) | (2U << 6));
    *gpioa_afr0 &= ~((0xFU << 8) | (0xFU << 12));
    *gpioa_afr0 |=  ((0x7U << 8) | (0x7U << 12));

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

static void delay_(volatile uint32_t n) { while (n--) __asm__("nop"); }

/* ============================ Main ============================ */

int main(void) {
    console_init();
    if (pwm_init(1000) != 0) {                 /* 1 kHz */
        puts_("pwm_init failed: frequency not reachable\r\n");
        while (1) { }
    }

    puts_("\r\n=== TIM4 PWM on PD12 — 1 kHz ===\r\n");
    puts_("Period: ");
    print_u32(pwm_get_period() + 1);
    puts_(" counts\r\n\r\n");

    while (1) {
        /* Fade up */
        for (uint8_t d = 0; d <= 100; d += 5) {
            pwm_set_duty(d);
            puts_("duty = ");
            print_u32(d);
            puts_("%\r\n");
            delay_(600000);
        }
        /* Fade down */
        for (int d = 100; d >= 0; d -= 5) {
            pwm_set_duty((uint8_t)d);
            puts_("duty = ");
            print_u32((uint32_t)d);
            puts_("%\r\n");
            delay_(600000);
        }
    }
}
