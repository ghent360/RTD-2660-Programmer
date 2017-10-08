Realtek RTD2660/2662 programmer
===============================

Note this project is quite old. There have been sevaral ports to different hardware or languages (in no particular order):
+ Linux port using standard i2c drivers <https://github.com/mushketer888/pyrtd2660i2c>
+ Python port for FT232H <https://github.com/juancgarcia/RTD-2660-Programmer-Python>
+ Port for arduino <https://github.com/adafruit/Adafruit_RTD266X_I2CFlasher>
+ Another linux port using VGA output <https://github.com/static-void/rtd266x_programmer>
+ FTDI MPSSE port <https://github.com/maximus64/RTD-2660-Programmer-FTDI>
+ Another python port <https://github.com/luisfcorreia/rtd2660>
+ Port to python and raspberry Pi <https://github.com/klirichek/RTD-2660-Programmer>

##Firmware programmer for LCD controllers based on the RTD2660 or RTD2662 chips.

The Realtek RTD2662 chip sometimes mislabeled as RTD2660 is found in many cheap LCD controller boards, most popular seems to be the PCB800099. In order to support different LCD panel resolutions, one has to load the "correct" firmware on the board.

If you search around the net you can find many different firmware images for this controller. I found that loading the firmware into the board was not for the weak of heart and requires some shady tools from eBay.

Here is a project how to build a programming tool for less than $15.

To build the programmer you would need a basic FX2LP device. One from amazon or ebay based on CY7C68013A would do. Install the FX2LP SDK from Cypress and use the Cypress Control Center tool to flash the i2c.iic file from the USB-I2C folder on the FX2LP device.

Many other hardware devices can be used to program the LCD controller - the protocol is based on the i2c standard.

Connect:
```
    FX2LP       VGA Port
    Device      Target
    ------      ---------------
    PA0    ---> Pin 15 DDC SCL
    PA1    ---> Pin 12 DDC SDA
    GND    ---> GND (choose any of the white ports, not the VGA connector itself)
```
You would need to power the target board from a 12V or 5V power supply.

Use the PC software to flash a .bin file to the target or to read the content of the existing flash. Saving a copy of the existing software is recommended.

The code may require some modification if your LCD controller is using different SPI flash chip to store the firmware. I've only tested this code with Winbond SPI flash chips.

Building the software:

There is a Windows PC project for visual studio 2010. Multiplatform support is pending.

The FX2LP device software needs the Keil PK51 toolchain.  It is a very simple firmware that implements i2c protocol and can be commanded via USB. There is a compiled binary i2c.iic you can upload to the FX2LP device as well.
