/* sbus-telemetry.c
 * ver 0.1
 *
 * SBus2 telemetry quick and dirty prototype
 *
 * Based on jitel project (https://github.com/delamonpansie/jitel)
 * Important links:
 * - http://sbustelemetrysensors.blogspot.ru/
 * - https://sites.google.com/site/sbus2diy/home (russian)
 */

/**
 * Based on SPRACINGF3 board.
 *
 * Pinouts:
 *
 * OUTPUTS 1-8:
 * PWM1 					-	PA6
 * PWM2 					-	PA7
 * PWM3						-	PA11
 * PWM4						-	PA12
 * PWM5						-	PB8
 * PWM6						-	PB9
 * PWM7						-	PA2
 * PWM8						-	PA3
 *
 * IO_1
 * RC_CH1					-	PA0
 * RC_CH2 					-	PA1
 * RC_CH5 					-	PB4
 * RC_CH6 					-	PB5
 * LED_STRIP				-	PA8
 *
 * IO_2
 * CURRENT_METER_ADC_PIN	-	PA5
 * RSSI_ADC_PIN				-	PB2
 * RC_CH3					-	PB11
 * RC_CH4					-	PB10
 * RC_CH7					-	PB0
 * RC_CH8					-	PB1
 *
 * VBAT_ADC_PIN				-	PA4
 * LED						-	PB3
 *
 */

/**
 *
 */

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

#include "sbus-telemetry.h"



static int measure_rpm = 360;
static int measure_temp = 0x8282;

static unsigned char sbus_slot_data[31][3];
const char sbus_slot_id[] = {
				0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,	// Frame0 04, Slot  0-7
				0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, // Frame1 14, Slot  8-15
				0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,	// Frame2 24, Slot 16-23
				0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb 	// Frame3 34, Slot 24-31
			};
static volatile char sbus_slot_index;
static char sbus_slot_base;



void sbus_send_slot(int n)
{
	if (sbus_slot_data[n][0] == 0) {
		return;
	}

	for (int i = 0; i < 3; i++) {
		usart_send_blocking(SBUS_PORT, sbus_slot_data[n][i]);
	}
}


void tim3_isr()
{
	if (++sbus_slot_index == 7)
		TIM3_CR1 &= ~TIM_CR1_CEN;
	sbus_send_slot(sbus_slot_base + sbus_slot_index);
	TIM3_SR &= ~TIM_SR_UIF;
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

	/** jitel
	 * channel data is 25 bytes long,
	 * 1 start bit + 8 data bit + parity bit + 2 stop bit
	 * gives 120 us per byte and 3000us per 25 bytes
	 */
	tim2_us(3000 - 120 + 50); // 24 bytes + jitter
	for (int i = 1; i < 25; i++) {
		// TBD
		while ((USART_ISR(SBUS_PORT) & (USART_SR_RXNE | USART_SR_ORE |
				     USART_SR_NE | USART_SR_FE |
				     USART_SR_PE)) == 0 &&
		       (TIM2_SR & TIM_SR_UIF) == 0);

		buf[i] = usart_recv_blocking(SBUS_PORT);
	}

	led0_toggle;
	switch (buf[24]) {
	case 0x04: sbus_slot_base = 0; break;	// frame0
	case 0x14: sbus_slot_base = 8; break;	// frame1
	case 0x24: sbus_slot_base = 16; break;	// frame2
	case 0x34: sbus_slot_base = 24; break;	// frame3
	default:
		return; /* framing error ? */
	}

	tim2_us(2000);  /* telemety is 2ms after end of channel data */
	sbus_update_slot_data(); /* calulate slot values while waiting */
	while ((TIM2_SR & TIM_SR_UIF) == 0);

	// TBD
	sbus_slot_index = 0;
	TIM3_CNT = 0;
	TIM3_CR1 |= TIM_CR1_CEN; /* start slot timer */

	sbus_send_slot(sbus_slot_base + sbus_slot_index);
	while (sbus_slot_index != 7); /* wait till all slots sent */
}


int main(void)
{
	init_clock();
	init_gpio();
	init_usart();
	init_timer();

	for (;;) {
		sbus_tick();
	}
}
