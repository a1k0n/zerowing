#include <stdint.h>

#define F_CPU           2000000UL
#define _SFR_(mem_addr) (*(volatile uint8_t *)(0x5000 + (mem_addr)))

/* PORT A */
#define PA_ODR _SFR_(0x00)
#define PA_IDR _SFR_(0x01)
#define PA_DDR _SFR_(0x02)
#define PA_CR1 _SFR_(0x03)
#define PA_CR2 _SFR_(0x04)

#define BLUE_LED  1
#define GREEN_LED 2

/* TIM4 */
#define TIM4_CR1  _SFR_(0x340)
#define TIM4_IER  _SFR_(0x343)
#define TIM4_SR   _SFR_(0x344)
#define TIM4_EGR  _SFR_(0x345)
#define TIM4_CNTR _SFR_(0x346)
#define TIM4_PSCR _SFR_(0x347)
#define TIM4_ARR  _SFR_(0x348)

/* UART */
#define UART_SR   _SFR_(0x230)
#define UART_TXE  7
#define UART_TC   6
#define UART_RXNE 5
#define UART_DR   _SFR_(0x231)
#define UART_BRR1 _SFR_(0x232)
#define UART_BRR2 _SFR_(0x233)
#define UART_CR1  _SFR_(0x234)
#define UART_CR2  _SFR_(0x235)
#define UART_TEN  3
#define UART_REN  2

#define TIM4_ISR 23

void timer_isr(void) __interrupt(TIM4_ISR) {
  PA_ODR ^= (1 << BLUE_LED);
  TIM4_SR &= ~1;
}

static inline void delay_ms(uint16_t ms) {
  uint32_t i;
  for (i = 0; i < ((F_CPU / 18000UL) * ms); i++) __asm__("nop");
}

int main() {
  PA_DDR |= (1 << GREEN_LED) | (1 << BLUE_LED);  // configure PD4 as output
  PA_CR1 |= (1 << GREEN_LED) | (1 << BLUE_LED);  // push-pull mode

  /* Prescaler = 128 */
  TIM4_PSCR = 0b00000111;
  /* Period ~= 30Hz */
  TIM4_ARR = 255;
  TIM4_IER |= 1;  // Enable Update Interrupt
  TIM4_CR1 |= 1;  // Enable TIM4

  while (1) {
    /* toggle pin every 250ms */
    PA_ODR ^= (1 << GREEN_LED);
    delay_ms(250);
  }
}
