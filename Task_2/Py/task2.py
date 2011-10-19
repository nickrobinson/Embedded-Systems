import serial
import time

#Setup up serial port for communication.
def comSetup():
        ser = serial.Serial(6, 57600)
        ser.close()
        ser.open()
        return ser

ser = comSetup();


#Main loop
while 1:

	#Prompt user to enter character for temperature format.
	print "Enter 'K' for temperature, '@' for min and max temperatures, or ",
	print "'#' for reference voltage: ",
	entry = raw_input()
	
	#Loop for invalid user input.
	while  entry != 'K' and entry != '@' and entry != '#':
		print "Enter 'K' for temperature, '@' for min and max temperatures, or "
		print "'#' for reference voltage: ",
		entry = raw_input()
		
	#Output user input to MCU.
	ser.write(entry);
                    
	#Print temperature in celcius.
	if entry == 'K' or entry == 'k':
		digitalTemp = ser.read(10)
		temp = int(digitalTemp, 16)
		voltage = ((3.0/4096) * temp) - .424
		tempCelcius = voltage/.00625
		print "Temperature in Celcius: ",
		print tempCelcius
		
		#Print temperature in Farenheit if user prompted.
		print "Convert to Farenheit? (y/n): ",
		temperatureScale = raw_input()
		
		if temperatureScale == "y" or temperatureScale == "Y":
				print "Temperature in Farenheit: ",
				temperatureFarenheit = ((9.0/5.0)*tempCelcius+32)
				print temperatureFarenheit
	
	#Print min and max temperatures.
	elif entry == '@':
		digitalTempMin = ser.read(10)
		digitalTempMax = ser.read(10)
		tempMax = int(digitalTempMax, 16)
		tempMin = int(digitalTempMin, 16)
		voltageMax = ((3.0/4096) * tempMax) - .424
		voltageMin = ((3.0/4096) * tempMin) - .424
		tempCelciusMax = voltageMax/.00625
		tempCelciusMin = voltageMin/.00625
		print "The Maximum temp read is: " 
		print tempCelciusMax 
		print "The Minimum temp read is: " 
		print tempCelciusMin
	
	elif entry == '#':
		refVoltage = ser.read(10)
		print "Reference Voltage: " + refVoltage[6] + "." + refVoltage[7:9]
		