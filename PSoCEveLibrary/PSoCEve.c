/*******************************************************************************
* File Name: PSoCEve.c 
* Version 0.1 Alpha
*
* Description:
*  PSoCEveLibrary project.
*  Library to easy the use of the FT EVE graphic chips from FTDI.
*  Actually in development and in a very early stage there is support only
*  for the FT800 chip with Cypress PSOC4 family microcontrollers.
*  It is planned to had support for FT801 and newer TF81x chips, also for
*  other PSOC families (PSOC3 and PSCO5).
*
* Note:
*
********************************************************************************
* Copyright (c) 2015 Jesús Rodríguez Cacabelos
* This library is dual licensed under the MIT and GPL licenses.
* http:
*******************************************************************************/

#include <project.h>
#include "PSoCEve.h"
#include "PSoCEve_Hal.h"

//enum LISTTYPE listInProgress = NONE;
unsigned int ramPtr;
unsigned int ramCMDOffset;

uint8 listInProgress = NONE;


unsigned char EVE_CoPro_IsReady(unsigned int *newoffset);



uint8 FTIsCoproccesorReady()
{
    return EVE_CoPro_IsReady(&ramCMDOffset);
}

/*******************************************************************************
*   Display list functions.
*******************************************************************************/ 
    
void DLStartList()
{
    ramPtr = RAM_DL;
    SPI_Transfer_Start(ramPtr  | MEMORY_WRITE);
}

void DLListNewItem(unsigned long item)
{
    SPI_Transfer_Write_Long(item);
}

void DLEndList()
{
    SPI_Transfer_Write_Long(DL_DISPLAY);
    SPI_Transfer_End();
    mEVE_Register_Write(REG_DLSWAP, DLSWAP_FRAME);
}

void CMDStartList()
{
    while (!EVE_CoPro_IsReady(&ramCMDOffset)) {}
    
    SPI_Transfer_Start((RAM_CMD + ramCMDOffset) | MEMORY_WRITE); // Start the display list
    SPI_Transfer_Write_Long(CMD_DLSTART);
    ramCMDOffset += 4;
}



void CMDEndList(unsigned char swap)
{
    if (swap)
    {
        SPI_Transfer_Write_Long(DL_DISPLAY); 
        ramCMDOffset += 4;
        SPI_Transfer_Write_Long(CMD_SWAP); 
        ramCMDOffset += 4;
    }
    
    SPI_Transfer_End();
    EVE_Memory_Write_Word(REG_CMD_WRITE, (ramCMDOffset));
}

void CMDListAddItem(unsigned char *tobesent, unsigned int length, unsigned char *string)
{
    unsigned char *cptr = string;
    
    SPI_TransferL_Write_ByteArray(tobesent, length);
    ramCMDOffset += length;
    
    if (string != 0)
    {
        while (*cptr != 0)
        {
            SPI_TransferL_Write_Byte(*cptr);
            ramCMDOffset++;
            cptr++;
        } 
    
        SPI_TransferL_Write_Byte(0);
        ramCMDOffset++;
    
        while ((ramCMDOffset % 4) != 0)
        {
            SPI_TransferL_Write_Byte(0);
            ramCMDOffset++;
        }
    }
}

void FT_Write_ByteArray_4(const unsigned char *data, unsigned long length)
{
    SPI_TransferL_Write_ByteArray(data, length);
    ramCMDOffset += length;
    
    while ((ramCMDOffset % 4) != 0)
    {
        SPI_TransferL_Write_Byte(0);
        ramCMDOffset++;
    }
}

void CMDListAddDLItem(unsigned long item)
{
    SPI_Transfer_Write_Long(item); 
    ramCMDOffset += 4;
}










unsigned char EVE_CoPro_IsReady(unsigned int *newoffset)
{
    unsigned int cmdbufferrd, cmdbufferwr;
    
    cmdbufferrd = EVE_Memory_Read_Word(REG_CMD_READ);		// Read the graphics processor read pointer.
	cmdbufferwr = EVE_Memory_Read_Word(REG_CMD_WRITE); 	    // Read the graphics processor write pointer.
    
    if (cmdbufferrd != cmdbufferwr) return 0;               // If both are equal, processor have finished
    else                                                    //    processing previous list.
    {
        *newoffset = cmdbufferwr;
        return 1;                  
    }
}




/******************************************************************************
 * Function:        void incCMDOffset(currentOffset, commandSize)
 * PreCondition:    None
 *                    starting a command list
 * Input:           currentOffset = graphics processor command list pointer
 *                  commandSize = number of bytes to increment the offset
 * Output:          newOffset = the new ring buffer pointer after adding the command
 * Side Effects:    None
 * Overview:        Adds commandSize to the currentOffset.  
 *                  Checks for 4K ring-buffer offset roll-over 
 * Note:            None
 *****************************************************************************/
unsigned int IncCMDOffset(unsigned int currentoffset, unsigned char commandsize)
{
    unsigned int newOffset = currentoffset + commandsize;
    
    if(newOffset > 4095) newOffset = (newOffset - 4096);
    
    return newOffset;
}









/*******************************************************************************
*   General Functions.
*******************************************************************************/

unsigned char EVE_Init_Display()
{

    
    
    
    
    // Force a hardware reset of EVE chip using PD_N pin.
    PD_N_Write(0); CyDelay(10); PD_N_Write(1); CyDelay(20);
    
    // Initialize EVE chip. Max SPI speed before the chip is initialized is 11Mhz.
    FTCommandWrite(FT800_ACTIVE);            // Start FT800
    CyDelay(5);	
    FTCommandWrite(FT800_CLKEXT);			// Set FT800 for external clock
    CyDelay(5);	
    FTCommandWrite(FT800_CLK48M);			// Set FT800 for 48MHz PLL
    CyDelay(5);	
    FTCommandWrite(FT800_CORERST);			// Set FT800 for 48MHz PLL
    CyDelay(5);
    
    // After initialization, EVE chip accept commands at up to 30Mhz clock on SPI bus.
    
    // Read ID register. If we don¨t get 0x7C something is bad.

    if (EVE_Memory_Read_Byte(REG_ID) != 0x7C) return 0;
    
    // At startup, PCLK (pixel clock) and PWM_DUTY (used for backlight) are programmed to 0.
    //      Display is off until user turns it on.
    
    mEVE_Register_Write(REG_PCLK, 0);
    mEVE_Register_Write(REG_PWM_DUTY, 0);	
    
    // Continue initializing registers with values from configuration header file.
    
    mEVE_Register_Write(REG_HSIZE, LCDWIDTH);	
    mEVE_Register_Write(REG_VSIZE, LCDHEIGHT);
    mEVE_Register_Write(REG_HCYCLE, LCDHCYCLE);
    mEVE_Register_Write(REG_HOFFSET, LCDHOFFSET);
    mEVE_Register_Write(REG_HSYNC0, LCDHSYNC0);
    mEVE_Register_Write(REG_HSYNC1, LCDHSYNC1);
    mEVE_Register_Write(REG_VCYCLE, LCDVCYCLE);
    mEVE_Register_Write(REG_VOFFSET, LCDVOFFSET);
    mEVE_Register_Write(REG_VSYNC0, LCDVSYNC0);
    mEVE_Register_Write(REG_VSYNC1, LCDVSYNC1);
    mEVE_Register_Write(REG_SWIZZLE, LCDSWIZZLE);
    mEVE_Register_Write(REG_PCLK_POL, LCDPCLKPOL);
    
    // Touch configuration - configure the resistance value to 1200 - this value is specific 
    //      to customer requirement and derived by experimentation.
    mEVE_Register_Write(REG_TOUCH_RZTHRESH, 1200);
        
    return 1;
}

void EVE_Display_ON()
{
    unsigned char gpio = EVE_Memory_Read_Byte(REG_GPIO);    // Read actual value of GPIO register.

    mEVE_Register_Write(REG_GPIO, (gpio | 0x80));			// Set bit 7 of GPIO register (DISP signal).
    mEVE_Register_Write(REG_PCLK, LCDPCLK);			    // Start clock.
}

void EVE_Display_OFF()
{
    unsigned char gpio = EVE_Memory_Read_Byte(REG_GPIO);    // Read actual value of GPIO register.

    mEVE_Register_Write(REG_GPIO, (gpio & 0x70));			// Clear bit 7 of GPIO register (DISP signal).
    mEVE_Register_Write(REG_PCLK, LCDPCLK);			    // Stop clock.
}







void FT_Touch_Enable()
{
    mEVE_Register_Write(REG_TOUCH_MODE, TOUCHMODE_FRAME);
    mEVE_Register_Write(REG_TOUCH_RZTHRESH, 1200);    
}

void FT_Touch_Disable()
{
    mEVE_Register_Write(REG_TOUCH_MODE, 0);
    mEVE_Register_Write(REG_TOUCH_RZTHRESH, 0);    
}

void FT_Touch_Calibrate()
{
    CMDStartList(); 
    CMDListAddDLItem(DLClearColorRGB(0x00, 0x00, 0x00));
    CMDListAddDLItem(DLClear(1, 1, 1));
    CMDListAddItem(CMDCalibrate());
    CMDEndList(END_DL_SWAP);  
    
    while (!FTIsCoproccesorReady()) {};
}

void FT_Touch_ReadCalibrationValues(TouchCalibrationValues* values)
{
    uint8 loop;
    uint32 ptr = REG_TOUCH_TRANSFORM_A;
        
    for (loop = 0; loop < 6; loop++)
    {
        values->TouchTransform_X[loop] = FTMemoryReadUint32(ptr);
        ptr += 4;
    }
}

void FT_Touch_WriteCalibrationValues(TouchCalibrationValues* values)
{
    uint8 loop;
    uint32 ptr = REG_TOUCH_TRANSFORM_A;
    
    for (loop = 0; loop < 6; loop++)
    {
        mEVE_Register_Write(ptr, values->TouchTransform_X[loop]);
        ptr += 4;
    }
}








void EVE_Touch_Enable()
{
    mEVE_Register_Write(REG_TOUCH_MODE, TOUCHMODE_FRAME);
    mEVE_Register_Write(REG_TOUCH_RZTHRESH, 1200);
}

void EVE_Touch_Disable()
{
    mEVE_Register_Write(REG_TOUCH_MODE, 0);
    mEVE_Register_Write(REG_TOUCH_RZTHRESH, 0);
}

void EVE_Touch_Calibrate()
{
//    unsigned int cmdoffset;
//    
//    while (!EVE_Is_Copro_Ready(&cmdoffset)) {}
//    
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (CMD_DLSTART));// Start the display list
//	cmdoffset += 4;
////    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_CLEAR_RGB | 0x0000FFUL));																									// Set the default clear color to black
//    cmdoffset += 4;// Update the command pointer
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG));																// Clear the screen - this and the previous prevent artifacts between lists
//    cmdoffset += 4;
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (CMD_CALIBRATE));
//    cmdoffset += 4;	
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_END));
//    cmdoffset += 4;
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_DISPLAY));
//    cmdoffset += 4;
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (CMD_SWAP));
//    cmdoffset += 4;
//    EVE_Memory_Write_Word(REG_CMD_WRITE, (cmdoffset));   
//    
//    EVE_Is_Copro_Ready(&cmdoffset);
}












//
//unsigned char EVE_Init_Display()
//{
//    unsigned long tvalue;
//    
//    
//    
//    
//    // Force a hardware reset of EVE chip using PD_N pin.
//    PD_N_Write(0); CyDelay(10); PD_N_Write(1); CyDelay(20);
//    
//    // Initialize EVE chip. Max SPI speed before the chip is initialized is 11Mhz.
//    EVE_CommandWrite(FT800_ACTIVE);            // Start FT800
//    CyDelay(5);	
//    EVE_CommandWrite(FT800_CLKEXT);			// Set FT800 for external clock
//    CyDelay(5);	
//    EVE_CommandWrite(FT800_CLK48M);			// Set FT800 for 48MHz PLL
//    CyDelay(5);	
//    EVE_CommandWrite(FT800_CORERST);			// Set FT800 for 48MHz PLL
//    CyDelay(5);
//    
//    // After initialization, EVE chip accept commands at up to 30Mhz clock on SPI bus.
//    
//    // Read ID register. If we don¨t get 0x7C something is bad.
//
//    
//    if (EVE_RegisterRead(REG_ID) != 0x7C) return 0;
//    
//    // At startup, PCLK (pixel clock) and PWM_DUTY (used for backlight) are programmed to 0.
//    //      Display is off until user turns it on.
//    
//    EVE_RegisterWrite(REG_PCLK, 0);
//    EVE_RegisterWrite(REG_PWM_DUTY, 0);	
//    
//    // Continue initializing registers with values from configuration header file.
//    
//    EVE_RegisterWrite(REG_HSIZE, LCDWIDTH);	
//    EVE_RegisterWrite(REG_VSIZE, LCDHEIGHT);
//    EVE_RegisterWrite(REG_HCYCLE, LCDHCYCLE);
//    EVE_RegisterWrite(REG_HOFFSET, LCDHOFFSET);
//    EVE_RegisterWrite(REG_HSYNC0, LCDHSYNC0);
//    EVE_RegisterWrite(REG_HSYNC1, LCDHSYNC1);
//    EVE_RegisterWrite(REG_VCYCLE, LCDVCYCLE);
//    EVE_RegisterWrite(REG_VOFFSET, LCDVOFFSET);
//    EVE_RegisterWrite(REG_VSYNC0, LCDVSYNC0);
//    EVE_RegisterWrite(REG_VSYNC1, LCDVSYNC1);
//    EVE_RegisterWrite(REG_SWIZZLE, LCDSWIZZLE);
//    EVE_RegisterWrite(REG_PCLK_POL, LCDPCLKPOL);
//    
//    // Touch configuration - configure the resistance value to 1200 - this value is specific 
//    //      to customer requirement and derived by experimentation.
//    EVE_RegisterWrite(REG_TOUCH_RZTHRESH, 1200);
//        
//    return 1;
//}
//
//void EVE_Display_ON()
//{
//    unsigned char gpio = EVE_RegisterRead(REG_GPIO);    // Read actual value of GPIO register.
//
//    EVE_RegisterWrite(REG_GPIO, (gpio | 0x80));			// Set bit 7 of GPIO register (DISP signal).
//    EVE_RegisterWrite(REG_PCLK, LCDPCLK);			    // Start clock.
//}
//
//void EVE_Display_OFF()
//{
//    unsigned char gpio = EVE_RegisterRead(REG_GPIO);    // Read actual value of GPIO register.
//
//    EVE_RegisterWrite(REG_GPIO, (gpio & 0x70));			// Clear bit 7 of GPIO register (DISP signal).
//    EVE_RegisterWrite(REG_PCLK, LCDPCLK);			    // Stop clock.
//}
//
//void EVE_Touch_Enable()
//{
//    EVE_RegisterWrite(REG_TOUCH_MODE, TOUCHMODE_FRAME);
//    EVE_RegisterWrite(REG_TOUCH_RZTHRESH, 1200);
//}
//
//void EVE_Touch_Disable()
//{
//    EVE_RegisterWrite(REG_TOUCH_MODE, 0);
//    EVE_RegisterWrite(REG_TOUCH_RZTHRESH, 0);
//}


//void EVE_StartList(enum LISTTYPE list)
//{
//    if (list == DISPLAY) ramPtr = RAM_DL;
//    else if (list == COPROCESSOR) ramPtr = RAM_CMD;
//    
//    listInProgress = list;
//    
//    SPI_Transfer_Start(ramPtr  | MEMORY_WRITE);
//}

//void EVE_ListNewItem(unsigned long item)
//{
//    SPI_Transfer_Write_Long(item);
//}

//void EVE_EndList(unsigned char swap)
//{
//    if (listInProgress == COPROCESSOR)
//    {
//        if (swap)
//        {
//            SPI_Transfer_Write_Long(DL_DISPLAY); 
//            ramCMDOffset += 4;
//            SPI_Transfer_Write_Long(CMD_SWAP); 
//            ramCMDOffset += 4;
//        }
//        
//        SPI_Transfer_End();
//        EVE_Memory_Write_Word(REG_CMD_WRITE, (ramCMDOffset));
//    }
//    else if (listInProgress == DISPLAY)
//    {
//        SPI_Transfer_Write_Long(DL_DISPLAY);
//        SPI_Transfer_End();
//        EVE_RegisterWrite(REG_DLSWAP, DLSWAP_FRAME);
//    }
//}

/******************************************************************************
 * Function:        void ft800cmdWrite(ftCommand)
 * PreCondition:    None
 * Input:           ftCommand
 * Output:          None
 * Side Effects:    None
 * Overview:        Sends FT800 command
 * Note:            None
 *****************************************************************************/
//void EVE_CommandWrite(unsigned char command)
//{
//    unsigned char tobesent[3] = {command, 0x00, 0x00};
//
//    SPI_Transfer_StartSS(); 
//    SPI_Transfer_Write_Array(tobesent, 3); 
//    SPI_Transfer_End();
//}
//
//void EVE_RegisterWrite(unsigned long everegister, unsigned long data)
//{
//    SPI_Transfer_Start(everegister | MEMORY_WRITE); 
//    SPI_Transfer_Write_Long(data); 
//    SPI_Transfer_End();
//}
//
//unsigned char EVE_RegisterRead(unsigned long everegister)
//{
//    unsigned long retdata;
//    
//    SPI_Transfer_StartRead(everegister | MEMORY_READ); 
//    retdata = SPI_Transfer_Read_Byte(); 
//    SPI_Transfer_End();    
//    
//    return retdata;
//}


















/* ***** GENERAL ***** */
/* ------------------- */
                                    
//unsigned char EVE_Init_Display()
//{
//    PD_N_Write(0);
//    CyDelay(10);
//    PD_N_Write(1);
//    CyDelay(20);
//    
//    
//	
//    EVE_Command_Write(FT800_ACTIVE);            // Start FT800
//    CyDelay(5);	
//    EVE_Command_Write(FT800_CLKEXT);			// Set FT800 for external clock
//    CyDelay(5);	
//    EVE_Command_Write(FT800_CLK48M);			// Set FT800 for 48MHz PLL
//    CyDelay(5);	
//    EVE_Command_Write(FT800_CORERST);			// Set FT800 for 48MHz PLL
//    CyDelay(5);
//    
//    /* After initialization, EVE chip accept commands at up to 30Mhz clock on SPI bus. */
//    
//    // Read ID register. If we don¨t get 0x7C something is bad.
//
//    if (EVE_Memory_Read_Byte(REG_ID) != 0x7C)
//    {
//        return 0;
//    }
//    
//    EVE_Memory_Write_Byte(REG_PCLK, ZERO);	    // Set PCLK to zero - don't clock the LCD until later
//    EVE_Memory_Write_Byte(REG_PWM_DUTY, ZERO);	
//
//    EVE_Memory_Write_Word(REG_HSIZE, LCDWIDTH);	
//    EVE_Memory_Write_Word(REG_VSIZE, LCDHEIGHT);
//    EVE_Memory_Write_Word(REG_HCYCLE, LCDHCYCLE);
//    EVE_Memory_Write_Word(REG_HOFFSET, LCDHOFFSET);
//    EVE_Memory_Write_Word(REG_HSYNC0, LCDHSYNC0);
//    EVE_Memory_Write_Word(REG_HSYNC1, LCDHSYNC1);
//    EVE_Memory_Write_Word(REG_VCYCLE, LCDVCYCLE);
//    EVE_Memory_Write_Word(REG_VOFFSET, LCDVOFFSET);
//    EVE_Memory_Write_Word(REG_VSYNC0, LCDVSYNC0);
//    EVE_Memory_Write_Word(REG_VSYNC1, LCDVSYNC1);
//    EVE_Memory_Write_Word(REG_SWIZZLE, LCDSWIZZLE);
//    EVE_Memory_Write_Word(REG_PCLK_POL, LCDPCLKPOL);
//    
//    /* Touch configuration - configure the resistance value to 1200 - this value is specific to customer requirement and derived by experiment */
//    EVE_Memory_Write_Word(REG_TOUCH_RZTHRESH, 1200);
//        
//    return 1;
//
//}
//
//void EVE_Display_ON()
//{
//    unsigned char gpio = EVE_Memory_Read_Byte(REG_GPIO);    // Read actual value of GPIO register.
//
//    EVE_Memory_Write_Byte(REG_GPIO, (gpio | 0x80));			// Set bit 7 of GPIO register (DISP signal).
//    EVE_Memory_Write_Byte(REG_PCLK, LCDPCLK);			    // Start clock.
//}
//
//void EVE_Display_OFF()
//{
//    unsigned char gpio = EVE_Memory_Read_Byte(REG_GPIO);    // Read actual value of GPIO register.
//
//    EVE_Memory_Write_Byte(REG_GPIO, (gpio & 0x70));			// Clear bit 7 of GPIO register (DISP signal).
//    EVE_Memory_Write_Byte(REG_PCLK, LCDPCLK);			    // Stop clock.
//}
//
//void EVE_Touch_Enable()
//{
//    EVE_Memory_Write_Byte(REG_TOUCH_MODE, TOUCHMODE_FRAME);
//    EVE_Memory_Write_Word(REG_TOUCH_RZTHRESH, 1200);
//}
//
//void EVE_Touch_Disable()
//{
//    EVE_Memory_Write_Byte(REG_TOUCH_MODE, 0);
//    EVE_Memory_Write_Word(REG_TOUCH_RZTHRESH, 0);
//}
//
//void EVE_Touch_Calibrate()
//{
//    unsigned int cmdoffset;
//    
//    while (!EVE_Is_Copro_Ready(&cmdoffset)) {}
//    
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (CMD_DLSTART));// Start the display list
//	cmdoffset += 4;
////    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_CLEAR_RGB | 0x0000FFUL));																									// Set the default clear color to black
//    cmdoffset += 4;// Update the command pointer
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG));																// Clear the screen - this and the previous prevent artifacts between lists
//    cmdoffset += 4;
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (CMD_CALIBRATE));
//    cmdoffset += 4;	
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_END));
//    cmdoffset += 4;
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (DL_DISPLAY));
//    cmdoffset += 4;
//    EVE_Memory_Write_Long(RAM_CMD + cmdoffset, (CMD_SWAP));
//    cmdoffset += 4;
//    EVE_Memory_Write_Word(REG_CMD_WRITE, (cmdoffset));   
//    
//    EVE_Is_Copro_Ready(&cmdoffset);
//}
//



/* USER CODE BEGIN 4 */


/******************************************************************************
 * Function:        void incCMDOffset(currentOffset, commandSize)
 * PreCondition:    None
 *                    starting a command list
 * Input:           currentOffset = graphics processor command list pointer
 *                  commandSize = number of bytes to increment the offset
 * Output:          newOffset = the new ring buffer pointer after adding the command
 * Side Effects:    None
 * Overview:        Adds commandSize to the currentOffset.  
 *                  Checks for 4K ring-buffer offset roll-over 
 * Note:            None
 *****************************************************************************/
//unsigned int IncCMDOffset(unsigned int currentoffset, unsigned char commandsize)
//{
//    unsigned int newOffset = currentoffset + commandsize;
//    
//    if(newOffset > 4095) newOffset = (newOffset - 4096);
//    
//    return newOffset;
//}





