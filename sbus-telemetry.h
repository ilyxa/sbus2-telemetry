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

void tim2_us(unsigned delay);
#ifdef DEBUG
extern void initialise_monitor_handles(void);
#endif

void init_clock(void)
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
}

void init_gpio(void)
{
	// LED0 PB3
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED0);
	gpio_set(GPIOB, LED0);
}

void init_usart(void)
{
	/* S.Bus 1000009E2 inverted
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
	rcc_periph_clock_enable(RCC_USART1);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO9 | GPIO10);  // GPIO_OTYPE_PP tx
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

	usart_set_mode(SBUS_PORT, USART_MODE_TX_RX);
	usart_set_baudrate(SBUS_PORT, 100000);
	usart_set_databits(SBUS_PORT, 9);
	usart_set_parity(SBUS_PORT, USART_PARITY_EVEN);
	usart_set_stopbits(SBUS_PORT, USART_STOPBITS_2);
	usart_set_flow_control(SBUS_PORT, USART_FLOWCONTROL_NONE);
	usart_enable_halfduplex(SBUS_PORT);
	usart_enable_rx_inversion(SBUS_PORT);
	usart_enable_tx_inversion(SBUS_PORT);

	// TBD
	/**
	 * enable Overrun disable while debug?
	 * USART_ISR_ORE flag
	 */
	USART_CR3(SBUS_PORT) |= USART_CR3_OVRDIS;
	//~ USART_CR3(USART1) &= ~USART_CR3_OVRDIS;

	usart_enable(SBUS_PORT);

	// console port USART3
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

void init_timer(void)
{
	rcc_periph_reset_pulse(RST_TIM2);
	rcc_periph_clock_enable(RCC_TIM2);
	TIM2_PSC = rcc_apb1_frequency / 500000; /* 1usec per count */
	TIM2_CR1 |= TIM_CR1_URS; /* setting EGR_UG won't trigger ISR */
	TIM2_EGR |= TIM_EGR_UG; /* force load PSC */

	rcc_periph_reset_pulse(RST_TIM3);
	rcc_periph_clock_enable(RCC_TIM3);

	nvic_set_priority(NVIC_TIM3_IRQ, 1 << 4);
	nvic_enable_irq(NVIC_TIM3_IRQ);

	TIM3_PSC = rcc_apb1_frequency / 500000; /* 1usec per count */
	TIM3_ARR = 660; /* 3 bytes * 120us per byte  = 360 us for data
			   an 300us gap between slots */
	TIM3_DIER |= TIM_DIER_UIE;

	TIM3_CR1 |= TIM_CR1_URS;
	TIM3_EGR |= TIM_EGR_UG;

}

// swap Low-High Nibble
static uint16_t swap(uint16_t a)
{
	return (a << 8) | (a >> 8);
}


void tim2_us(unsigned delay) // from jitel project
{
	TIM2_CR1 &= ~TIM_CR1_CEN;
	TIM2_SR &= ~TIM_SR_UIF;

	TIM2_CNT = 0;
	TIM2_ARR = delay;
	TIM2_CR1 |= TIM_CR1_CEN;
}
