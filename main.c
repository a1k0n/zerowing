#include <stdint.h>

#include "stm8.h"

#define F_CPU 16000000UL
// log (F_CPU/1e6) / log 2
#define US_SHIFT 2

#define BLUE_LED  1
#define GREEN_LED 2

#define PWM_PERIOD       (F_CPU / 1000)
#define SERVO_PWM_PERIOD (F_CPU / 4 / 100)

static void deadtime_delay() {
  // delay 200us, each loop is 3 instrs at 2MHz, minus the overhead of the call
  // itself
  volatile uint16_t i = (F_CPU * 200 / 1e6) / 7 - 2;
  while (--i)
    ;
}

// FIXME!
// need to wait 50us after changing directions before setting nonzero PWM period
static void set_hbridge_brake(uint16_t period) {
  if (PB_ODR != 0x30) {
    TIM1_CCR1H = 0;  // HLR (M+)
    TIM1_CCR1L = 0;
    TIM1_CCR2H = 0;  // HLL (M-)
    TIM1_CCR2L = 0;
    PB_ODR     = 0x30;  // HUR high (off) HUL high (off)
    deadtime_delay();
  }
  TIM1_CCR1H = period >> 8;  // HLR (M+)
  TIM1_CCR1L = period & 255;
  TIM1_CCR2H = period >> 8;  // HLL (M-)
  TIM1_CCR2L = period & 255;
}

static void set_hbridge_fwd(uint16_t period) {
  if (PB_ODR != 0x10) {
    TIM1_CCR1H = 0;  // HLR (M+)
    TIM1_CCR1L = 0;
    TIM1_CCR2H = 0;  // HLL (M-)
    TIM1_CCR2L = 0;
    PB_ODR     = 0x10;  // HUR low (on) HUL high (off)
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
    PB_ODR     = 0x20;  // HUR high (off) HUL low (on)
    deadtime_delay();
  }
  TIM1_CCR1H = period >> 8;  // HLR (M+)
  TIM1_CCR1L = period & 255;
  TIM1_CCR2H = 0;  // HLL (M-)
  TIM1_CCR2L = 0;
}

static void set_ch1_period(uint16_t us) {
  TIM2_CCR1H = (us << US_SHIFT) >> 8;
  TIM2_CCR1L = (us << US_SHIFT) & 255;
}

static void set_ch2_period(uint16_t us) {
  TIM2_CCR3H = (us << US_SHIFT) >> 8;
  TIM2_CCR3L = (us << US_SHIFT) & 255;
}

volatile uint8_t run_state = 0;
volatile uint8_t tim1sav[4];

void TIM1_ovf(void) __interrupt(TIM1_OVR_UIF_IRQ) {
  // 1kHz motor current PWM timer
  TIM1_SR1 &= ~TIM_SR1_UIF;
  switch (run_state) {
    case 1:
      tim1sav[0] = TIM1_CCR1H;
      tim1sav[1] = TIM1_CCR1L;
      tim1sav[2] = TIM1_CCR2H;
      tim1sav[3] = TIM1_CCR2L;
      TIM1_CCR1H = 0;
      TIM1_CCR1L = 0;
      TIM1_CCR2H = 0;
      TIM1_CCR2L = 0;
      // next cycle, we'll have no PWM output; in fact we should start sampling
      // as soon as UAV occurs
      run_state++;
      break;
    case 2:
      // reload timer regs for next cycle
      TIM1_CCR1H = tim1sav[0];
      TIM1_CCR1L = tim1sav[1];
      TIM1_CCR2H = tim1sav[2];
      TIM1_CCR2L = tim1sav[3];
      run_state++;
      break;
    case 3:
      // finished sampling
      PA_ODR &= ~(1 << BLUE_LED);
      ADC_CSR &= ~ADC_CSR_EOCIE;  // disable conversion interrupt
      run_state = 0;
      while (!(UART1_SR & UART_SR_TC))
        ;
      UART1_DR = 0xfe;  // frame end
      break;
  }
}

void TIM1_uev(void) __interrupt(TIM1_UEV_IRQ) {
  TIM1_SR1 &= ~TIM_SR1_CC1IF;
  switch (run_state) {
    case 2:
      // start analog sampling!
      // for now this is just represented as the blue LED...
      PA_ODR |= (1 << BLUE_LED);
      ADC_CSR |= ADC_CSR_EOCIE;  // enable conversion interrupt
      ADC_CSR &= ~ADC_CSR_EOC;
      while (!(UART1_SR & UART_SR_TC))
        ;
      UART1_DR = 0xff;  // frame start
      break;
  }
}

void TIM2_ovf(void) __interrupt(TIM2_OVR_UIF_IRQ) {
  // 100Hz servo PWM timer
  PA_ODR ^= (1 << GREEN_LED);
  TIM2_SR1 &= ~TIM_SR1_UIF;
  if (run_state == 0) {
    // next motor PWM output cycle will be an idle measurement cycle
    run_state = 1;
  }
}

#if 0
void TIM4_ovf(void) __interrupt(TIM4_OVR_UIF_IRQ) {
  PA_ODR ^= (1 << GREEN_LED);
  TIM4_SR &= ~TIM_SR1_UIF;
}
#endif

void ADC_eoc(void) __interrupt(ADC1_EOC_IRQ) {
  uint8_t r = ADC_DRH;
  ADC_CSR &= ~(ADC_CSR_EOC);
  // while (!(UART1_SR & UART_SR_TC));
  UART1_DR = r;
  PA_ODR ^= (1 << BLUE_LED);
}

void UART1_rxint(void) __interrupt(UART1_RX_IRQ) {
  uint8_t rxch = UART1_DR;
  if (rxch >= '0' && rxch <= '9') {
    uint16_t period = (rxch - '0') * (PWM_PERIOD / 9);
    set_hbridge_rev(period);
  } else if (rxch == '.') {
    set_hbridge_brake(0);
  } else if (rxch == '-') {
    set_hbridge_brake(PWM_PERIOD);
  } else {
    while (!(UART1_SR & UART_SR_TC))
      ;
    UART1_DR = rxch;
  }
}

int main() {
  CLK_CKDIVR = 0x00;  // switch HSI to 8MHz
  ADC_CR1 = 0x01;  // Wake up ADC; we'll configure it later once it's fully on

  PA_DDR |= (1 << GREEN_LED) | (1 << BLUE_LED);  // configure PD4 as output
  PA_CR1 |= (1 << GREEN_LED) | (1 << BLUE_LED);  // push-pull mode
  PB_ODR = 0x30;   // HUR high (off) HUL high (off)
  PB_DDR |= 0x30;  // HUL, HUR open drain outputs

  /*  pretty sure this is not necessary
  PC_DDR |= 0xc0;  // HLL, HLR outputs
  PC_CR1 |= 0xc0;
  PC_ODR = 0x00;
  */

  TIM1_ARRH = (PWM_PERIOD - 1) >> 8;
  TIM1_ARRL = (PWM_PERIOD - 1) & 255;

  TIM1_CCMR1 = 0x68;  // PWM mode w/ buffered compare reg
  TIM1_CCMR2 = 0x68;  // PWM mode w/ buffered compare reg
  TIM1_CCER1 = 0x11;  // Enable output compare 1 and 2

  TIM2_PSCR = 2;  // clock down to 4MHz
  TIM2_ARRH = (SERVO_PWM_PERIOD - 1) >> 8;
  TIM2_ARRL = (SERVO_PWM_PERIOD - 1) & 255;

  TIM2_CCMR1 = 0x68;  // PWM mode w/ buffered compare reg
  TIM2_CCMR3 = 0x68;  // PWM mode w/ buffered compare reg
  TIM2_CCER1 = 0x01;  // Enable output compare 1
  TIM2_CCER2 = 0x01;  // Enable output compare 3

  set_hbridge_brake(0);

  set_ch1_period(1500);
  set_ch2_period(1500);

  TIM1_CR1 = 0x01;                          // Enable TIM1
  TIM1_IER |= TIM_IER_UIE | TIM_IER_CC1IE;  // Enable Update, compare 2 event
  TIM1_BKR |= 0x80;                         // main output enable

  TIM2_CR1 = 0x01;          // Enable TIM2
  TIM2_IER |= TIM_IER_UIE;  // Enable Update Interrupt

#if 0
  // Prescaler = 128
  TIM4_PSCR = 0b00000111;
  // Period ~= 120Hz
  TIM4_ARR = 250;
  TIM4_IER |= TIM_IER_UIE;  // Enable Update Interrupt
  TIM4_CR1 |= TIM_CR1_CEN;  // Enable TIM4
#endif

  UART1_BRR2 = 0x00;  // 1MBaud = 16MHz/16
  UART1_BRR1 = 0x01;
  UART1_CR2  = UART_CR2_RIEN | UART_CR2_TEN | UART_CR2_REN;
  UART1_CR3  = 0;

  ADC_CSR = 2;  // read from AIN2, the back-EMF feedback, and enable interrupt
  ADC_CR1 = 0x73;           // fADC = fMASTER/18 (888kHz, 63.5kHz samplerate)
  ADC_CR1 |= ADC_CR1_ADON;  // start continuous conversion
  // want ADC interrupt

  rim();
  uint8_t i = 0;
  while (1) {
    /* toggle pin every 250ms */
    wfi();
  }
}
