#include <math.h>
#include <stdint.h>

#ifdef GEN_BITMAP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifndef GEN_BITMAP
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

#include "random_field.h"
#include "rgb.h"



// The number of possible steps that our custom PWM implementation uses.
#define M_PI 3.14159265358979323846
#define PWM_RESOLUTION 32
#define FIELD_RES 128

typedef unsigned char uint8_t;

const uint16_t X_MAX = RANDOM_FIELD_WIDTH*FIELD_RES;
const uint16_t Y_MAX = RANDOM_FIELD_HEIGHT*FIELD_RES;



uint8_t pwm_step = 0;
volatile uint16_t movement_step = 0;
// LED's:
struct Rgb center = {0xff,0xff,0xff};
volatile struct Rgb corner_tl = {0xff,0xff,0xff}, corner_tr = {0xff,0xff,0xff}, corner_bl = {0xff,0xff,0xff}, corner_br = {0xff,0xff,0xff};



void apply_variation_color(volatile uint8_t* base, uint8_t variation, uint8_t orders_down);
uint8_t approximate(
	uint8_t val_a, float dist_a,
	uint8_t val_b, float dist_b,
	uint8_t val_c, float dist_c,
	uint8_t val_d, float dist_d
);
void coord_2_color(uint16_t x, uint16_t y, volatile struct Rgb* color);
void coord_2_color_layer(uint16_t x, uint16_t y, volatile struct Rgb* color);
void coord_2_color_lowres(uint16_t x, uint16_t y, volatile struct Rgb* color);
void coord_2_color_variation(uint16_t x, uint16_t y, uint8_t scale, uint8_t orders_down, volatile struct Rgb* color);
float distance(uint16_t x, uint16_t y);
void initialize();
void set_center_r(uint8_t level);
void set_center_g(uint8_t level);
void set_center_b(uint8_t level);
void set_corner_tl_g(uint8_t level);
void set_timer0_pwma_value(uint8_t val);
void set_timer0_pwmb_value(uint8_t val);
void set_timer2_pwma_value(uint8_t val);
void set_timer2_pwmb_value(uint8_t val);
void setup_clock();
void setup_io_pins();
void setup_pwm_pins();
void setup_usart();
float weight(float dist);
void transmit(uint8_t data);



void apply_variation_color(volatile uint8_t* base, uint8_t variation, uint8_t orders_down) {
	int sum = *base;
	sum += variation >> orders_down;

	if (sum > 255)
		*base = 255;
	else if (sum < 0)
		*base = 0;
	else
		*base = sum;
}

/// Gives the mean average of a color value depending on its distance to that value
uint8_t approximate(
	uint8_t val_a, float dist_a,
	uint8_t val_b, float dist_b,
	uint8_t val_c, float dist_c,
	uint8_t val_d, float dist_d
) {
	float dist_total = dist_a + dist_b + dist_c + dist_d;
	float x = val_a*dist_a + val_b*dist_b + val_c*dist_c + val_d*dist_d;

	return x / dist_total;
}

/// Calculates what color is active at the given point in space, calculated from the random field.
void coord_2_color(uint16_t x, uint16_t y, volatile struct Rgb* color) {
	coord_2_color_layer(x/8, y/8, color);
	coord_2_color_variation(x/4, y/4, 1, 1, color);
	coord_2_color_variation(x/2, y/2, 8, 3, color);
}

void coord_2_color_layer(uint16_t x, uint16_t y, volatile struct Rgb* color) {

	// 'local' coordinates; where in between the surrounding 4 colors in the random field this point is located
	uint16_t x_local = x % FIELD_RES;
	uint16_t y_local = y % FIELD_RES;

	// 'field' coordinates; the 4 colors surrounding the coord point.
	uint16_t field_tl_x = (x / FIELD_RES)%RANDOM_FIELD_WIDTH;
	uint16_t field_tl_y = (y / FIELD_RES)%RANDOM_FIELD_HEIGHT;
	uint16_t field_br_x = ((field_tl_x+1)%RANDOM_FIELD_WIDTH);
	uint16_t field_br_y = ((field_tl_y+1)%RANDOM_FIELD_HEIGHT);

	const struct Rgb* color_tl = &RANDOM_FIELD[field_tl_x][field_tl_y];
	const struct Rgb* color_tr = &RANDOM_FIELD[field_br_x][field_tl_y];
	const struct Rgb* color_bl = &RANDOM_FIELD[field_tl_x][field_br_y];
	const struct Rgb* color_br = &RANDOM_FIELD[field_br_x][field_br_y];
	
	// The distance to each surrounding field coordinate
	float weight_tl = weight(distance(x_local, y_local));
	float weight_tr = weight(distance(FIELD_RES - x_local, y_local));
	float weight_bl = weight(distance(x_local, FIELD_RES - y_local));
	float weight_br = weight(distance(FIELD_RES - x_local, FIELD_RES - y_local));

	// Calculate the value of each color depending on which surrounding colors are closest.
	color->r = approximate(
		color_tl->r, weight_tl,
		color_tr->r, weight_tr,
		color_bl->r, weight_bl,
		color_br->r, weight_br
	);
	color->g = approximate(
		color_tl->g, weight_tl,
		color_tr->g, weight_tr,
		color_bl->g, weight_bl,
		color_br->g, weight_br
	);
	color->b = approximate(
		color_tl->b, weight_tl,
		color_tr->b, weight_tr,
		color_bl->b, weight_bl,
		color_br->b, weight_br
	);
}

void coord_2_color_variation(uint16_t x, uint16_t y, uint8_t scale, uint8_t orders_down, volatile struct Rgb* color) {
	struct Rgb var;
	coord_2_color_layer(x*scale, y*scale, &var);

	// Apply variation
	apply_variation_color(&color->r, var.r, orders_down);
	apply_variation_color(&color->g, var.g, orders_down);
	apply_variation_color(&color->b, var.b, orders_down);
}

/// Calculates the color of a point in space, but assign the colors with a resolution suitable for the PWM simulation.
void coord_2_color_lowres(uint16_t x, uint16_t y, volatile struct Rgb* color) {
	coord_2_color(x, y, color);

	color->r >>= 3;
	color->g >>= 3;
	color->b >>= 3;
}

/// Same as `coord_2_color_lowres`, except it only downscales the red and blue part.
void coord_2_color_lowres_rb(uint16_t x, uint16_t y, volatile struct Rgb* color) {
	coord_2_color(x, y, color);	// TODO: Only calculate red and blue...

	color->r >>= 3;
	color->b >>= 3;
}

// The Pythagorean theorem
float distance(uint16_t x, uint16_t y) {
	return sqrtf(x*x + y*y);
}

#ifndef GEN_BITMAP
int main() {

	initialize();

	uint16_t x = 0, y = 0;
	const int8_t x_dir = 1, y_dir = 1;
	uint32_t space = 0;

	while (1) {
		// Approximately every tenth of a second:
		if (movement_step >= 5000) {
			movement_step -= 5000;

			// Small step in a direction
			x += x_dir;
			if (x >= X_MAX)	x -= 0;
			
			if (x % 3 == 0) {
				y += y_dir;

				if (y >= Y_MAX)	y -= 0;
			}
			

			uint16_t tl_x = 64 <= x ? x - 64 : (X_MAX - 64 - x);
			uint16_t tl_y = 64 <= y ? y - 64 : (Y_MAX - 64 - y);
			uint16_t br_x = x + 64;
			uint16_t br_y = y + 64;

			coord_2_color(x, y, &center);
			coord_2_color_lowres_rb(tl_x, tl_y, &corner_tl);
			coord_2_color_lowres(br_x, tl_y, &corner_tr);
			coord_2_color_lowres(tl_x, br_y, &corner_bl);
			coord_2_color_lowres(br_x, br_y, &corner_br);

			set_center_r(center.r);
			set_center_g(center.g);
			set_center_b(center.b);
			set_corner_tl_g(corner_tl.g);

			//i += 1;
		}
		else
			space += 1;
	}

	return 0;
}
#endif

#ifdef GEN_BITMAP
// When compiling with `GEN_BITMAP` set, compiles an executable that generates a bitmap file of the color field that the LED-driver uses.
int main() {
	// Dimensions
    int32_t width = X_MAX*2;
    int32_t height = Y_MAX*2;
    uint16_t bitcount = 24;//<- 24-bit bitmap

    // Width with padding
    int width_in_bytes = ((width * bitcount + 31) / 32) * 4;

    // Pixel count
    uint32_t imagesize = width_in_bytes * height;

    // sizeof(BITMAPINFOHEADER)
    const uint32_t biSize = 40;

    // Bitmap bits start after headerfile:
    // sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
    const uint32_t bfOffBits = 54; 

    // Total filesize
    uint32_t filesize = 54 + imagesize;

    // Number of planes
    const uint16_t biPlanes = 1;

    // The header
    unsigned char header[54] = { 0 };
    memcpy(header, "BM", 2);
    memcpy(header + 2 , &filesize, 4);
    memcpy(header + 10, &bfOffBits, 4);
    memcpy(header + 14, &biSize, 4);
    memcpy(header + 18, &width, 4);
    memcpy(header + 22, &height, 4);
    memcpy(header + 26, &biPlanes, 2);
    memcpy(header + 28, &bitcount, 2);
    memcpy(header + 34, &imagesize, 4);

    // Construct image
    unsigned char* buf = malloc(imagesize);
    for(int row = 0; row < height; row++)
    {
        for(int col = 0; col < width; col++)
        {
			struct Rgb color;
			coord_2_color(col, row, &color);
            buf[(height-1-row) * width_in_bytes + col * 3 + 0] = color.b;//blue
            buf[(height-1-row) * width_in_bytes + col * 3 + 1] = color.g;//green
            buf[(height-1-row) * width_in_bytes + col * 3 + 2] = color.r;//red
        }
    }

	// Write to file
    FILE *fout = fopen("bin/random_field.bmp", "wb");
    fwrite(header, 1, 54, fout);
    fwrite((char*)buf, 1, imagesize, fout);
    fclose(fout);
    free(buf);

    return 0;
}
#endif

// Startup code
void initialize() {
	setup_usart();
	setup_clock();
	setup_io_pins();
	setup_pwm_pins();

	// Set initial value of PWM pins
	set_center_r(0);
	set_center_g(0);
	set_center_b(0);
	set_corner_tl_g(0);
}

void setup_clock() {
#ifndef GEN_BITMAP
	cli();		// Disable global interrupts
	TCCR1B |= 1<<WGM12 | 1<<CS10;	//Put Timer/Counter1 in CTC mode, with no prescaling
	OCR1A = 1599;	// Run every 1/10000 of a second. This is just enough to execute the PWM simulating code at a frequency that won't really be seen by the eye.
	
	TIMSK1 |= 1<<OCIE1A;	//enable timer compare interrupt
	sei();	//Enable global interrupts
#endif
}

// Sets all used pins to outgoing and high
void setup_io_pins() {
#ifndef GEN_BITMAP
	// Port B pin 1, 2, 4 & 5:
	DDRB = 0xFF;
	PORTB = 0b110110;

	// Port C pins 0,1,2,5,6
	DDRC = 0xFF;
	PORTC = 0b1111111;

	// Port D pins 3, 5 & 6 are going to be used for PWM, but they need to be turned to 0 in order for that to work...
	DDRD = 0xFF;
	PORTD = 0b100;
#endif
}

void setup_pwm_pins() {
#ifndef GEN_BITMAP
	TCCR0A |= 1<<COM0A0 | 1<<COM0A1 | 1<<COM0B0 | 1<<COM0B1 | 1<<WGM00 | 1<<WGM01;	// Put pin OC0A into fast pwm mode.
	TCCR0B |= 0<<WGM02 | 1<<CS00;	// Make the PWM value variable and as fast as possible.

	TCCR2A |= 1<<COM2A0 | 1<<COM2A1 | 1<<COM2B0 | 1<<COM2B1 | 1<<WGM20 | 1<<WGM21;	// Put pin OC0B into fast pwm mode.
	TCCR2B |= 0<<WGM22 | 1<<CS20;	// Make the PWM value variable and as fast as possible.
#endif
}

void setup_usart() {
#ifndef GEN_BITMAP
	UCSR0B |= 1<<TXEN0;
	UCSR0C |= /*1<<UPM01 | 1<<UPM00 |*/ 1<<UCSZ01 | 1<<UCSZ00;	// Asynchronous mode, odd parity, 1 stop-bit, 8-bit characters
	UCSR0C |= 1<<UCPOL0;	// Needed for asynchronous mode
	// Baudrate of 9600:
	UBRR0H = 0;
	UBRR0L = 103;
#endif
}

void set_center_r(uint8_t level) {
	set_timer0_pwma_value(level);
}

void set_center_g(uint8_t level) {
	set_timer0_pwmb_value(level);
}

void set_center_b(uint8_t level) {
	set_timer2_pwmb_value(level);
}

void set_corner_tl_g(uint8_t level) {
	set_timer2_pwma_value(level);
}

void set_timer0_pwma_value(uint8_t val) {
#ifndef GEN_BITMAP
	OCR0A = val;
#endif
}

void set_timer0_pwmb_value(uint8_t val) {
#ifndef GEN_BITMAP
	OCR0B = val;
#endif
}

void set_timer2_pwma_value(uint8_t val) {
#ifndef GEN_BITMAP
	OCR2A = val;
#endif
}

void set_timer2_pwmb_value(uint8_t val) {
#ifndef GEN_BITMAP
	OCR2B = val;
#endif
}

/// Gives a float from 0 to 1 that indicates how important a given distance is for the determination of a color.
float weight(float input) {
	if (input >= (FIELD_RES-1))	return 1;
	return FIELD_RES-input;
}

/// Sends a byte through the USART0 port.
void transmit(uint8_t data) {
#ifndef GEN_BITMAP
	// Wait until ready to transmit
	while ( !( UCSR0A & (1<<UDRE0) )) {}

	UDR0 = data;
#endif
}



#ifndef GEN_BITMAP
uint8_t portb = 0, portc = 0, portd = 0;

// Timer 1 comparator A interrupt:
ISR(TIMER1_COMPA_vect)
{	
	// Every PWM_RESOLUTION steps, turn all colors which are not 0 on.
	if (pwm_step == 0) {
		portb = 0;
		portc = 0;
		portd = 0;
	}
	
	if (corner_tl.r <= pwm_step)	portb |= 1<<1;
	if (corner_tl.b <= pwm_step)	portb |= 1<<2;

	if (corner_tr.r <= pwm_step)	portb |= 1<<4;
	if (corner_tr.g <= pwm_step)	portb |= 1<<5;
	if (corner_tr.b <= pwm_step)	portc |= 1<<0;

	if (corner_bl.r <= pwm_step)	portc |= 1<<1;
	if (corner_bl.g <= pwm_step)	portc |= 1<<2;
	if (corner_bl.b <= pwm_step)	portc |= 1<<3;

	if (corner_br.r <= pwm_step)	portc |= 1<<4;
	if (corner_br.g <= pwm_step)	portc |= 1<<5;
	if (corner_br.b <= pwm_step)	portd |= 1<<2;

	// Make the changes effective
	PORTB = portb;
	PORTC = portc;
	PORTD = portd;
		
	// Increment steps	
	pwm_step += 1;
	if (pwm_step == (PWM_RESOLUTION-1))	pwm_step = 0;

	movement_step += 1;
}
#endif