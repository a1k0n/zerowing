#include <stdint.h>
#include "stm8.h"

#define F_CPU           2000000UL

#define BLUE_LED  1
#define GREEN_LED 2

void TIM4_ovf(void) __interrupt(TIM4_OVR_UIF_IRQ) {
  PA_ODR ^= (1 << GREEN_LED);
  TIM4_SR &= ~TIM_SR1_UIF;
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
  TIM4_ARR = 250;
  TIM4_IER |= TIM_IER_UIE;  // Enable Update Interrupt
  TIM4_CR1 |= TIM_CR1_CEN;  // Enable TIM4

  rim();
  while (1) {
    /* toggle pin every 250ms */
    // wfi();
    delay_ms(250);
    PA_ODR ^= (1 << BLUE_LED);
  }
}
