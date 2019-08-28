/*
ADXL345.h - Header file for the ADXL345 Triple Axis Accelerometer Arduino Library.
Copyright (C) 2011 Love Electronics (loveelectronics.co.uk)

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef ADXL345_h
#define ADXL345_h

#define ADXL345_Address 0x53

#define Register_PowerControl 0x2D
#define Register_DataFormat 0x31
#define Register_DataX 0x32
#define Register_DataY 0x34
#define Register_DataZ 0x36
#define Register_OFSX 0x1E
#define Register_OFSY 0x1F
#define Register_OFSZ 0x20

#define ErrorCode_1 "Entered range was invalid. Should be 2, 4, 8 or 16g."
#define ErrorCode_1_Num 1

#define ScaleFor2G 0.00390625
#define ScaleFor4G 0.0078125
#define ScaleFor8G 0.015625
#define ScaleFor16G 0.03125
// SIGNED macro used for conversion between raw uint16 to signed number
// see: https://en.wikipedia.org/wiki/Two%27s_complement
#define SIGNED(number, mask) (-((number) & (mask)) + ((number) & ~(mask)))
//#define SIGNED(number, mask) (number)

struct AccelerometerScaled
{
	double XAxis;
	double YAxis;
	double ZAxis;
};

struct AccelerometerRaw
{
	int16_t XAxis;
	int16_t YAxis;
	int16_t ZAxis;
};

class ADXL345
{
	public:
	  ADXL345(uint8_t address=ADXL345_Address):m_Address(address) {}

	  AccelerometerRaw ReadRawAxis();
	  AccelerometerScaled ReadScaledAxis();
  
	  uint8_t SetRange(int range, bool fullResolution);
	  void SetOffset(double x, double y, double z);
	  uint8_t EnableMeasurements();

	  const char* GetErrorText(int errorCode);
	  
	  uint8_t EnsureConnected();
	  
	  uint8_t IsConnected;
	  
	//protected:
	  uint8_t Write(int address, int byte);
	  uint8_t Read(int address, int length, uint8_t* buffer);

	private:
	  int m_Address;
	  uint16_t signed_mask;
	  uint8_t highbit_mask;
	  float m_Scale;
};
#endif