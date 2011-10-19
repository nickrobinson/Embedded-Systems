import serial

# This class serves as a wrapper for the pyserial class to
#   control I2C devices

# TODO: doxygen-ate this file
#       scan all serial ports to find the hardware via a key/ID string
#   
class i2cDevice:
	def __init__(self, PORT):
		self.addr = 0x9E
		self.ser = serial.Serial('COM7', 57600, timeout=0)
        
	def setPortName(self, portName):
		self.ser.port = portName

	def setAddress(self, address):
		self.address = address

	def open(self):
		self.ser.close()
		self.ser.open()
        
	def isOpen(self):
		return self.ser.isOpen()

	def close(self):
		self.ser.close()

	def getWriteAddress(self):
		return (self.address + 0x01)

	def getReadAddress(self):
		return self.address

	def read(self, length):
		
		return self.ser.read(length)

	def write(self, buffer):
		self.ser.write(buffer)
                
        
