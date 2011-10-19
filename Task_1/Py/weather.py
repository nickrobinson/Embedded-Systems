#!/usr/bin/python
# -*- coding: utf-8 -*-

# Base error class
class Error(Exception):

    pass


class temperature:

	# OutOfRangeError class derived from base class Error
    class OutOfRangeError(Error):

        pass

	# Supply absolute zero values to avoid magic numbers
    absZeroCelsius = -273.15
    absZeroFahrenheit = -459.67

	# Initialize temperature class and perform error checking on passed temp values
    def __init__(self, celsius=0, fahrenheit=0):
        if celsius < self.absZeroCelsius or fahrenheit < self.absZeroFahrenheit:
            raise self.OutOfRangeError, \
                "number out of range (must be above absolute zero)"
        else:
            self.tempCelsius = celsius
            self.tempFahrenheit = fahrenheit

    def fahrenheit(self):
        if self.tempCelsius < self.absZeroCelsius:
            raise self.OutOfRangeError, \
                "number out of range (must be above absolute zero)"
        self.tempFahrenheit = (9.0 / 5.0) * self.tempCelsius + 32
        return self.tempFahrenheit

    def celsius(self):
        if self.tempFahrenheit < self.absZeroFahrenheit:
            raise OutOfRangeError, \
                "number out of range (must be above absolute zero)"
        self.tempCelsius = (self.tempFahrenheit - 32) * (5.0 / 9.0)
        return self.tempCelsius


