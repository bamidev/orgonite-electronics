import serial
import time

from serial.serialutil import EIGHTBITS, PARITY_ODD, STOPBITS_ONE



with serial.Serial(
	port='/dev/ttyUSB1',
	baudrate=9600,
	bytesize=EIGHTBITS,
	#parity=PARITY_ODD,
	stopbits=STOPBITS_ONE,
	timeout=60
) as dev:
	last_timestamp = None

	while True:
		byte = dev.read(1)
		if len(byte) == 0:
			print("Nothing received for 60 seconds, exiting.")
			break
		timestamp = time.time()
		if last_timestamp != None:
			if (timestamp - last_timestamp) > 0.1:
				print("Byte:", int(byte[0]), "- Delta", timestamp - last_timestamp)
		else:
			print("Byte:", int(byte[0]))

		last_timestamp = timestamp