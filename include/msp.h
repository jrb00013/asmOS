#ifndef MSP_H
#define MSP_H

#include <stdint.h>  // For standard integer types
#include <stddef.h>  // For size_t

// Memory-mapped I/O macros 
#define MMIO32(addr) (*(volatile uint32_t *)(addr))
#define MMIO16(addr) (*(volatile uint16_t *)(addr))
#define MMIO8(addr)  (*(volatile uint8_t *)(addr))


#define UART_BASE       0x40001000 
#define TIMER_BASE      0x40002000 

// System functions
void disable_interrupts(void);
void enable_interrupts(void);
void halt_system(void);

#endif // MSP_H
