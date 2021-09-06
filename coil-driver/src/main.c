#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>




void initialize();
void setup_clock();
void setup_io_pins();
void setup_usart();
void transmit(uint8_t data);



// Startup code
void initialize() {
	setup_usart();
	setup_clock();
	setup_io_pins();
}

void setup_clock() {
	cli();		// Disable global interrupts
	TCCR1B |= 1<<WGM12 | 1<<CS10;	//Put Timer/Counter1 in CTC mode, with no prescaling

	// 16000000/7.83 is the amount of cycles to execute to fire at 7.83Hz (the Schumann resonance).
	// 16000000/7.83/63857 is approx. 31.9998
	// Therefor, approx. 63857*32 cycles constitues to one interval for the Schumann resonance.
	// (63857*32)/(16000000/7.83) is approx 1.0000006
	// So that gives us a deviation of approx. 0,00006% if we interrupt every 63857 cycles, and switch at the 32th interrupt.
	OCR1A = 63857-1;
	
	TIMSK |= 1<<OCIE1A;	//enable timer compare interrupt
	sei();	//Enable global interrupts
}

// Sets up the pins used
void setup_io_pins() {
	// Port B pin 0 & 1:
	DDRB = 0xFF;
	PORTB = 0b01;
}

void setup_usart() {
	// TODO: Implement...
	//UCSR0B |= 1<<TXEN0;
	//UCSR0C |= /*1<<UPM01 | 1<<UPM00 |*/ 1<<UCSZ01 | 1<<UCSZ00;	// Asynchronous mode, odd parity, 1 stop-bit, 8-bit characters
	//UCSR0C |= 1<<UCPOL0;	// Needed for asynchronous mode
	// Baudrate of 9600:
	//UBRR0H = 0;
	//UBRR0L = 103;
}

/// Sends a byte through the USART0 port.
void transmit(uint8_t data) {
	// TODO: Implement...
	// Wait until ready to transmit
	//while ( !( UCSR0A & (1<<UDRE0) )) {}

	//UDR0 = data;
}



int main() {

	// Do nothing.
	// Only the timer does what we need.
	while (1) {}

	return 0;
}



// Timer 1 comparator A interrupt:
uint8_t step = 0;
ISR(TIMER1_COMPA_vect)
{	
	step += 1;

	if (step == 32) {
		step = 0;

		// Switch the on/off status of both pin 0 & 1 of port B on step 32.
		PORTB ^= 0b11;
	}
}