#include <math.h>
#include <stdint.h>

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>



#define WAVE_MODE_SQUARE 0
#define WAVE_MODE_SINE 1
#define WAVE_MODE_SAW 2
#define WAVE_MODE_SHAPE_MASK 0b11
#define WAVE_MODE_BIDIRECTIONAL 0b100

#define WAVE_RESOLUTION 500	//< This number is chosen because the chip has 512 bytes of RAM.




void calculate_best_wave_parameters();
void calculate_wave();
void calculate_wave_saw();
void calculate_wave_sine();
void initialize();
void load_mode();
void save_mode();
void setup_io_pins();
void setup_spi();
void setup_timer0();



uint8_t wave_mode = 0;
float wave_frequency = 0.0;
uint8_t wave_points[WAVE_RESOLUTION];	//< Precalculated set of points that describe the wave
uint16_t wave_points_len = 0;	//< Length of array wave_points
uint8_t wave_scale = 1;

uint8_t step = 0;
uint16_t wave_pos = 0;



/// Sets `wave_scale` and `wave_points_len` to values that most accurately describe the frequency of `wave_frequency`.
void calculate_best_wave_parameters() {

	// The number of intervals that span 1 second
	const double one_sec_ints = 62500.0;	// 16000000/256
	// The number of intervals that span one period of the wave
	double period_ints = one_sec_ints / wave_frequency;

	// Scale of the wave, minimum 1
	wave_scale = (uint8_t)ceil(period_ints / WAVE_RESOLUTION);
	if (wave_scale == 0)	wave_scale = 1;

	// The actual number of points for the period of the wave
	double actual_period_ints = period_ints / wave_scale;
	wave_points_len = (uint16_t)round(actual_period_ints);
}

/// Calculates the points that describe the wave function.
void calculate_wave() {

	// First, calculate the wave_scale and timer_interval.
	calculate_best_wave_parameters();

	// Then, calculate the points
	uint8_t shape = wave_mode & WAVE_MODE_SHAPE_MASK;
	if (shape == WAVE_MODE_SAW) {
		calculate_wave_saw();
	}
	else if (shape == WAVE_MODE_SINE) {
		calculate_wave_sine();
	}
}

void calculate_wave_saw() {
	double quarter_period = wave_points_len / 4;
	uint16_t i = 0;
	

	uint16_t quarter_period_int = quarter_period;
	for (; i < quarter_period; i++) {
		wave_points[i] = 127 + (uint8_t)((double)i / quarter_period * 128.0);
	}

	for (int16_t j = i; i < (quarter_period*3); i++, j--) {
		wave_points[i] = 127 + (int8_t)((double)j / quarter_period * 128.0);
	}

	for (uint32_t j = (int32_t)i-wave_points_len; i < wave_points_len; i++, j++) {
		wave_points[i] = 127 + (int8_t)((double)j / quarter_period * 128.0);
	}
}

void calculate_wave_sine() {

	for (uint16_t i = 0; i < wave_points_len; i++) {
		wave_points[i] = 127 + 128 * sin(M_PI*2 / wave_points_len);
	}
}

// Startup code
void initialize() {
	setup_spi();
	setup_io_pins();
	
	load_mode();

	setup_timer0();
}

/// Loads the configured wave parameters from eeprom.
void load_mode() {
	wave_mode = eeprom_read_byte((void*)0);
	eeprom_read_block((void*)&wave_frequency, (void*)1, sizeof(float));
}

/// Saves the current wave mode parameters onto eeprom.
void save_mode() {
	eeprom_write_byte((void*)0, wave_mode);
	eeprom_write_block((void*)&wave_frequency, (void*)1, sizeof(float));
}

void setup_timer0() {
	cli();		// Disable global interrupts
	TCCR0B = 0<<WGM02 | 0<<CS02 | 0<<CS01 | 1<<CS00;	//Put Timer/Counter1 in CTC mode, with /256 prescaling
	TCCR0A = 1<<WGM01 | 1<<WGM02;
	
	TIMSK0 = 1<<OCIE0B;	// Enable timer compare interrupt
	sei();	// Enable global interrupts
}

/// Sets up the pins used
void setup_io_pins() {
	// Port B pin 0 & 1:
	DDRB = 0b11;
	PORTB = 0b01;
}

void setup_spi() {
}



int main() {
	initialize();

	// Do nothing.
	// Only the timer does what we need.
	while (1) {}

	return 0;
}



// Timer 0 comparator B interrupt:
ISR(TIMER0_COMPB_vect) {	

	// Continually loop through the wave points:
	if (step == 0) {
		OCR0A = wave_points[wave_pos];
		//OCR0B = 255 - wave_points[wave_pos]?

		wave_pos += 1;
		if (wave_pos >= wave_points_len)	wave_pos = 0;
	}

	step += 1;
	if (step >= wave_scale)	step = 0;
}