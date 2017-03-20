#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#define led0_toggle gpio_toggle(GPIOB, GPIO3);
#define delay for (int i = 0; i < 800000; i++) __asm__("nop");
#define LED0 GPIO3 // PB3

extern void initialise_monitor_handles(void);

void setup_clock(void)
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);
}

void setup_gpio(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED0);
}

void setup_timer(void)
{
	//~ proto
	rcc_periph_clock_enable(RCC_TIM2);
	timer_reset(TIM2);
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT_MUL_2,
		TIM_CR1_CMS_EDGE, TIM_CR1_DIR_DOWN);
	timer_set_prescaler(TIM2, rcc_apb1_frequency / 500000);
	//~ timer_set_prescaler(TIM2, 1);
	timer_set_period(TIM2, 1);
	timer_enable_counter(TIM2);
}
void tim2_us(unsigned delay_us)
{
	timer_disable_counter(TIM2);
	timer_reset(TIM2);
	timer_set_period(TIM2, delay_us);
	timer_enable_counter(TIM2);
}
int main(void)
{
    initialise_monitor_handles();
    //~ printf("hello world!\n");
	//~ printf("Init...");
	//~ printf("Clock setup...");
	//~ setup_clock();
	//~ setup_gpio();
	//~ setup_timer();
	//~ tim2_us(1000);
	//~ while (timer_get_counter(TIM2) != 0);
	//~ while (!timer_get_flag(TIM2, TIM_SR_UIF));
	//~ led0_toggle;
	led0_toggle;
	    fprintf(stdout, "Stdout output: \r\n");

for(;;);
}
