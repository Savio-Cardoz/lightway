/*
 * LightWay.c
 *
 * Created: 8/4/2017 8:34:33 PM
 *  Author: Cardoz
 */ 
#define F_CPU 1000000UL		// 1 MHZ clock set in fuse bits

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define PIR_PIN 2
#define LED_PIN	1

#define TIMER_VAL	15535		// with a clock running @ clk/1024, TIMER_VAL will give a time of 50ms
#define LIGHT_TIMEOUT 3//1200		// 60 secs / 50ms	

typedef struct			// Using to access individual bits/pins of a register/port
{
	unsigned int bit0:1;
	unsigned int bit1:1;
	unsigned int bit2:1;
	unsigned int bit3:1;
	unsigned int bit4:1;
	unsigned int bit5:1;
	unsigned int bit6:1;
	unsigned int bit7:1;
} _io_reg;

#define REGISTER_BIT(rg,bt) ((volatile _io_reg*)&rg)->bit##bt

#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))
#define BIT_FLIP(ADDRESS,BIT) ((ADDRESS)^=(BIT))

#define LED_ON SETBIT(PORTB, 1)
#define LED_OFF CLEARBIT(PORTB, 1)

volatile uint8_t pir_count, flag;
volatile uint32_t timer_loop;
uint16_t timer_load_val;

ISR(INT0_vect)
{
	pir_count += 1;
	TCNT1 = TIMER_VAL;
	TCCR1B |= (1 << CS12);		// Start timer clock @ clk/1024 =>  1.024ms
	LED_ON;
}

ISR(TIMER1_OVF_vect)
{
	//After overflowing the desired number of times proceed down
	++timer_loop;
	TCNT1 = TIMER_VAL;
	if(timer_loop > LIGHT_TIMEOUT)
	{
		TCCR1B = 0x00;		// stop all timer clock
		pir_count = 0;	
		cli();
		LED_OFF;
		_delay_ms(10000);
	}
	
	//go to sleep
}

void setup_controller()
{
	DDRD &= ~(1 << PIR_PIN);		// PIR Input
	PORTD |= (1 << PIR_PIN);		// Pulled up
	
	DDRB |= (1 << LED_PIN);			// LED Input
	PORTB &= ~(1 << LED_PIN);		// Keep LED OFF
	
	GICR |= (1 << INT0);					// Enable interrupt on INT0 pin
	CLEARBIT(MCUCR, ISC00);
	CLEARBIT(MCUCR, ISC01);
	
	ACSR |= (1 << ACD);		// Disable the analog comparator to reduce power consumption
	ADCSRA &= ~(1 << ADEN);	// Ensuring ADC is kept OFF to reduce power consumption
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void init_timer()
{
	TCCR1A = 0x00;				// Normal timer function.
	TCCR1B = 0x00;				// Clock is off
	TIMSK |= (1 << TOIE1);		// Overflow interrupt enabled
	TCNT1 = TIMER_VAL;			// time for 50ms @ clk/1024 
}

int main(void)
{
	setup_controller();
	init_timer();
	pir_count = 0;
	sei();
	
    while(1)
    {
		if(pir_count == 0)
		{
			sleep_enable();
			sleep_cpu();
			sleep_disable();
		}
    }
}