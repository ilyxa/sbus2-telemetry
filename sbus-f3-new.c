#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

//~ #define CONSOLE_PORT 	USART1
#define SBUS_PORT 		USART1
#define LED0 GPIO3 // PB3
#define led0_toggle 	gpio_toggle(GPIOB, LED0);
#define delay800  for (int i = 0; i < 800000; i++) __asm__("nop");

//~ sbus declaration
static unsigned char sbus_slot_data[31][3];
const char sbus_slot_id[] = { 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
			 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
			 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
			 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb };
static volatile char sbus_slot_index;
static char sbus_slot_base;

void setup_clock(void)
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);
}
void setup_gpio(void)
{
	//~ prototype
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED0);
}
void tim2_us(unsigned delay) // from jitel project
{
	TIM2_CR1 &= ~TIM_CR1_CEN;
	TIM2_SR &= ~TIM_SR_UIF;

	TIM2_CNT = 0;
	TIM2_ARR = delay;
	TIM2_CR1 |= TIM_CR1_CEN;
}

static void sbus_slot(int n, unsigned short val)
{
	sbus_slot_data[n][0] = sbus_slot_id[n];
	sbus_slot_data[n][2] = val & 0xff;
	sbus_slot_data[n][1] = val >> 8;
}


static void sbus_update_slot_data()
{
	int rpm=100;
	/* SBS-01RM */
	rpm++;
	//~ sbus_slot(4, swap(rpm/6)); //rpm sensor
	/* SBS-01V */
	sbus_slot(5, 0x1000);
	sbus_slot(6, 0xF0);
	// temp
	sbus_slot(1, 0x8181);
}

void setup_usart_sbus(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
	//~ gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);
	rcc_periph_clock_enable(RCC_USART1);
	usart_set_baudrate(SBUS_PORT, 100000);
	usart_set_databits(SBUS_PORT, 8);
	usart_set_stopbits(SBUS_PORT, USART_STOPBITS_2);
	usart_set_mode(SBUS_PORT, USART_MODE_TX_RX);
	usart_set_parity(SBUS_PORT, USART_PARITY_EVEN);
	usart_set_flow_control(SBUS_PORT, USART_FLOWCONTROL_NONE);
	//~ usart_enable_data_inversion(SBUS_PORT);
	usart_enable_rx_inversion(SBUS_PORT);
	//~ usart_enable_tx_inversion(SBUS_PORT);
	usart_enable(SBUS_PORT);
}
void setup_timer(void)
{
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_clock_enable(RCC_TIM3);
	rcc_periph_reset_pulse(RST_TIM2);
	rcc_periph_reset_pulse(RST_TIM3);
	nvic_set_priority(NVIC_TIM3_IRQ, 1 << 4);
	nvic_enable_irq(NVIC_TIM3_IRQ);

	//~ TIM2_PSC = rcc_apb1_frequency / 500000 - 1; /* 1usec per count */
	TIM3_PSC = rcc_apb1_frequency / 500000 - 1; /* 1usec per count */
	TIM2_PSC = 63;
	TIM3_ARR = 660; /* 3 bytes * 120us per byte  = 360 us for data
			   an 300us gap between slots */
	TIM3_DIER |= TIM_DIER_UIE;

	TIM2_CR1 |= TIM_CR1_URS; /* setting EGR_UG won't trigger ISR */
	TIM3_CR1 |= TIM_CR1_URS;
	// stm32f3 problem is here need more check
	TIM2_EGR |= TIM_EGR_UG; /* force load PSC */
	TIM3_EGR |= TIM_EGR_UG;
}

void sbus_send_slot(int n)
{
	//~ prototype 
	if (sbus_slot_data[n][0] == 0) return;
	//~ redefine gpio slot while sending data here
	for (int i = 0; i < 3; i++) {
		while ((USART_ISR(SBUS_PORT) & USART_SR_TXE) == 0);
		USART1_TDR = sbus_slot_data[n][i];
	}
	while ((USART_ISR(SBUS_PORT) & USART_ISR_TC) == 0);
	//~ redefine gpio back
}

void tim3_isr()
{
	if (++sbus_slot_index == 7)
		TIM3_CR1 &= ~TIM_CR1_CEN;
	sbus_send_slot(sbus_slot_base + sbus_slot_index);
	TIM3_SR &= ~TIM_SR_UIF;
}


void sbus_tick(void)
{
	uint32_t isr;
	uint8_t c;
	unsigned char buf[25] = {0};

	do buf[0] = usart_recv_blocking(USART1);
	while (buf[0] != 0x0f);
	/* channel data is 25 bytes long,
	   1 start bit + 8 data bit + parity bit + 2 stop bit
	   gives 120 us per byte and 3000us per 25 bytes */
	tim2_us(3000 - 120 + 50); /* 24 bytes + jitter */
	for (int i = 1; i < 25; i++) {
		isr = USART1_ISR;
		
		while ((isr & (USART_SR_RXNE | USART_SR_ORE |
				     USART_SR_NE | USART_SR_FE |
				     USART_SR_PE)) == 0 &&
		       (TIM2_SR & TIM_SR_UIF) == 0);
		
		isr = USART1_ISR;
		
		if ((isr & (USART_SR_ORE | USART_SR_NE |
				   USART_SR_FE | USART_SR_PE)) ||
				(TIM2_SR & TIM_SR_UIF)) 
			return;
		c = USART1_RDR;
		buf[i] = c;
		led0_toggle;
		//~ buf[i] = usart_recv_blocking(USART1);
		usart_send_blocking(SBUS_PORT, buf[24]);
	}
	switch (buf[24]) {
	case 0x04: sbus_slot_base = 0; break;
	case 0x14: sbus_slot_base = 8; break;
	case 0x24: sbus_slot_base = 16; break;
	case 0x34: sbus_slot_base = 24; break;
	default:
		return; /* framing error ? */
	}
	tim2_us(2000);  /* telemety is 2ms after end of channel data */
	sbus_update_slot_data(); /* calulate slot values while waiting */

	while ((TIM2_SR & TIM_SR_UIF) == 0);

	sbus_slot_index = 0;
	TIM3_CNT = 0;
	TIM3_CR1 |= TIM_CR1_CEN; /* start slot timer */

	sbus_send_slot(sbus_slot_base + sbus_slot_index);
	while (sbus_slot_index != 7); /* wait till all slots sent */
}

int main()
{
	setup_clock();
	setup_timer();
	setup_gpio();
	setup_usart_sbus();
	tim2_us(1);
	for (;;) {
		sbus_tick(); // NOOOP
		led0_toggle;
	}
}
