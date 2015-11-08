#ifndef __I2C_IFC_H__
#define __I2C_IFC_H__

void I2C_Init(void);
void SCL_Up(void);
void SCL_Down(void);
bit SCL_Read(void);
void SDA_Up(void);
void SDA_Down(void);
bit SDA_Read(void);
void I2CBitDly(void);
bit I2CSendAddr(unsigned char addr, unsigned char rd);
bit I2CSendByte(unsigned char bt);
unsigned char I2CGetByte(unsigned char lastone);
void I2CSendStart(void);
void I2CSendStop(void);

#endif