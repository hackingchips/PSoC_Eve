/*******************************************************************************
* Description:
*  This file belongs to the PSoCEve library project.
*  Used for testing features of PSocEve library during development.
*
* Note:
*
********************************************************************************
* Copyright (c) 2015, 2016 Jesús Rodríguez Cacabelos
* This library is dual licensed under the MIT and GPL licenses.
* http:
*******************************************************************************/

#include <project.h>

#include "..\..\..\PSoC_Eve_Library\PsoCEve.h"
#include "demo_2.h"
#include "Demos_Resources.h"

// storage for callback.
static void (*TouchCallback)(DEMO_2_EVENTS button) = 0;

/* *** Function prototypes. ***************************************************
*/
void Demo_2_Screen();
void Demo_2_Loop(); 

/* *** Demo initialization. ***************************************************
*/
void* Demo_2_Start(void (*touchcallback)(DEMO_2_EVENTS button), void (**closefunction)())
{
    /* Paint screen contents. */
    Demo_2_Screen();
    
    /* Store callback pointer. */
    TouchCallback = touchcallback;
    
    /* Return address of this demo main loop function. Also for closing function. */
    closefunction = 0;        // This demo, doesn´t need a closing function.  
    return Demo_2_Loop;
}

/* *** Demo main loop. To be called from the program main loop. ***************
*/
void Demo_2_Loop()
{
    // Here we read the TAG register from EVE chip to know if a button is pressed.
    uint8 rdtag = FT_Read_Byte(REG_TOUCH_TAG);
    
    // Call the callback function with value of TAG.
    if (rdtag == D2_BTN_EXIT)
        (*TouchCallback)(rdtag);
}


/* *** Paint contents of screen. **********************************************
*/
void Demo_2_Screen()
{
     /* ***** Block 1. ***** */
    // First upload font data to FT Chip.
    // At RAM-G, offset = 0. Font metric data.
    // At RAM-G, offset = 148. Font raw data.
    FT_Transfer_Start((RAM_G) | MEMORY_WRITE);                  // Start transfer of data to FTDI chip.                                                //    Will be stored at offset 0x00 in RAM_G
        FT_Send_ByteArray(font_setfont, 8413); 
    FT_Transfer_End();
    
    // The display list.
    FT_ListStart(DLIST);
        DLClearColorRGB(0x00, 0x00, 0x00);
        DLClear(1, 1, 1);
        
        // Show top message.
        DLColorRGB(0xFF, 0x00, 0x00);
        CMDText( 10, 10, 30, 0, "PSoC Eve Library DEMO/TEST");
        CMDText( 10, 50, 30, 0, "(setfont_1)");
        
        /* ***** Block 2. ***** */
        // Configure font. Assign handle 6 to the new font.
        DLBitmapHandle(6);
        DLBitmapSource(66);                                    // Source is offset 148 in RAM_G.
        DLBitmapLayout(BITMAP_LAYOUT_L1, 3, 29);
        DLBitmapSize(BITMAP_SIZE_FILTER_NEAREST, BITMAP_SIZE_WRAP_BORDER, BITMAP_SIZE_WRAP_BORDER, 20, 29);
            
        /* ***** Block 3. ***** */
        // At this point. Bitmap data related to the font is at RAM-G, offset = 148.
        // First, will show some characters using Bitmap Primitives.
        DLColorRGB(0xFF, 0xFF, 0xFF);
        DLBegin(PRIMITIVE_BITMAP);
            DLVertex2II(50, 100, 6, 1);     // cell33. char 'A' = ascii 65 (0x41)
            DLVertex2II(100, 100, 6, 18);     // cell 18. char '2' = ascii 50 (0x32)
            
        /* ***** Block 4. ***** */
        // Register the font and use some commands that use it.
        CMDSetfont(6, RAM_G);
        CMDText(50, 150, 6, 0, "#bCdEdFf");  // ascii = 65,98,67,100,69,100,70,102
        CMDNumber(50, 175, 6, 0, 1234);       // ascii = 49,50,51,52
        
        /* ***** Block 5. ***** */
        // Now, repeat previous Vertex.
        DLBegin(PRIMITIVE_BITMAP);
            DLVertex2II(50, 200, 6, 3);     // char 'A' = ascii 65 (0x41)
            DLVertex2II(100, 200, 6, 18);     // char '2' = ascii 50 (0x32)   
            // As you can see, now, no proper chars are shown with these vertex commands.
            // It looks like CMDSetfont2 command makes some changes internally that
            // probably affects pointers related to DLBitmaSource.
    
    FT_ListEnd(END_DL_SWAP);
}

/* [] END OF FILE */