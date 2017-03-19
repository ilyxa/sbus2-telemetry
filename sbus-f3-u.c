#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define LED0 GPIO3
#define delay 	for (int i = 0; i < 800000; i++) __asm__("nop");

int main(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED0);

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);
	
	//~ usart_disable(SBUS_PORT);
	rcc_periph_clock_enable(RCC_USART1);

	usart_set_baudrate(USART1, 9600);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	//~ usart_enable_rx_inversion(USART1);
	//~ usart_enable_tx_inversion(USART1);
	//~ usart_enable_data_inversion(USART1);
	usart_enable(USART1);
	gpio_toggle(GPIOB, LED0);
	for (;;) {
	unsigned char buf[25] = {0}; // circullar buffer 
	buf[0] = usart_recv_blocking(USART1); // wait for frame
	usart_send_blocking(USART1, buf[0]);
	gpio_toggle(GPIOB, LED0);
	}
	
	return 0;
}
