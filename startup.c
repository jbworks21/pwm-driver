// startup.c - Minimal startup code for ARM Cortex-M
#include <stdint.h>

// Symbols defined in the linker script
extern uint32_t _sidata; // Start of .data in flash
extern uint32_t _sdata;  // Start of .data in RAM
extern uint32_t _edata;  // End of .data in RAM
extern uint32_t _sbss;   // Start of .bss
extern uint32_t _ebss;   // End of .bss
extern uint32_t _estack; // End of stack

// The Reset_Handler function is defined in main.c
extern int main(void);
extern void Reset_Handler(void);

// The vector table: an array of function pointers.
// The first entry is the initial stack pointer.
// The second entry is the reset handler.
__attribute__((section(".isr_vector")))
void (*const g_pfnVectors[])(void) = {
    (void (*)(void))(&_estack), // Initial Stack Pointer
    Reset_Handler,              // Reset Handler
};

// Reset_Handler: The first C function to run
void Reset_Handler(void) {
    // 1. Copy initialized data from Flash to RAM
    uint32_t *pSrc = &_sidata;
    uint32_t *pDest = &_sdata;
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }

    // 2. Zero the .bss section (uninitialized data)
    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }

    // 3. Call the main application
    main();

    // 4. If main ever returns, loop forever
    while (1);
}
