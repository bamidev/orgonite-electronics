import random



MAX_VAL = 32
WIDTH = 4
HEIGHT = 4


def value_to_color(value):
	ONE_THIRD = 1.0/3
	TWO_THIRD = 2.0/3

	if value < ONE_THIRD:
		r = round(value*3*MAX_VAL)
		return (r, 0, MAX_VAL-r)
	elif value < TWO_THIRD:
		g = round((value*3-1)*MAX_VAL)
		return (MAX_VAL-g, g, 0)
	else:
		b = round((value*3-2)*MAX_VAL)
		return (0, MAX_VAL-b, b)



print('#include "rgb.h"\n')

print('const struct Rgb RANDOM_FIELD[{}][{}] = {{'.format(WIDTH, HEIGHT))

random.seed()



# Generate a list with equally spaced values between 0 and 1.
values = [i / (WIDTH*HEIGHT) for i in range(WIDTH*HEIGHT)]

rgbs = []
# Initialize 2D field
for x in range(WIDTH):

	print('\t{')
	for y in range(HEIGHT):
		# Put the color closest to purple as the first color
		if x == 0 and y == 0:
			index = round(len(values)/6)
		# Otherwise, just distribute them randomly
		else:
			index = random.randint(0, len(values)-1) if len(values) > 0 else 0

		# Take a random value
		value = values[index]
		values.pop(index)

		# Convert random value into RGB values in C
		r, g, b = value_to_color(value)
		rgbs.append((r, g, b))
		print('\t\t{' + str(r) + ',' + str(g) + ',' + str(b) + '},')
	print('\t},')

print('};')

# Sanity check
total_r = sum([x[0] for x in rgbs])
total_g = sum([x[1] for x in rgbs])
total_b = sum([x[2] for x in rgbs])
assert (max((total_r, total_g, total_b)) - min((total_r, total_g, total_b))) < 3, "Colors not evenly distributed: R=%i, G=%i, B=%i" % (total_r, total_g, total_b)