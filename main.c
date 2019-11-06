#include <stdint.h>
#include "stm8.h"

#define F_CPU           2000000UL

#define BLUE_LED  1
#define GREEN_LED 2

#define PWM_PERIOD 3000
#define SERVO_PWM_PERIOD 20000

static void deadtime_delay() {
  // delay 50us, each loop is 3 instrs at 2MHz, minus the overhead of the call itself
  volatile uint8_t i = 33 - 3;
  while(--i);
}

// FIXME!
// need to wait 50us after changing directions before setting nonzero PWM period
static void set_hbridge_brake(uint16_t period) {
  if (PB_ODR != 0x30) {
    TIM1_CCR1H = 0;  // HLR (M+)
    TIM1_CCR1L = 0;
    TIM1_CCR2H = 0;  // HLL (M-)
    TIM1_CCR2L = 0;
    PB_ODR = 0x30;  // HUR high (off) HUL high (off)
    deadtime_delay();
  }
  TIM1_CCR1H = period >> 8;  // HLR (M+)
  TIM1_CCR1L = period & 255;
  TIM1_CCR2H = PWM_PERIOD >> 8;  // HLL (M-)
  TIM1_CCR2L = PWM_PERIOD & 255;
}

static void set_hbridge_fwd(uint16_t period) {
  if (PB_ODR != 0x10) {
    TIM1_CCR1H = 0;  // HLR (M+)
    TIM1_CCR1L = 0;
    TIM1_CCR2H = 0;  // HLL (M-)
    TIM1_CCR2L = 0;
    PB_ODR = 0x10;  // HUR low (on) HUL high (off)
    deadtime_delay();
  }
  TIM1_CCR1H = 0;  // HLR (M+)
  TIM1_CCR1L = 0;
  TIM1_CCR2H = period >> 8;  // HLL (M-)
  TIM1_CCR2L = period & 255;
}

static void set_hbridge_rev(uint16_t period) {
  if (PB_ODR != 0x20) {
    TIM1_CCR1H = 0;  // HLR (M+)
    TIM1_CCR1L = 0;
    TIM1_CCR2H = 0;  // HLL (M-)
    TIM1_CCR2L = 0;
    PB_ODR = 0x20;  // HUR high (off) HUL low (on)
    deadtime_delay();
  }
  TIM1_CCR1H = period >> 8;  // HLR (M+)
  TIM1_CCR1L = period & 255;
  TIM1_CCR2H = 0;  // HLL (M-)
  TIM1_CCR2L = 0;
}

static void set_ch1_period(uint16_t us) {
  TIM1_CCR1H = (us<<1)>>8;
  TIM1_CCR1L = (us<<1) & 255;
}

static void set_ch2_period(uint16_t us) {
  TIM1_CCR2H = (us<<1)>>8;
  TIM1_CCR2L = (us<<1) & 255;
}

void TIM1_ovf(void) __interrupt(TIM1_OVR_UIF_IRQ) {
  TIM1_SR1 &= ~TIM_SR1_UIF;
}

void TIM2_ovf(void) __interrupt(TIM2_OVR_UIF_IRQ) {
  // 100Hz timer
  PA_ODR ^= (1 << BLUE_LED);
  TIM2_SR1 &= ~TIM_SR1_UIF;
}

void TIM4_ovf(void) __interrupt(TIM4_OVR_UIF_IRQ) {
  PA_ODR ^= (1 << GREEN_LED);
  TIM4_SR &= ~TIM_SR1_UIF;
}

void UART1_int(void) __interrupt(UART1_RX_IRQ) {
  uint8_t rxch = UART1_DR;
  if (rxch >= '0' && rxch <= '9') {
    uint16_t period = (rxch - '0') * (PWM_PERIOD/9);
    set_hbridge_fwd(period);
  } else if (rxch == '.') {
    set_hbridge_brake(0);
  } else if (rxch == '-') {
    set_hbridge_brake(PWM_PERIOD);
  } else {
    while (!(UART1_SR & UART_SR_TC));
    UART1_DR = rxch;
  }
}

int main() {
  PA_DDR |= (1 << GREEN_LED) | (1 << BLUE_LED);  // configure PD4 as output
  PA_CR1 |= (1 << GREEN_LED) | (1 << BLUE_LED);  // push-pull mode
  PB_ODR = 0x30;  // HUR high (off) HUL high (off)
  PB_DDR |= 0x30;  // HUL, HUR open drain outputs

  PC_DDR |= 0xc0;  // HLL, HLR outputs
  PC_CR1 |= 0xc0;
  PC_ODR = 0x00;

  PC_DDR |= 0x10;  // SRV output
  PC_CR1 |= 0x10;

  TIM1_ARRH = (PWM_PERIOD-1) >> 8;
  TIM1_ARRL = (PWM_PERIOD-1) & 255;

  TIM1_CCER1 = 0x11;  // Enable output compare
  TIM1_CCMR1 = 0x60;  // PWM mode
  TIM1_CCMR2 = 0x60;  // PWM mode

  TIM2_ARRH = (SERVO_PWM_PERIOD-1) >> 8;
  TIM2_ARRL = (SERVO_PWM_PERIOD-1) & 255;

  TIM2_CCER1 = 0x11;  // Enable output compare
  TIM2_CCMR1 = 0x60;  // PWM mode
  TIM2_CCMR2 = 0x60;  // PWM mode

  set_hbridge_brake(0);

  set_ch1_period(1500);
  set_ch2_period(1500);

  TIM1_CR1 = 0x01;  // Enable TIM1
  TIM1_IER |= TIM_IER_UIE;  // Enable Update Interrupt
  TIM1_BKR |= 0x80;  // main output enable

  TIM2_CR1 = 0x01;  // Enable TIM1
  TIM2_IER |= TIM_IER_UIE;  // Enable Update Interrupt

  /* Prescaler = 128 */
  TIM4_PSCR = 0b00000111;
  /* Period ~= 30Hz */
  TIM4_ARR = 250;
  TIM4_IER |= TIM_IER_UIE;  // Enable Update Interrupt
  TIM4_CR1 |= TIM_CR1_CEN;  // Enable TIM4

  UART1_BRR2 = 0x01;  // 115200 baud ~= 2MHz/17
  UART1_BRR1 = 0x01;
  UART1_CR2 = UART_CR2_RIEN | UART_CR2_TEN | UART_CR2_REN;

  rim();
  uint8_t i = 0;
  while (1) {
    /* toggle pin every 250ms */
    // wfi();
    wfi();
    //while (!(UART1_SR & UART_SR_TC));
    // UART1_DR = i++;
  }
}
