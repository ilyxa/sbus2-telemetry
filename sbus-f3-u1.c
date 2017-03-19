#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#define led0_toggle gpio_toggle(GPIOB, GPIO3);
#define delay for (int i = 0; i < 800000; i++) __asm__("nop");


void tim2_us(unsigned delay_us) // from jitel project
{
	TIM2_CR1 &= ~TIM_CR1_CEN;
	TIM2_SR &= ~TIM_SR_UIF;

	TIM2_CNT = 0;
	TIM2_ARR = delay_us;
	TIM2_CR1 |= TIM_CR1_CEN;
}

void rr(void)
{		
		
		unsigned char buf[25] = {0};
		//~ uint8_t reg;
		
		do  
		{
			buf[0] = usart_recv_blocking(USART1);	
			led0_toggle;
			//~ usart_send(USART1, buf[0]);
		}
		while (buf[0] != 0x0f);
		
		led0_toggle;
		tim2_us(300 - 12 + 5);
		for (int i = 1; i < 25; i++) {
			//~ do reg = USART_ISR(USART1);
			//~ while ((reg & (USART_SR_RXNE )) == 0  &&
				//~ (TIM2_SR & TIM_SR_UIF) == 0);
			//~ if ( 
				//~ (TIM2_SR & TIM_SR_UIF)) 
			//~ return;
			
			buf[i] = USART1_RDR;
			led0_toggle;
			
		}
		
		for (int i = 1; i < 25; i++) {
			usart_send_blocking(USART1,buf[i]);
			usart_send_blocking(USART1,i);
		}

}


int main()
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);

	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);

	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_reset_pulse(RST_TIM2);
	//~ TIM2_PSC = rcc_apb1_frequency / 500000 -1 ; /* 1usec per count */
	TIM2_PSC=63;
	TIM2_CR1 |= TIM_CR1_URS; /* setting EGR_UG won't trigger ISR */
	TIM2_EGR |= TIM_EGR_UG; /* force load PSC */
	
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);
	rcc_periph_clock_enable(RCC_USART1);
	usart_set_baudrate(USART1, 100000);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_2);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_EVEN);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable_data_inversion(USART1);
	usart_enable_rx_inversion(USART1);
	//~ usart_enable_tx_inversion(USART1);
	usart_enable(USART1);
	
	tim2_us(1);
	led0_toggle;
		do {
			rr();
		}
		while(1);
}
