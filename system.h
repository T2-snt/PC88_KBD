/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
*******************************************************************************/

#ifndef SYSTEM_H
#define SYSTEM_H

#include <xc.h>
#include <stdbool.h>

//#include "buttons.h"
//#include "leds.h"

//#include "io_mapping.h"
//#include "fixed_address_memory.h"

#include "usb_config.h"

#define MAIN_RETURN void

//#include "io_mapping.h"の定義内容
/* USB Stack I/O options. */
#define self_power                                      1

//#include "fixed_address_memory.h"の定義内容
#if(__XC8_VERSION < 2000)
    #define KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG  @0x240
    #define KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG  @0x248
#else
    #define KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG      __at(0x240)
    #define KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG     __at(0x248)
#endif

//キー入力に関するグローバル変数
extern char KeyInit;       // キー初期化有無
extern volatile char cRcvFlag;  // キー受信フラグ
extern char cKeyBits;             // キー受信中ビット
union KeyMap{           // キーデータ
    struct{
        unsigned char Clm;
        unsigned char Row;};
    struct{
        unsigned ClmL:4;
        unsigned ClmH:4;
        unsigned RowL:4;
        unsigned RowH:4;};
    struct{
        unsigned OT1:7;
        unsigned CMSB:1;
        unsigned OT2:3;
        unsigned RMSB:1;
        unsigned OT3:4;};
    struct{
        unsigned D0:1;
        unsigned D1:1;
        unsigned D2:1;
        unsigned D3:1;
        unsigned D4:1;
        unsigned D5:1;
        unsigned D6:1;
        unsigned D7:1;
        unsigned OT:8;};
    unsigned short RD;
};
extern volatile union KeyMap KeyData;   //キー受信データ

union ByteBit
{
    struct{
        unsigned D0:1;
        unsigned D1:1;
        unsigned D2:1;
        unsigned D3:1;
        unsigned D4:1;
        unsigned D5:1;
        unsigned D6:1;
        unsigned D7:1;
    };
    unsigned char Dx;
};
extern union ByteBit KeyState[16];  // 現在のキー押下状態

/*** System States **************************************************/
typedef enum
{
    SYSTEM_STATE_USB_START,
    SYSTEM_STATE_USB_SUSPEND,
    SYSTEM_STATE_USB_RESUME
} SYSTEM_STATE;

/*********************************************************************
* Function: void SYSTEM_Initialize( SYSTEM_STATE state )
*
* Overview: Initializes the system.
*
* PreCondition: None
*
* Input:  SYSTEM_STATE - the state to initialize the system into
*
* Output: None
*
********************************************************************/
void SYSTEM_Initialize( SYSTEM_STATE state );

/*********************************************************************
* Function: void SYSTEM_Tasks(void)
*
* Overview: Runs system level tasks that keep the system running
*
* PreCondition: System has been initalized with SYSTEM_Initialize()
*
* Input: None
*
* Output: None
*
********************************************************************/
//void SYSTEM_Tasks(void);
#define SYSTEM_Tasks()

#endif //SYSTEM_H
