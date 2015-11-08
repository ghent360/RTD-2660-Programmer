/******************************************************************************
* Copyright (2013), Cypress Semiconductor Corporation.
*******************************************************************************
* This software is owned by Cypress Semiconductor Corporation (Cypress) and is
* protected by and subject to worldwide patent protection (United States and
* foreign), United States copyright laws and international treaty provisions.
* Cypress hereby grants to licensee a personal, non-exclusive, non-transferable
* license to copy, use, modify, create derivative works of, and compile the
* Cypress Source Code and derivative works for the sole purpose of creating
* custom software in support of licensee product to be used only in conjunction
* with a Cypress integrated circuit as specified in the applicable agreement.
* Any reproduction, modification, translation, compilation, or representation of
* this software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH
* REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* Cypress reserves the right to make changes without further notice to the
* materials described herein. Cypress does not assume any liability arising out
* of the application or use of any product or circuit described herein. Cypress
* does not authorize its products for use as critical components in life-support
* systems where a malfunction or failure may reasonably be expected to result in
* significant injury to the user. The inclusion of Cypress' product in a life-
* support systems application implies that the manufacturer assumes all risk of
* such use and in doing so indemnifies Cypress against all charges. Use may be
* limited by and subject to the applicable Cypress software license agreement.
*
*******************************************************************************
*  Project Name: A_Hssp_Pioneer
*  Project Revision: 1.00
*  Software Version: PSoC Creator 2.2
*  Device Tested: CY8C5868LTI-LP039
*  Compilers Tested: ARM GCC 4.4.1
*  Related Hardware: PSoC 4 Pioneer Kit (CY8CKIT-042)
*******************************************************************************
******************************************************************************/

/******************************************************************************
* Project Description:
* This is a sample HSSP implmentation demonstrating a PSoC 5LP programming
* PSoC 4. The project is tested using PSoC 4 Pioneer kit with on-board PSoC 5LP
* chip and PSoC 4. PSoC 5LP contains a bootloader. This HSSP project is of type 
* bootloadable and is bootloaded into PSoC 5LP using Bootloader Host tool. To 
* reduce the dependency on LCD, this project uses USBUART to display the output
* on HyperTerminal.
*
* Connections Required:
*   All the pins are pre-connected on the pioneer kit.
*
* To Test this project, Follow these steps:
*	1. Enter PSoC 5P bootloader by powering the kit using USB cable while 
*      pressing and holding reset switch on the kit.
*	2. Bootload this project using Bootloader host tool
*	3. Disconnect and re-connect the USB cable. Install USBUART driver from the 
*	   device manager. Driver files are attached with the project.
*	4. Open and configure the HyperTerminal (8 data bits, 9600 baud rate, 
*      No parity, 1 Stop bit and no hardware control).
*   5. Press any key on keyboard to start HSSP programming.
*   6. "Programming PSoC 4" and "HSSP Success" is displayed on the HyperTerminal. 
*      Red LED on the kit blinks after successful programming.
* 
* ProgramDevice() function in main.c calls all the programming steps 
* sequentially to program PSoC 4.
*
* main.c checks the result of ProgramDevice() function and if it returns 
* SUCCESS, "HSSP Success" is displayed on the HyperTerminal. If ProgramDevice()
* returns failure, Step of failure, Reason of failure is displayed on the LCD.
* If the error is because of SROM, It displays the SROM error code.
******************************************************************************/
#include <device.h> 
#include "ProgrammingSteps.h"

/* MACROS for writing decimal or hexadecimal values on HyperTerminal */
#define DECIMAL 0
#define HEX		1

/* MACROS for converting the nibble to its HEX value */
#define CONVERT_TO_DIGIT	48
#define CONVERT_TO_ALPHA	55

/* Function which sequentially calls all the programming steps */
unsigned char ProgramDevice( void );

/* Function to send a string on HyperTerminal */
void ShowStringOnTerminal( unsigned char *, uint8 );

/* Function to send a value (DECIMAL or HEX) on HyperTerminal */
void ShowValueOnTerminal( unsigned char , uint8 );

/* Global variable which stores the current programming step under execution */
unsigned char currentStep;

void main()
{
	/* Variable to store the result of HSSP operation */
    unsigned char programResult;   
	
	/* Variable to store the HSSP Error status in case of failure */
    unsigned char errorStatus; 
	
	/* Variable to store the SPC Error status if errorStatus contains 
	   SPC_TIMEOUT_ERROR error */
    unsigned char sromErrorStatus;
	
	/* buffer arrays to store the strings */
	unsigned char buf1[] = {"Programming PSoC 4"};
	unsigned char buf2[] = {"HSSP Success"};
	unsigned char buf3[] = {"HSSP Fail Step"};
	unsigned char buf4[] = {"Err"};
	unsigned char buf5[] = {"SROM"};
	
	/* Enable all interrupts */
	CyGlobalIntEnable;
	
	/* Start USBUART Component */
	USBUART_Start(0, USBUART_5V_OPERATION);
	while(!USBUART_GetConfiguration());
	
	/* Initialize USBUART CDC  */
    USBUART_CDC_Init();
	
	/* Wait for an alpha-numeric key press on the keyboard */
	while(USBUART_DataIsReady() == 0);
	
	/* Put "Programming PSoC 4" string on the HyperTerminal */
	ShowStringOnTerminal(buf1, sizeof(buf1));
    
    /* Start the HSSP Programming and store the status */
    programResult = ProgramDevice();
    
	/* HSSP completed successfully */
    if(programResult == SUCCESS)
    {
        /* Put "HSSP Success" on the HyperTerminal */
		ShowStringOnTerminal(buf2, sizeof(buf2));
    }
	/* HSSP operation Fails */
    else
    { 
        /* Display the step number where the HSSP failed */
	   	
		/* Put "HSSP Fail Step" string on the HyperTerminal */
		ShowStringOnTerminal(buf3, sizeof(buf3));
		
		/* Put step number on the HyperTerminal */
		ShowValueOnTerminal(currentStep, DECIMAL);
        		
        /* Get the HSSP error status and display on the HyperTerminal */
        errorStatus = ReadHsspErrorStatus();
        
        /* Put "Err" string on the HyperTerminal */
		ShowStringOnTerminal(buf4, sizeof(buf4));
		
		/* Put error code on the HyperTerminal */
        ShowValueOnTerminal(errorStatus, HEX);
        
        /* If the errorStatus contains SROM_TIMEOUT_ERROR error condition,
           read the Status Code returned by CPUSS_SYSARG register and display
		   on THE LCD */
        if(errorStatus & SROM_TIMEOUT_ERROR)
        {
            /* Read the Status Code returned by SROM */
			sromErrorStatus = ReadSromStatus();
			
			/* Put "SROM" string on the HyperTerminal */
			ShowStringOnTerminal(buf5, sizeof(buf5));
			
			/* Put SROM error code on the HyperTerminal */
			ShowValueOnTerminal(sromErrorStatus, HEX);
        }
    }

    for( ; ; )
    {
        /* Do Nothing */
    }
}

/******************************************************************************
* Function Name: ProgramDevice
*******************************************************************************
* Summary:
*  This function sequentially calls all the functions required to program a
*  PSoC 4. These functions are defined in ProgrammingSteps.h.
*
* Parameters:
*  None.
*
* Return:
*  SUCCESS - All the programming steps executed successfully
*  FAILURE - HSSP programming fails in any one of the programming step
*
******************************************************************************/
unsigned char ProgramDevice(void)
{
    currentStep = 0;
    
    currentStep++;    
    if(DeviceAcquire() == FAILURE)     
        return(FAILURE);
    
    currentStep++;
    if(VerifySiliconId() == FAILURE)    
        return(FAILURE);

    currentStep++;
    if(EraseAllFlash() == FAILURE)             
        return(FAILURE);

    currentStep++;
    if(ChecksumPrivileged() == FAILURE)                    
        return(FAILURE);

    currentStep++;
    if(ProgramFlash() == FAILURE)          
        return(FAILURE);

    currentStep++;
    if(VerifyFlash() == FAILURE)       
        return(FAILURE);

    currentStep++;
    if(ProgramProtectionSettings() == FAILURE) 
        return(FAILURE);

    currentStep++;
    if(VerifyProtectionSettings() == FAILURE)
        return(FAILURE);

    currentStep++;
    if(VerifyChecksum() == FAILURE) 
        return(FAILURE);

    ExitProgrammingMode();
        
	/* All the steps were completed successfully */
    return(SUCCESS);
}

/******************************************************************************
* Function Name: ShowStringOnTerminal
*******************************************************************************
* Summary:
*  This function sends a string to HyperTerminal using USBUART component of
*  PSoC 5LP.
*
* Parameters:
*  1. uint8 * buf - buffer containing the string to display on HyperTerminal
*  2. uint8 size - size of buffer is passed as it is required by the 
*	  			   USBUART_PutData API
*
* Return:
*  None
*
* Note:
*  None
*
******************************************************************************/
void ShowStringOnTerminal(uint8 *buf, uint8 size)
{
	/* Put the string in buffer on the HyperTerminal */
	while(USBUART_CDCIsReady() == 0u);
    USBUART_PutData(buf, size);
	
	/* Go to new line on the HyperTerminal */
	while(USBUART_CDCIsReady() == 0u);
	USBUART_PutCRLF();
}

/******************************************************************************
* Function Name: ShowValueOnTerminal
*******************************************************************************
* Summary:
*  This function sends a value (Decimal or Hex) to HyperTerminal using USBUART
*  component of PSoC 5LP.
*
* Parameters:
*  1. unsigned char value - Value to send using the USBUART component to the
*							HyperTerminal
*  2. uint8 type - This takes two MACRO values only:
*					a) DECIMAL	- To display a decimal value on the HyperTerminal
*					b) HEX		- To display a hexa-decimal value on the
*								  HyperTerminal
*
* Return:
*  None
*
* Note:
*  None
*
******************************************************************************/
void ShowValueOnTerminal(unsigned char value, uint8 type)
{
	uint8 count = 0;
	
	/* If HEX value is required to send to HyperTerminal */
	if (type == HEX)
	{
		/* Copying MSB to the count variable */
		count = (((value & 0xF0) >> 4) & 0x0F);
		
		/* If the MSB contains a digit (0 to 9) */
		if(count < 10)
		{
			while(USBUART_CDCIsReady() == 0u);
			count = count + CONVERT_TO_DIGIT;
		    USBUART_PutData(&count,1);
		}
		/* If the MSB contains a alphabet (A, B, C, D, E, F) */
		else
		{
			while(USBUART_CDCIsReady() == 0u);
			count = count + CONVERT_TO_ALPHA;
		    USBUART_PutData(&count,1);
		}
		
		/* Copying LSB to the count variable */
		count = value & 0x0F;
		
		/* If the LSB contains a digit (0 to 9)*/
		if(count < 10)
		{
			while(USBUART_CDCIsReady() == 0u);
			count = count + CONVERT_TO_DIGIT;
		    USBUART_PutData(&count,1);
		}
		/* If the LSB contains a alphabet (A, B, C, D, E, F) */
		else
		{
			while(USBUART_CDCIsReady() == 0u);
			count = count + CONVERT_TO_ALPHA;
		    USBUART_PutData(&count,1);
		}
	}
	
	/* If DECIMAL value is required to send to HyperTerminal */
	if (type == DECIMAL)
	{
		/* If value is a one digit number */
		if(value/10 == 0)
		{
			while(USBUART_CDCIsReady() == 0u);
			count = value + CONVERT_TO_DIGIT;
		    USBUART_PutData(&count,1);
			
			while(USBUART_CDCIsReady() == 0u);
			USBUART_PutCRLF();
		}
		else /* If value is a two digit number */
		{
			while(USBUART_CDCIsReady() == 0u);
			count = (value / 10) + CONVERT_TO_DIGIT;
		    USBUART_PutData(&count,1);
			
			while(USBUART_CDCIsReady() == 0u);
			USBUART_PutCRLF();
			
			while(USBUART_CDCIsReady() == 0u);
			count = (value % 10) + CONVERT_TO_DIGIT;
		    USBUART_PutData(&count,1);
			
			while(USBUART_CDCIsReady() == 0u);
			USBUART_PutCRLF();
		}
	}
}


/* [] End of File */
