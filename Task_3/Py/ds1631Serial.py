import i2cDevice
import binascii
import time

class ds1631Serial(i2cDevice.i2cDevice):
    def __init__(self, PORT):
        i2cDevice.i2cDevice.__init__(self, PORT)

    def binaryToTemp(self, binary_string):

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
             temp = (127 - int(binary_string[1:8],2))
             
             if int(binary_string[8:12], 2) != 0:
                 temp = temp + 1
                 
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

    def decToTwoHexBufZero(self,Data):
        DataStr = str(Data)

        if Data == 0:
             binaryOut = '0000000000000000'
        elif Data > 0:
             strWholePart = DataStr[0:DataStr.index('.')]
             strDecimal = DataStr[DataStr.index('.'):]
             binWholePart = bin(int(strWholePart,10))[2:].zfill(7)
             floatDecimal = float(strDecimal)
             count = 0
             while floatDecimal > 0:
                 floatDecimal = floatDecimal - .0625
                 count = count + 1
             binDecimalPart = bin(count)[2:].zfill(4)
             binaryOut = '0' + binWholePart + binDecimalPart + '0000'
        elif Data < 0:
             strWholePart = DataStr[1:DataStr.index('.')]
             strDecimal = DataStr[DataStr.index('.'):]
             binWholePart =  bin(127 - int(strWholePart,10))[2:].zfill(7)
             floatDecimal = float(strDecimal)
             largestDecimal = 0.9375
             count = 0
             while largestDecimal >= floatDecimal:
                 largestDecimal = largestDecimal - .0625
                 count = count + 1
             binDecimalPart = bin(count)[2:].zfill(4)
             if floatDecimal == 0:
                 binaryOut = '1' + binWholePart[0:7] + '00000000'
             else:
                 binaryOut = '1' + binWholePart+ binDecimalPart + '0000'
     
        return hex(int(binaryOut,2))[2:].zfill(4)

    def getConfig(self):    
        #read config reg
	#control write 0x90
	#control read  0x91
        
		self.open()
		self.write('S')
		self.write('9E')
		self.write('AC')
		time.sleep(0.05)
		self.write('P')
		time.sleep(0.05)
		self.write('R01')
		time.sleep(0.05)
		configBits = self.read(1)
		self.close()
		configBits = (binascii.hexlify(configBits)).upper()
		return chr(int(configBits, 16))
         
        
    def setConfig(self,configData):
	#write config reg
                if len(hex(configData)) < 4:
                    data = hex(configData)
                    data = data.upper()
                    secondByte = data[2]
                    configData = "0x0" + secondByte
                    self.open()
                    self.write('S')
                    self.write('9E')
                    self.write('AC')
                    time.sleep(0.05)
                    self.write(configData[2].upper() + configData[3].upper())
                    time.sleep(0.05)
                    self.write('P')
                    self.close()
                else:
                    self.open()
                    self.write('S')
                    self.write('9E')
                    self.write('AC')
                    time.sleep(0.05)
                    self.write(hex(configData)[2] + hex(configData)[3])
                    time.sleep(0.05)
                    self.write('P')
                    self.close()
        
    def getCounter(self):
	#not sure what this counter is
        pass
        
    def getTempC(self):
        # need to check 
	#get number from device, then convert to acutal number
	#bit 15 is sign bit
	#bits 14-8 contain first part of number for negative numbers 127-value = number
	#bits 7-4 contain decimal portion with bit

		self.open()
		self.write('S')
		self.write('9E')
		self.write('AA')
		time.sleep(0.05)
		self.write('R02')
		time.sleep(0.05)
		msb = self.read(1)
		lsb = self.read(1)
		self.write('P')
		self.close()

		msbstr = bin(int(binascii.hexlify(msb), 16))[2:].zfill(8)
		lsbstr = bin(int(binascii.hexlify(lsb), 16))[2:].zfill(8)
		tempBinStr = msbstr + lsbstr
		temp = self.binaryToTemp(tempBinStr)

		return temp
		 
    def getTempF(self):
	# call getTempC and do the F converstion thing
        temp = self.getTempC()
        temp = temp * 1.8
        temp = temp + 32
        return temp
		 
        
    def softwareReset(self):
		self.open()
		self.write('S')
		self.write('9E')
		self.write('54')
		time.sleep(0.05)
		self.write('P')
		self.close()
		 
        
    # Returns the thermostat low value in Celsius.
    def getTH(self):
	#read value from THF
		self.open()
		self.write('S')
		self.write('9E')
		self.write('A1')
		time.sleep(0.05)
		self.write('P')
		time.sleep(0.05)
		self.write('R02')
		time.sleep(0.05)
		msb = self.read(1)
		lsb = self.read(1)
		self.close()

		msbstr = bin(int(binascii.hexlify(msb), 16))[2:].zfill(8)
		lsbstr = bin(int(binascii.hexlify(lsb), 16))[2:].zfill(8)
		tempBinStr = msbstr + lsbstr
		temp = self.binaryToTemp(tempBinStr)

		return temp
    
    # TH data must be in Celsius.
    def setTH(self,thData):
	#write value to TH reg 
		hexDataVal = self.decToTwoHexBufZero(thData)
		hexDataVal = hexDataVal.upper()
		self.open()
		self.write('S')
		self.write('9E')
		self.write('A1')
		self.write(hexDataVal[0:2])
		self.write(hexDataVal[2:4])
		self.write('P')
		self.close()
         
    
    # Returns the thermostat low value in Celsius.
    def getTL(self):
		#read from TL reg
		self.open()
		self.write('S')
		self.write('9E')
		self.write('A2')
		time.sleep(0.05)
		self.write('P')
		time.sleep(0.05)
		self.write('R02')
		time.sleep(0.05)
		msb = self.read(1)
		lsb = self.read(1)
		self.close()

		msbstr = bin(int(binascii.hexlify(msb), 16))[2:].zfill(8)
		lsbstr = bin(int(binascii.hexlify(lsb), 16))[2:].zfill(8)
		 
		tempBinStr = msbstr + lsbstr
		 
		temp = self.binaryToTemp(tempBinStr)

		return temp
    
    # TL value must be in Celsius.
    def setTL(self,tlData):
		#read from TL reg convert to celsius
		hexDataVal = self.decToTwoHexBufZero(tlData)
		hexDataVal = hexDataVal.upper()
		self.open()
		self.write('S')
		self.write('9E')
		self.write('A2')
		self.write(hexDataVal[0:2])
		self.write(hexDataVal[2:4])
		self.write('P')
		self.close()
        
        
    def startConvert(self):
		self.open()
		self.write('S')
		self.write('9E')
		self.write('51')
		time.sleep(0.05)
		self.write('P')
		self.close()
		 
    def stopConvert(self):
		self.open()
		self.write('S')
		self.write('9E')
		self.write('22')
		time.sleep(0.05)
		self.write('P')
		self.close()
    
    # Resolution (res) should only be values from 9-12.
    def setResolution(self, res):
        configVal = self.getConfig()
        newConfig = configVal
        setFlag = 0;
	# set resolution register
	# bits 3,2 in config reg
	# 00b = 9 01b = 10 10b=11 11b = 12
        if res == 9:
            if (ord(configVal) & 0x08) == 0x08:
                newConfig = ord(newConfig) - 0x08
                setFlag = 1
            if (ord(configVal) & 0x04) == 0x04:
                if setFlag == 1:
                    newConfig = newConfig - 0x04
                else:
                    newConfig = ord(configVal) - 0x04
                setFlag = 1

            if setFlag == 1:
                self.setConfig(newConfig)
        if res == 10:
            if (ord(configVal) & 0x08) == 0x08:
                newConfig = ord(newConfig) - 0x08
                setFlag = 1
            if (ord(configVal) & 0x04) == 0:
                if setFlag == 1:
                    newConfig = newConfig + 0x04
                else:
                    newConfig = ord(configVal) + 0x04
                
                setFlag = 1
                
            if setFlag == 1:
                self.setConfig(newConfig)
        if res == 11:
            if (ord(configVal) & 0x08) == 0:
                newConfig = ord(configVal) + 0x08
                setFlag = 1
            if (ord(configVal) & 0x04) == 0x04:
                if setFlag == 1:
                    newConfig = newConfig - 0x04
                else:
                    newConfig = ord(configVal) - 0x04

                setFlag = 1

            if setFlag == 1:
                self.setConfig(newConfig)
        if res == 12:
            if (ord(configVal) & 0x08) == 0:
                newConfig = ord(newConfig) + 0x08
                setFlag = 1
            if (ord(configVal) & 0x04) == 0:
                if setFlag == 1:
                    newConfig = newConfig + 0x04
                else:
                    newConfig = ord(configVal) + 0x04

                setFlag = 1
                
            if setFlag == 1:
                print newConfig
                self.setConfig(newConfig)
	
    # Should pass either True for active high or False for active low.
    # See DS1631 datasheet, pages 10-11.
    def setPolarity(self, pol):
        configVal = self.getConfig()
	     # set bit 1 in config reg
        if pol == True:
            if (ord(configVal) & 0x02) == 0x00:
                newConfig = ord(configVal) + 0x02
                self.setConfig(newConfig)
        if pol == False:
            if (ord(configVal) & 0x02) == 0x02:
                newConfig = ord(configVal) - 0x02
                self.setConfig(newConfig)
		 
	 
    # True for one shot enabled, False for continuous mode.
    def setOneShot(self, oneshot):
	configVal = self.getConfig()
	#set bit 0 in config reg
	if oneshot == True:
            if (ord(configVal) & 0x01) == 0x00:
		newConfig = ord(configVal) + 0x01
		self.setConfig(newConfig)
	if oneshot == False:
	    if (ord(configVal) & 0x01) == 0x01:
		newConfig = ord(configVal) - 0x01
		self.setConfig(newConfig)
		 
    # Returns True for flag set, False otherwise.
    def getTHFlag(self):
	#read from bit 6 in config reg 
        configVal = self.getConfig()
        if (ord(configVal) & 0x40) == 0x40:
            return True
        elif (ord(configVal) & 0x40) == 0x00:
            return False		 
        
    # Returns True for flag set, False otherwise.
    def getTLFlag(self):
	#read from bit 5 in config reg
        configVal = self.getConfig()
        if (ord(configVal) & 0x20) == 0x20:
            return True
        elif (ord(configVal) & 0x20) == 0x00:
            return False
    
    def clearTHFlag(self):
	#set bit 5 in config reg = 0
        configVal = self.getConfig()

        if (ord(configVal) & 0x40) == 0x40:
            newConfig = ord(configVal) - 0x40
            self.setConfig(newConfig)
		 
    def clearTLFlag(self):
	#set bit 5 in config reg = 0
        configVal = self.getConfig()
		 
        if (ord(configVal) & 0x20) == 0x20:
            newConfig = ord(configVal) - 0x20
            self.setConfig(newConfig)
         
    
    # Returns True if conversion complete, false otherwise.
    def isConversionDone(self):
        configVal = self.getConfig()
        if (ord(configVal) & 0x80) == 0x80:
            return True
        elif (ord(configVal) & 0x80) == 0x00:
            return False
        
    # Returns True if write is in progress, false otherwise.
    def isNonVolatileMemoryBusy(self):
	# check bit 4 in config reg
        conf = self.getConfig()
        if (ord(configVal) & 0x10) == 0x10:
            return True
        elif (ord(configVal) & 0x10) == 0x00:
            return False   
        
