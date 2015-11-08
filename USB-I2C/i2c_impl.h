// I2C implementation

#include "i2c_ifc.h"

void I2C_Init(void)
{
    IOA = 0xFC;
}

void SCL_Up(void)
{
    SCL_PORT_O &= ~SCL_BIT;
}

void SCL_Down(void)
{
    SCL_PORT_O |= SCL_BIT;
}

bit SCL_Read(void)
{
    return SCL_PORT_I;
}

void SDA_Up(void)
{
    SDA_PORT_O &= ~SDA_BIT;
}

void SDA_Down(void)
{
    SDA_PORT_O |= SDA_BIT;
}

bit SDA_Read(void)
{
    return SDA_PORT_I;
}

void I2CBitDly(void) // wait approximately 10uS
{
	unsigned char time_end;
	for(time_end = 0; time_end<10; time_end++)
	    _nop_( );
	return;
}

void I2CSCLHigh(void) // Set SCL high, and wait for it to go high
{
	register char timeout = 100;
	SCL_Up();
	while (!SCL_Read())
	{
		timeout--;
		if (!timeout)
		{
			return;
		}
	}
}

bit I2CSendAddr(unsigned char addr, unsigned char rd)
{
	I2CSendStart();
	return I2CSendByte(addr|rd); // send address byte
}

bit I2CSendByte(unsigned char bt)
{
	register unsigned char i;
	bit ack;
	for (i=0; i<8; i++)
	{
		if (bt & 0x80) SDA_Up(); // Send each bit, MSB first changed 0x80 to 0x01
		else SDA_Down();
		I2CSCLHigh();
		I2CBitDly();
		SCL_Down();
		I2CBitDly();
		bt = bt << 1;
	}
	SDA_Up(); // Check for ACK
	I2CBitDly();
	I2CSCLHigh();
	I2CBitDly();
	ack = SDA_Read();
    SCL_Down();
	I2CBitDly();
	return ack;
}

unsigned char I2CGetByte(unsigned char lastone) // last one == 1 for last byte; 0 for any other byte
{
	register unsigned char i, res;
	res = 0;
	for (i=0;i<8;i++) // Each bit at a time, MSB first
	{
		I2CSCLHigh();
		I2CBitDly();
		res <<= 1;
		if (SDA_Read()) res |= 1;
		SCL_Down();
		I2CBitDly();
	}
	// Send ACK according to 'lastone'
	if (lastone) SDA_Up();
	else SDA_Down();
	I2CSCLHigh();
	I2CBitDly();
	SCL_Down();
	I2CBitDly();
	SDA_Up();
	I2CBitDly();
	return(res);
}

void I2CSendStart(void)
{
	I2CSCLHigh();
	I2CBitDly();
	SDA_Down();
	I2CBitDly();
	SCL_Down();
	I2CBitDly();
}

void I2CSendStop(void)
{
	SDA_Down();
	I2CBitDly();
	I2CSCLHigh();
	I2CBitDly();
	SDA_Up();
	I2CBitDly();
}