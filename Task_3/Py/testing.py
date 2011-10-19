import serial
import time
import i2cDevice
import binascii

def binaryToTemp(binary_string):
	if binary_string[0] == '0':
		 temp = int(binary_string[1:8],2)
		 if binary_string[8] == '1':
			 temp = temp + .5
		 if binary_string[9] == '1':
			 temp = temp + .25
		 if binary_string[10] == '1':
			 temp = temp + .125
		 if binary_string[11] == '1':
			 temp = temp + .0625
	elif binary_string[0] == '1':
		 temp = (128 - int(binary_string[1:8],2))
		 if binary_string[8] == '1':
			 temp = temp - .5
		 if binary_string[9] == '1':
			 temp = temp - .25
		 if binary_string[10] == '1':
			 temp = temp - .125
		 if binary_string[11] == '1':
			 temp = temp - .0625

		 temp = temp * -1
		 
	return temp
	
def getTempF(temp):
        temp = temp * 1.8
        temp = temp + 32
        return temp

x = i2cDevice.i2cDevice("COM7")
x.open()

if (x.isOpen()):
	x.write("S9EAA")
	time.sleep(0.5)
	x.write("R02")
	time.sleep(0.5)
	msb = x.read(1)
	lsb = x.read(1)
	
	msbInt = int(binascii.hexlify(msb), 16)
	lsbInt = int(binascii.hexlify(lsb), 16)
	
	msbstr = bin(msbInt)[2:].zfill(8)
	lsbstr = bin(lsbInt)[2:].zfill(8)
	tempBinStr = msbstr + lsbstr
	temp = binaryToTemp(tempBinStr)
	print "Celcius Temp: "
	print temp
	print
	tempF = getTempF(temp)
	print "Fahrenheit Temp: "
	print tempF
	print
	

