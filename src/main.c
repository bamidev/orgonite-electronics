#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>

#include "random_field.h"
#include "rgb.h"



// The number of possible steps that our custom PWM implementation uses.
#define PWM_RESOLUTION 32
#define FIELD_RES 32

typedef unsigned char Byte;
typedef unsigned int Uint;

const Uint X_MAX = RANDOM_FIELD_WIDTH*FIELD_RES;
const Uint Y_MAX = RANDOM_FIELD_HEIGHT*FIELD_RES;



Byte pwm_step = 0;
// LED's:
struct Rgb center, corner_tl, corner_tr, corner_bl, corner_br;



Byte approximate(
	Byte val_a, Uint dist_a,
	Byte val_b, Uint dist_b,
	Byte val_c, Uint dist_c,
	Byte val_d, Uint dist_d
);
void coord_2_color(Uint x, Uint y, struct Rgb* color);
Uint distance(Uint x, Uint y);
void initialize();
void setup_clock();
void setup_pins();



Byte approximate(
	Byte val_a, Uint dist_a,
	Byte val_b, Uint dist_b,
	Byte val_c, Uint dist_c,
	Byte val_d, Uint dist_d
) {
	Uint dist_total = dist_a + dist_b + dist_c + dist_d;

	Uint x = val_a*dist_a + val_b*dist_b + val_c*dist_c + val_d*dist_d;
	return x / dist_total;
}

void coord_2_color(Uint x, Uint y, struct Rgb* color) {

	// 'local' coordinates; where in between the surrounding 4 colors in the random field this point is located
	Byte x_local = x % FIELD_RES;
	Byte y_local = y % FIELD_RES;

	// 'field' coordinates; the 4 colors surrounding the coord point.
	Uint field_tl_x = x / FIELD_RES;
	Uint field_tl_y = y / FIELD_RES;
	Uint field_br_x = x_local != 0 ? (field_tl_x+1) : field_tl_x;
	Uint field_br_y = y_local != 0 ? (field_tl_y+1) : field_tl_y;

	struct Rgb color_tl = RANDOM_FIELD[field_tl_x][field_tl_y];
	struct Rgb color_tr = RANDOM_FIELD[field_br_x][field_tl_y];
	struct Rgb color_bl = RANDOM_FIELD[field_tl_x][field_br_y];
	struct Rgb color_br = RANDOM_FIELD[field_br_x][field_br_y];
	
	// The distance to each surrounding field coordinate
	Uint dist_tl = distance(x_local, y_local);
	Uint dist_tr = distance(FIELD_RES - x_local, y_local);
	Uint dist_bl = distance(x_local, FIELD_RES - y_local);
	Uint dist_br = distance(FIELD_RES - x_local, FIELD_RES - y_local);

	// Calculate the value of each color depending on which surrounding colors are closest.
	color->r = approximate(
		color_tl.r, dist_tl,
		color_tr.r, dist_tr,
		color_bl.r, dist_bl,
		color_br.r, dist_br
	);
	color->g = approximate(
		color_tl.g, dist_tl,
		color_tr.g, dist_tr,
		color_bl.g, dist_bl,
		color_br.g, dist_br
	);
	color->b = approximate(
		color_tl.b, dist_tl,
		color_tr.b, dist_tr,
		color_bl.b, dist_bl,
		color_br.b, dist_br
	);
}

// The Pythagorean theorem, but floored
Uint distance(Uint x, Uint y) {
	return sqrt(x*x + y*y);
}

int main() {

	initialize();

	Uint x = 0, y = 0, i = 0;
	Uint direction_x = 3, direction_y = 1;


	while(1) {

		// TODO: Only perform this operation when the timer that controls the Mobius coil switching has been invoked.

		// Small step in a direction
		x += direction_x;
		if (x >= X_MAX)	x -= X_MAX;
		y += direction_y;
		if (y >= Y_MAX)	y -= Y_MAX;

		Uint tl_x = 8 <= x ? x - 8 : (X_MAX - 8 - x);
		Uint tl_y = 8 <= y ? y - 8 : (Y_MAX - 8 - y);
		Uint br_x = x + 8;
		Uint br_y = y + 8;

		coord_2_color(x, y, &center);
		coord_2_color(tl_x, tl_y, &corner_tl);
		coord_2_color(br_x, tl_y, &corner_tr);
		coord_2_color(tl_x, br_y, &corner_bl);
		coord_2_color(br_x, br_y, &corner_br);

		i += 1;
	}

	return 0;
}

void initialize() {
	setup_clock();
	setup_pins();

	// Initialize LED colors
	center = RANDOM_FIELD[0][0];
	coord_2_color(X_MAX - 8, Y_MAX - 8, &corner_tl);
	coord_2_color(8, Y_MAX - 8, &corner_tr);
	coord_2_color(X_MAX - 8, 8, &corner_bl);
	coord_2_color(8, 8, &corner_br);
}

void setup_clock() {
	cli();			//Disable global interrupts
	TCCR1B |= 1<<CS11 | 1<<CS11;	//Divide by 64
	OCR1A = 64;		//Count 15624 cycles for 1 second interrupt
	TCCR1B |= 1<<WGM12;		//Put Timer/Counter1 in CTC mode
	TIMSK1 |= 1<<OCIE1A;		//enable timer compare interrupt
	sei();			//Enable global interrupts
}

// Sets all used pins to outgoing and high
void setup_pins() {
	// Port B pin 0 through 7:
	DDRB = 0b11111111;
	PORTB = 0b11111111;
	// Port D pin 0 through 6:
	DDRD = 0b1111111;
	PORTD = 0b1111111;
}

// Timer 1 interrupt:
ISR(TIMER1_COMPA_vect)
{
	// Every PWM_RESOLUTION steps, turn all colors which are not 0 on.
	if (pwm_step % PWM_RESOLUTION == 0) {
		Byte new_portb = 0b00000000, new_portd = 0b0000000;

		if (center.r == 0)	new_portb |= 1<<0;
		if (center.g == 0)	new_portb |= 1<<1;
		if (center.b == 0)	new_portb |= 1<<2;

		if (corner_tl.r == 0)	new_portb |= 1<<3;
		if (corner_tl.g == 0)	new_portb |= 1<<4;
		if (corner_tl.b == 0)	new_portb |= 1<<5;

		if (corner_tr.r == 0)	new_portb |= 1<<6;
		if (corner_tr.g == 0)	new_portb |= 1<<7;
		if (corner_tr.b == 0)	new_portd |= 1<<0;

		if (corner_bl.r == 0)	new_portd |= 1<<1;
		if (corner_bl.g == 0)	new_portd |= 1<<2;
		if (corner_bl.b == 0)	new_portd |= 1<<3;

		if (corner_br.r == 0)	new_portd |= 1<<4;
		if (corner_br.g == 0)	new_portd |= 1<<5;
		if (corner_br.b == 0)	new_portd |= 1<<6;

		PORTB = new_portb;
		PORTD = new_portd;

		pwm_step = 1;
		return;
	}

	// Otherwise, turn each color pin off when they have reached the step at which their duty cycle has ended.
	Byte new_portb = PORTB, new_portd = PORTD;

	if (center.r == pwm_step)	new_portb |= 1<<0;
	if (center.g == pwm_step)	new_portb |= 1<<1;
	if (center.b == pwm_step)	new_portb |= 1<<2;

	if (corner_tl.r == pwm_step)	new_portb |= 1<<3;
	if (corner_tl.g == pwm_step)	new_portb |= 1<<4;
	if (corner_tl.b == pwm_step)	new_portb |= 1<<5;

	if (corner_tr.r == pwm_step)	new_portb |= 1<<6;
	if (corner_tr.g == pwm_step)	new_portb |= 1<<7;
	if (corner_tr.b == pwm_step)	new_portd |= 1<<0;

	if (corner_bl.r == pwm_step)	new_portd |= 1<<1;
	if (corner_bl.g == pwm_step)	new_portd |= 1<<2;
	if (corner_bl.b == pwm_step)	new_portd |= 1<<3;

	if (corner_br.r == pwm_step)	new_portd |= 1<<4;
	if (corner_br.g == pwm_step)	new_portd |= 1<<5;
	if (corner_br.b == pwm_step)	new_portd |= 1<<6;

	PORTB = new_portb;
	PORTD = new_portd;

	// Increment step
	pwm_step += 1;
	if (pwm_step > PWM_RESOLUTION)	pwm_step = 0;
}