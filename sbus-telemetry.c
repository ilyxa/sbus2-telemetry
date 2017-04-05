#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#define SBUS_PORT USART1
#define CONSOLE_PORT USART3
#define LED0 GPIO3 // PB3
#define led0_toggle 	gpio_toggle(GPIOB, LED0);
#define delay800  for (int i = 0; i < 800000; i++) __asm__("nop");
#ifdef ENABLE_SEMIHOSTING
#define DEBUG
#endif

#ifdef DEBUG
#include <stdio.h>
#endif

static int rpm = 360;

void _sbus_telemetry_c(void) {}
void tim2_us(unsigned delay);
static unsigned char sbus_slot_data[31][3];

/**
 * Futaba Sbus2 slot numbering and frame aligement
 * 	04h> Frame0				14h> Frame1			 	24h> Frame2				34h> Frame3
 *	03h	00000011	Slot0	13h	00010011	Slot8	0Bh	00001011	Slot16	1Bh	00011011	Slot24
 *	83h	10000011	Slot1	93h	10010011	Slot9	8Bh	10001011	Slot17	9Bh	10011011	Slot25
 *	43h	01000011	Slot2	53h	01010011	Slot10	4Bh	01001011	Slot18	5Bh	01011011	Slot26
 *	C3h	11000011	Slot3	D3h	11010011	Slot11	CBh	11001011	Slot19	DBh	11011011	Slot27
 *	23h	00100011	Slot4	33h	00110011	Slot12	2Bh	00101011	Slot20	3Bh	00111011	Slot28
 *	A3h	10100011	Slot5	B3h	10110011	Slot13	ABh	10101011	Slot21	BBh	10111011	Slot29
 *	63h	01100011	Slot6	73h	01110011	Slot14	6Bh	01101011	Slot22	7Bh	01111011	Slot30
 *	E3h	11100011	Slot7	F3h	11110011	Slot15	EBh	11101011	Slot23	FBh	11111011	Slot31
 */

const char sbus_slot_id[] = {
				0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,	// Frame0 04, Slot  0-7
				0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, // Frame1 14, Slot  8-15
				0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,	// Frame2 24, Slot 16-23
				0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb 	// Frame3 34, Slot 24-31
			};
static volatile char sbus_slot_index;
//~ static int sbus_slot_index;
static char sbus_slot_base;

#ifdef DEBUG
extern void initialise_monitor_handles(void);
#endif

void init_clock(void)
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);
}

void init_gpio(void)
{
	// LED0 PB3
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED0);
	gpio_set(GPIOB, LED0);
	// PA0 IO2 pin 3
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	gpio_clear(GPIOB, GPIO5);
}

void init_usart(void)
{
	/* S.Bus 1000008E2 inverted
	 *
	 * AF7
	 * USART1_TX	PA9
	 * USART1_RX	PA10
	 *
	 * USART3_TX	PB10
	 * USART3_RX	PB11
	 *
	 */

	// sbus
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART1);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO9 | GPIO10);  // GPIO_OTYPE_PP tx
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

	//~ gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
	//~ gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO10);
	//~ gpio_set_af(GPIOA, GPIO_AF7, GPIO10);

	usart_set_mode(SBUS_PORT, USART_MODE_TX_RX);
	usart_set_baudrate(SBUS_PORT, 100000);
	usart_set_databits(SBUS_PORT, 9);
	usart_set_parity(SBUS_PORT, USART_PARITY_EVEN);
	usart_set_stopbits(SBUS_PORT, USART_STOPBITS_2);
	usart_set_flow_control(SBUS_PORT, USART_FLOWCONTROL_NONE);
	usart_enable_halfduplex(SBUS_PORT);
	usart_enable_rx_inversion(SBUS_PORT);
	usart_enable_tx_inversion(SBUS_PORT);
	//~ usart_enable_data_inversion(SBUS_PORT);

	/**
	 * enable Overrun disable while debug?
	 * USART_ISR_ORE flag
	 */
	USART_CR3(SBUS_PORT) |= USART_CR3_OVRDIS;
	// halfduplex flag
	//~ USART_CR3(SBUS_PORT) |= USART_CR3_HDSEL;
	//~ USART_CR2(SBUS_PORT) &= ~USART_CR2_CLKEN;
	//~ USART_CR2(SBUS_PORT) &= ~USART_CR2_LINEN;
	//~ USART_CR3(SBUS_PORT) &= ~USART_CR3_SCEN;
	//~ USART_CR3(SBUS_PORT) &= ~USART_CR3_IREN;

	// disable
	//~ USART_CR3(USART1) &= ~USART_CR3_OVRDIS;

	usart_enable(SBUS_PORT);
	// console port USART3
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_USART3);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10 | GPIO11);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO10 | GPIO11); // GPIO_OTYPE_OD rx GPIO_OTYPE_PP tx ???
	gpio_set_af(GPIOB, GPIO_AF7, GPIO10 | GPIO11);
	usart_set_mode(CONSOLE_PORT, USART_MODE_TX_RX);
	usart_set_baudrate(CONSOLE_PORT, 115200);
	usart_set_databits(CONSOLE_PORT, 8);
	usart_set_parity(CONSOLE_PORT, USART_PARITY_NONE);
	usart_set_stopbits(CONSOLE_PORT, USART_STOPBITS_1);
	usart_set_flow_control(CONSOLE_PORT, USART_FLOWCONTROL_NONE);
	usart_enable(CONSOLE_PORT);
}

void sbus_send_slot(int n)
{
	if (sbus_slot_data[n][0] == 0) {
		return;
	}
	//~ tim2_us(100);
	//~ tim2_us(5);
	//~ gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO9 | GPIO10);  // GPIO_OTYPE_PP tx
	//~ gpio_set(GPIOB, GPIO5);

	for (int i = 0; i < 3; i++) {
	//~ gpio_set(GPIOB, GPIO5);

		usart_send_blocking(SBUS_PORT, sbus_slot_data[n][i]);
		usart_send_blocking(CONSOLE_PORT, sbus_slot_data[n][i]);
	}
	//~ gpio_clear(GPIOB, GPIO5);
	//~ gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO9 | GPIO10);  // GPIO_OTYPE_PP tx


}


void tim3_isr()
{
	if (++sbus_slot_index == 7)
		TIM3_CR1 &= ~TIM_CR1_CEN;
	sbus_send_slot(sbus_slot_base + sbus_slot_index);
	TIM3_SR &= ~TIM_SR_UIF;
}



void init_timer(void)
{
	rcc_periph_reset_pulse(RST_TIM2);
	rcc_periph_clock_enable(RCC_TIM2);
	TIM2_PSC = rcc_apb1_frequency / 500000; /* 1usec per count */
	//~ TIM2_PSC = rcc_apb1_frequency / 500000 / 2 - 1;
	TIM2_CR1 |= TIM_CR1_URS; /* setting EGR_UG won't trigger ISR */
	TIM2_EGR |= TIM_EGR_UG; /* force load PSC */

	rcc_periph_reset_pulse(RST_TIM3);
	rcc_periph_clock_enable(RCC_TIM3);

	nvic_set_priority(NVIC_TIM3_IRQ, 1 << 4);
	nvic_enable_irq(NVIC_TIM3_IRQ);

	TIM3_PSC = rcc_apb1_frequency / 500000; /* 1usec per count */
	//~ TIM3_PSC = rcc_apb1_frequency / 500000 / 2 - 1;
	TIM3_ARR = 660; /* 3 bytes * 120us per byte  = 360 us for data
			   an 300us gap between slots */
	TIM3_DIER |= TIM_DIER_UIE;

	TIM3_CR1 |= TIM_CR1_URS;
	// stm32f3 problem is here need more check
	TIM3_EGR |= TIM_EGR_UG;

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
	/**
	 * example
	 * val = 0x8182
	 * val[2] & 0xff = 82
	 * val[1] >> 8 = 81
	*/

	sbus_slot_data[n][0] = sbus_slot_id[n];
	sbus_slot_data[n][2] = val & 0xff;
	sbus_slot_data[n][1] = val >> 8;

	/*
	 * 03 = 03
	 * 13 = 93
	 */

	//~ sbus_slot_data[n][0] = 0x83;
	//~ sbus_slot_data[n][2] = 0x83;
	//~ sbus_slot_data[n][1] = 0x81;


	//~ usart_send_blocking(CONSOLE_PORT, sbus_slot_data[n][0]);
	//~ usart_send_blocking(CONSOLE_PORT, sbus_slot_data[n][1]);
	//~ usart_send_blocking(CONSOLE_PORT, sbus_slot_data[n][2]);

}

static uint16_t swap(uint16_t a)
{
	return (a << 8) | (a >> 8);
}

static void sbus_update_slot_data()
{
	//~ /* SBS-01RM */
	rpm++;
	sbus_slot(4, swap(rpm/6)); //rpm sensor
	/* SBS-01V */
	sbus_slot(5, 0x1000);
	sbus_slot(6, 0xF0);
	// temp
	sbus_slot(1, 0x8282	);
}



void sbus_tick(void)
{
	unsigned char buf[25] = {0};

	do {
		buf[0] = usart_recv_blocking(SBUS_PORT);
	}
	while (buf[0] != 0x0f);

	#ifdef DEBUG
	printf("got 0xf\r\n");
	#endif

	/** channel data is 25 bytes long,
	 * 1 start bit + 8 data bit + parity bit + 2 stop bit
	 * gives 120 us per byte and 3000us per 25 bytes
	 */
	tim2_us(3000 - 120 + 50); // 24 bytes + jitter
	for (int i = 1; i < 25; i++) {
		//~ while ((USART_ISR(SBUS_PORT) & (USART_SR_RXNE | USART_SR_ORE |
				     //~ USART_SR_NE | USART_SR_FE |
				     //~ USART_SR_PE)) == 0 &&

		while ((USART_ISR(SBUS_PORT) & (USART_SR_RXNE | USART_SR_ORE |
				     USART_SR_NE | USART_SR_FE |
				     USART_SR_PE)) == 0 &&
		       (TIM2_SR & TIM_SR_UIF) == 0);

		//~ while (!(TIM2_SR & TIM_SR_UIF));

		//~ if ((USART_ISR(SBUS_PORT) & (USART_SR_ORE | USART_SR_NE |
			//~ USART_SR_FE | USART_SR_PE)) ||
			//~ (TIM2_SR & TIM_SR_UIF))
				//~ return;

		buf[i] = usart_recv_blocking(SBUS_PORT);
	}

	//~ printf("%i %i %i\r\n", buf[2], buf[3], buf[24]);
	//~ usart_send_blocking(CONSOLE_PORT, buf[24]);
	//~ led0_toggle;
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

			//~ usart_send_blocking(CONSOLE_PORT, sbus_slot_base);
			//~ usart_send_blocking(CONSOLE_PORT, sbus_slot_index);

	sbus_send_slot(sbus_slot_base + sbus_slot_index);
	//~ usart_send_blocking(CONSOLE_PORT, sbus_slot_index);
	while (sbus_slot_index != 7); /* wait till all slots sent */
}


int main(void)
{
	//~ printf("sbus-telemetry \r\n");

	init_clock();
	init_gpio();
	init_usart();
	init_timer();
	//~ for(;;) 	usart_send_blocking(CONSOLE_PORT,18);
	for (;;) {
		sbus_tick();
		//~ usart_send_blocking(CONSOLE_PORT,255);
	}

}
