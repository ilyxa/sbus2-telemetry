#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

static unsigned char sbus_slot_data[31][3];
const char slot_id[] = { 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
			 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
			 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
			 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb };
static volatile char slot_idx;
static char slot_base;

static int rpm = 360;

static void led()
{
	gpio_toggle(GPIOC, GPIO13);
	for (int i = 0; i < 800; i++)   /* Wait a bit. */
                        __asm__("nop");
}

void tim2_us(unsigned delay)
{
	TIM2_CR1 &= ~TIM_CR1_CEN;
	TIM2_SR &= ~TIM_SR_UIF;

	TIM2_CNT = 0;
	TIM2_ARR = delay;
	TIM2_CR1 |= TIM_CR1_CEN;
}

void exti9_5_isr(void)
{
	unsigned pin = GPIO_IDR(GPIOB) & GPIO9;
	GPIO_BSRR(GPIOA) = pin ? GPIO4 << 16 : GPIO4;
	EXTI_PR = EXTI9;
}

void exti2_isr(void)
{
	unsigned pin = GPIO_IDR(GPIOA) & GPIO2;
	GPIO_BSRR(GPIOB) = pin ? GPIO5 << 16 : GPIO5;
	EXTI_PR = EXTI2;
}

static void timer_init()
{
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_clock_enable(RCC_TIM3);
	nvic_set_priority(NVIC_TIM3_IRQ, 1 << 4);
	nvic_enable_irq(NVIC_TIM3_IRQ);

	TIM2_PSC = rcc_apb1_frequency / 500000; /* 1usec per count */
	TIM3_PSC = rcc_apb1_frequency / 500000; /* 1usec per count */

	TIM3_ARR = 660; /* 3 bytes * 120us per byte  = 360 us for data
			   an 300us gap between slots */
	TIM3_DIER |= TIM_DIER_UIE;

	TIM2_CR1 |= TIM_CR1_URS; /* setting EGR_UG won't trigger ISR */
	TIM3_CR1 |= TIM_CR1_URS;
	TIM2_EGR |= TIM_EGR_UG; /* force load PSC */
	TIM3_EGR |= TIM_EGR_UG;
}


static uint16_t swap(uint16_t a)
{
	return (a << 8) | (a >> 8);
}

static void sbus_slot(int n, unsigned short val)
{
	sbus_slot_data[n][0] = slot_id[n];
	sbus_slot_data[n][2] = val & 0xff;
	sbus_slot_data[n][1] = val >> 8;
}

static void sbus_send_slot(int n)
{
	if (sbus_slot_data[n][0] == 0)
		return;

	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO5); /* PB5 */

	for (int i = 0; i < 3; i++) {
		while ((USART2_SR & USART_SR_TXE) == 0);
		USART2_DR = sbus_slot_data[n][i];
	}
	while ((USART2_SR & USART_SR_TC) == 0);

	gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, GPIO5);
}

static void update_slot_data()
{
	/* SBS-01RM */
	rpm++;
	sbus_slot(4, swap(rpm/6)); //rpm sensor
	/* SBS-01V */
	sbus_slot(5, 0x1000);
	sbus_slot(6, 0xF0);
	// temp
	sbus_slot(1, 0x8181);
}

static void sbus_tick()
{
	unsigned char buf[25] = {0};
	do buf[0] = usart_recv_blocking(USART2);
	while (buf[0] != 0x0f);

	led();
	/* channel data is 25 bytes long,
	   1 start bit + 8 data bit + parity bit + 2 stop bit
	   gives 120 us per byte and 3000us per 25 bytes */
	tim2_us(3000 - 120 + 50); /* 24 bytes + jitter */

	for (int i = 1; i < 25; i++) {
		while ((USART2_SR & (USART_SR_RXNE | USART_SR_ORE |
				     USART_SR_NE | USART_SR_FE |
				     USART_SR_PE)) == 0 &&
		       (TIM2_SR & TIM_SR_UIF) == 0);

		if ((USART2_SR & (USART_SR_ORE | USART_SR_NE |
				  USART_SR_FE | USART_SR_PE)) ||
		    (TIM2_SR & TIM_SR_UIF))
			return;
		buf[i] = USART2_DR;
	}

	switch (buf[24]) {
	case 0x04: slot_base = 0; break;
	case 0x14: slot_base = 8; break;
	case 0x24: slot_base = 16; break;
	case 0x34: slot_base = 24; break;
	default:
		return; /* framing error ? */
	}

	tim2_us(2000);  /* telemety is 2ms after end of channel data */
	update_slot_data(); /* calulate slot values while waiting */
	while ((TIM2_SR & TIM_SR_UIF) == 0);

	slot_idx = 0;
	TIM3_CNT = 0;
	TIM3_CR1 |= TIM_CR1_CEN; /* start slot timer */

	sbus_send_slot(slot_base + slot_idx);
	while (slot_idx != 7); /* wait till all slots sent */
}

void tim3_isr()
{
	if (++slot_idx == 7)
		TIM3_CR1 &= ~TIM_CR1_CEN;
	sbus_send_slot(slot_base + slot_idx);
	TIM3_SR &= ~TIM_SR_UIF;
}

static void sbus_uart_init()
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_USART2);
	rcc_periph_clock_enable(RCC_AFIO);

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX); /* PA2 */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, 	      GPIO_USART2_RX); /* PA3 */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL,       GPIO4); /* PA4 -> PA3, in */

	gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_PULL_UPDOWN,     GPIO9); /* PB9 */
	gpio_clear(GPIOB, GPIO9); /* weak pull down */

	gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT,           GPIO5); /* PB5 */


	exti_select_source(EXTI9, GPIOB);
	exti_set_trigger(EXTI9, EXTI_TRIGGER_BOTH);
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);
	exti_enable_request(EXTI9);

	exti_select_source(EXTI2, GPIOA);
	exti_set_trigger(EXTI2, EXTI_TRIGGER_BOTH);
	nvic_enable_irq(NVIC_EXTI2_IRQ);
	exti_enable_request(EXTI2);

	usart_set_baudrate(USART2, 100000);
	USART_CR2(USART2) |= USART_CR2_STOPBITS_2;
	USART_CR1(USART2) |= USART_CR1_RE | USART_CR1_TE | /* rx,tx */
			     USART_CR1_M | USART_CR1_PCE | /* 8E */
			     USART_CR1_UE;
}

int main()
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	timer_init();

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set(GPIOC, GPIO13);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, /* LED on PC13 */
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO11);

	sbus_uart_init(); // NOOOP
	for (int i = 0; i < 4; i++) led(); //wait few moment...

	for (;;) {
		sbus_tick(); // NOOOP
	}
}

