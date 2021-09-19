#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>




void initialize();
void setup_clock();
void setup_io_pins();
void setup_usart();
void transmit(uint8_t data);



volatile uint32_t step = 0;



// Startup code
void initialize() {
	setup_usart();
	setup_io_pins();
	setup_clock();
}

void setup_clock() {
	cli();		// Disable global interrupts
	TCCR1 = 1<<CTC1 | 0<<CS13 | 0<<CS12 | 0<<CS11 | 1<<CS10;	//Put Timer/Counter1 in CTC mode, with no prescaling

	// The chip uses an external clock source of 16Mhz.
	// 16000000/7.83 is the amount of cycles to run endure one interval of 7.83Hz (the Schumann resonance).
	// 16000000/7.83/60 is approx. 34057.
	// Therefor, approx. 34057*60 cycles constitues to one interval for the Schumann resonance.
	// (34057*60)/(16000000/7.83) is approx 0.999999, so that gives us a deviation of about 0,0001%.
	// For some reason, the timer doesn't work when going too fast (e.g. 32 cycles).
	// So we stick with something like 60 cycles, which is still more than accurate enough.
	OCR1C = 60-1;
	
	TIMSK |= 1<<OCIE1A;	// Enable timer compare interrupt
	sei();	// Enable global interrupts
}

/// Sets up the pins used
void setup_io_pins() {
	// Port B pin 0 & 1:
	DDRB = 0b11;
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
	initialize();

	// Do nothing.
	// Only the timer does what we need.
	while (1) {
		if (step >= 34057) {
			PORTB ^= 0b11;

			step -= 34057;
		}
	}

	return 0;
}



// Timer 1 comparator A interrupt:
ISR(TIMER1_COMPA_vect) {	
	step += 1;
}