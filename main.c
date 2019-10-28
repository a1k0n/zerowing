#include <stdint.h>
#define F_CPU 2000000UL
#define _SFR_(mem_addr)     (*(volatile uint8_t *)(0x5000 + (mem_addr)))
/* PORT A */
#define PA_ODR      _SFR_(0x00)
#define PA_IDR      _SFR_(0x01)
#define PA_DDR      _SFR_(0x02)
#define PA_CR1      _SFR_(0x03)
#define PA_CR2      _SFR_(0x04)

#define BLUE_LED 1
#define GREEN_LED 2

/* UART */
#define UART_SR     _SFR_(0x230)
#define UART_TXE    7
#define UART_TC     6
#define UART_RXNE   5
#define UART_DR     _SFR_(0x231)
#define UART_BRR1   _SFR_(0x232)
#define UART_BRR2   _SFR_(0x233)
#define UART_CR1    _SFR_(0x234)
#define UART_CR2    _SFR_(0x235)
#define UART_TEN    3
#define UART_REN    2


static inline void delay_ms(uint16_t ms) {
  uint32_t i;
  for (i = 0; i < ((F_CPU / 18000UL) * ms); i++)
    __asm__("nop");
}


int main() {
  PA_DDR |= (1 << GREEN_LED); // configure PD4 as output
  PA_CR1 |= (1 << GREEN_LED); // push-pull mode
  while (1) {
    /* toggle pin every 250ms */
    PA_ODR ^= (1 << GREEN_LED);
    delay_ms(250);
  }
}
