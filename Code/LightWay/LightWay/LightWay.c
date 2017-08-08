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

#define LED_ON do{PORTB |= (1 << LED_PIN);} while (0)
#define LED_OFF do{PORTB &= ~(1 << LED_PIN);} while(0)

#define TIMER_VAL	15535		// with a clock running @ clk/1024, TIMER_VAL will give a time of 50ms
#define LIGHT_TIMEOUT 1//1200		// 60 secs / 50ms	

volatile uint8_t pir_count;
volatile uint32_t timer_loop;
uint16_t timer_load_val;

ISR(INT0_vect)
{
	++pir_count;
	TCNT1 = TIMER_VAL;
	TCCR1B |= (1 << CS12) | (1 << CS10);		// Start timer clock @ clk/1024 =>  1.024ms
	
}

ISR(TIMER1_OVF_vect)
{
	//After overflowing the desired number of times procedd down
	++timer_loop;
	if(timer_loop > LIGHT_TIMEOUT)
	{
		TCCR1B = 0x00;		// stop all timer clock
		pir_count = 0;	
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
	MCUCR |= (1 << ISC00) | (1 << ISC01);	// Rising edge of INT0 will trigger an interrupt
	
	ACSR |= (1 << ACD);		// Disable the analog comparator to reduce power consumption
	ADCSRA &= ~(1 << ADEN);	// Ensuring ADC is kept OFF to reduce power consumption
	
}

void init_timer()
{
	TCCR1A = 0x00;				// Normal timer function.
	TCCR1B = 0x00;				// Clock is off
	TIMSK |= (1 << TOIE1);		// Overflow interrupt enabled
	TCNT1 = TIMER_VAL;			// time for 50ms @ clk/1024 
	
	TCCR1B |= (1 << CS12) | (1 << CS10);
}

int main(void)
{
	
	setup_controller();
	init_timer();
	pir_count = 0;
	sei();
	
	LED_ON;
	
    while(1)
    {
		if(pir_count == 0)
		{
			LED_OFF;
			// sleep;
		}
		
		if(pir_count > 0)
		{
			LED_ON;
		}
    }
}