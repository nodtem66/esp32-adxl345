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

#include "Wire.h" 
#include "ADXL345.h"

AccelerometerRaw ADXL345::ReadRawAxis()
{
	uint8_t buffer[6];
	Read(Register_DataX, 6, buffer);
	AccelerometerRaw raw = AccelerometerRaw();
	raw.XAxis = SIGNED(((buffer[1] & highbit_mask) << 8) | buffer[0], signed_mask);
	raw.YAxis = SIGNED(((buffer[3] & highbit_mask) << 8) | buffer[2], signed_mask);
	raw.ZAxis = SIGNED(((buffer[5] & highbit_mask) << 8) | buffer[4], signed_mask);
	return raw;
}

AccelerometerScaled ADXL345::ReadScaledAxis()
{
	AccelerometerRaw raw = ReadRawAxis();
	AccelerometerScaled scaled = AccelerometerScaled();
	scaled.XAxis = raw.XAxis * m_Scale;
	scaled.YAxis = raw.YAxis * m_Scale;
	scaled.ZAxis = raw.ZAxis * m_Scale;
	return scaled;
}

uint8_t ADXL345::EnableMeasurements()
{
	return Write(Register_PowerControl, 0x08);
}

uint8_t ADXL345::SetRange(int range, bool fullResolution)
{
	// Get current data from this register.
	uint8_t data = 0;
	Read(Register_DataFormat, 1, &data);

	// We AND with 0xF4 to clear the bits are going to set.
	// Clearing ----X-XX
	//data &= 0xF4;

	// By default (range 2) or FullResolution = true, scale is 2G.
	m_Scale = ScaleFor2G;
	signed_mask = 1 << 9;
	highbit_mask = 0x03;
	
	// Set the range bits.
	switch(range)
	{
		case 2:
			break;
		case 4:
			data |= 0x01;
			if(!fullResolution) { m_Scale = ScaleFor4G; }
			else { signed_mask = 1 << 10; highbit_mask = 0x07;}
			break;
		case 8:
			data |= 0x02;
			if(!fullResolution) { m_Scale = ScaleFor8G; }
			else { signed_mask = 1 << 11; highbit_mask = 0x0f;}
			break;
		case 16:
			data |= 0x03;
			if(!fullResolution) { m_Scale = ScaleFor16G; }
			else { signed_mask = 1 << 12; highbit_mask = 0x1f;}
			break;
		default:
			return ErrorCode_1_Num;
	}

	// Set the full resolution bit.
	if(fullResolution)
		data |= 0x08;

	return Write(Register_DataFormat, data);
}

void ADXL345::SetOffset(double x, double y, double z)
{
	Write(Register_OFSX, (int8_t)(x*64.10256));
	Write(Register_OFSY, (int8_t)(y*64.10256));
	Write(Register_OFSZ, (int8_t)(z*64.10256));
}

uint8_t ADXL345::EnsureConnected()
{
	uint8_t data;
	Read(0x00, 1, &data);
	
	if(data == 0xE5)
		IsConnected = true;
	else
		IsConnected = false;

	return IsConnected;
}

uint8_t ADXL345::Write(int address, int data)
{
	Wire.beginTransmission(m_Address);
	Wire.write(address);
	Wire.write(data);
	return Wire.endTransmission();
}

uint8_t ADXL345::Read(int address, int length, uint8_t* buffer)
{
	Wire.beginTransmission(m_Address);
	Wire.write(address);
	Wire.endTransmission();
  
	Wire.beginTransmission(m_Address);
	Wire.requestFrom(m_Address, length);

	if(Wire.available() == length)
	{
		for(uint8_t i = 0; i < length; i++)
		{
			buffer[i] = Wire.read();
		}
	}
	return Wire.endTransmission();
}

const char* ADXL345::GetErrorText(int errorCode)
{
	if(ErrorCode_1_Num == 1)
		return ErrorCode_1;
	
	return "Error not defined.";
}