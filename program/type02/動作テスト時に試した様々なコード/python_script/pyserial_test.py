import serial


ser = serial.Serial("COM18", 19200)
for i in range(1000):
	ser.write(range(48, 58))
ser.close()