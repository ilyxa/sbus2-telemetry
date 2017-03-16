#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#define CONSOLE_PORT 	USART1
#define SBUS_PORT 		USART2

//~ sbus declaration
//~ static unsigned char sbus_slot_data[31][3];
const char sbus_slot_id[] = { 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
			 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
			 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
			 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb };
static volatile char sbus_slot_index;
static char sbus_slot_base;


void clock_setup(void)
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);
}
void gpio_setup(void)
{
}
void console_usart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART1);
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable(USART1);
}
void sbus_usart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART2);
	usart_set_baudrate(USART2, 100000);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_2);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_EVEN);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
	usart_enable_data_inversion(USART2);
	usart_enable(USART2);
}
void timer_setup(void)
{
	//~ rcc_periph_clock_enable(RCC_TIM2);
	//~ rcc_periph_clock_enable(RCC_TIM3);
}

void sbus_tick(void)
{
	unsigned char buf[25] = {0}; // circullar buffer 
	do buf[0] = usart_recv_blocking(SBUS_PORT); // wait for frame
	while (buf[0] != 0x0f);
	
/* comments from jitel
 * channel data is 25 bytes long,
 * 1 start bit + 8 data bit + parity bit + 2 stop bit
 * gives 120 us per byte and 3000us per 25 bytes
 */ 

	// wait before tel injection
	//~ tim2_us(3000 - 120 + 50); /* 24 bytes + jitter */
	for (int i = 1; i < 25; i++) {
		// reading from port
	}
	switch (buf[24]) {
		case 0x04: sbus_slot_base = 0; break;
		case 0x14: sbus_slot_base = 8; break;
		case 0x24: sbus_slot_base = 16; break;
		case 0x34: sbus_slot_base = 24; break;
	default:
		return; /* framing error ? */
}
	//~ tim2_us(2000);  /* telemety is 2ms after end of channel data */
	//~ update_sbus_slot_data(); /* calulate slot values while waiting */
	//~ while ((TIM2_SR & TIM_SR_UIF) == 0);
	sbus_slot_index = 0;

}

int main()
{
	clock_setup();
	gpio_setup();
	sbus_usart_setup();
	//~ console_usart_setup();
	timer_setup();


	for (;;) {
		sbus_tick(); // NOOOP
	}
}
