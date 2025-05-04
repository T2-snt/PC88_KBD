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

#include "system.h"
#include "usb.h"

// Global Register
char KeyInit=0;               // キー初期化有無
volatile char cRcvFlag=0;   // キー受信フラグ
char cKeyBits;              // キー受信中ビット
volatile union KeyMap KeyData;    // キーデータ
union ByteBit KeyState[16]; // 現在のキー押下状態



/** CONFIGURATION Bits **********************************************/

/** PIC18F14K50  CONFIGURATION Bits *********************************/
// CONFIG1L
#pragma config CPUDIV = NOCLKDIV
#pragma config USBDIV = OFF
// CONFIG1H
#pragma config FOSC   = HS
#pragma config PLLEN  = ON
#pragma config FCMEN  = OFF
#pragma config IESO   = OFF
// CONFIG2L
#pragma config PWRTEN = OFF
#pragma config BOREN  = OFF
#pragma config BORV   = 30
// CONFIG2H
#pragma config WDTEN  = OFF
#pragma config WDTPS  = 32768
// CONFIG3H
#pragma config MCLRE  = OFF
#pragma config HFOFST = OFF
// CONFIG4L
#pragma config STVREN = ON
#pragma config LVP    = OFF
#pragma config XINST  = OFF
#pragma config BBSIZ  = OFF
// CONFIG5L
#pragma config CP0    = OFF
#pragma config CP1    = OFF
// CONFIG5H
#pragma config CPB    = OFF
#pragma config CPD    = OFF
// CONFIG6L
#pragma config WRT0   = OFF
#pragma config WRT1   = OFF
// CONFIG6H
#pragma config WRTB   = OFF
#pragma config WRTC   = OFF
#pragma config WRTD   = OFF
// CONFIG7L
#pragma config EBTR0  = OFF
#pragma config EBTR1  = OFF
// CONFIG7H
#pragma config EBTRB  = OFF

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
void SYSTEM_Initialize( SYSTEM_STATE state )
{
    // 呼び出されるのはmain処理の最初だけ。

    // PORT Cは全ビット出力。Num/Caps/Scrol Lockを表示
    PORTC = 0;  // 全消灯
    LATC  = 0;
    TRISC = 0;  // 全ビット出力
    
    // PORT B
    PORTB = 0;
    LATB  = 0;
    TRISB = 0b10000000;       //1で入力　0で出力 RB7:入力
    WPUBbits.WPUB7 = 1;       //RB7をプルアップ
    INTCON2bits.RABPU = 0;    //プルアップ有効
    // ------- RB7 Pin 状態変化 初期化 -----------
    IOCBbits.IOCB7 = 1;       // RB7入力変更時の割込みを許可
    PORTB = PORTB;            // 空読み
    INTCON2bits.RABIP = 1;    // Portからの割込みを高優先に設定
    INTCONbits.RABIF = 0;     // Portからの割込みフラグをクリア
    INTCONbits.RABIE = 1;     // Port入力変更時の割込みを許可

    RCONbits.IPEN=1;          // ２段階の割込みに設定
    INTCONbits.GIEH=1;        // 高優先割込みを許可
    INTCONbits.GIEL=1;        // 低優先割込みを許可

    // ----------- Timer2 初期化 -----------
    T2CON = 0b00000101;         // 48uSecごと Post1/1  PS_1/4
    PR2   = 144;
    IPR1bits.TMR2IP = 1;        // Timer2からの割込みを高優先に設定
    PIR1bits.TMR2IF = 0; 
    PIE1bits.TMR2IE = 0;        // 最初は割込なし

    memset(KeyState,0xFF,sizeof(KeyState)); // キー押下状態クリア
}

#if(__XC8_VERSION < 2000)
    #define INTERRUPT interrupt
#else
    #define INTERRUPT __interrupt()
#endif

void INTERRUPT SYS_InterruptHigh(void)
{
    // PORTB RB7変化 (キー入力あり)
    if ((INTCONbits.RABIE == 1) && (INTCONbits.RABIF == 1) &&
        (PORTBbits.RB7 == 0))
    {
        cKeyBits = 0;           // キー受信中ビット クリア
        KeyData.RD = 0;         // キーデータ クリア
        cRcvFlag = 0;           // キー受信フラグ OFF
        
        PORTB = PORTB;  // 空読み
        INTCONbits.RABIF = 0;   // Portからの割込みフラグをクリア
        INTCONbits.RABIE = 0;   // Port入力変更時の割込みを禁止
        
        TMR2 = 74;              // 半ビット分進める
        PIR1bits.TMR2IF = 0;    // Timer2クリア
        PIE1bits.TMR2IE = 1;    // Timer2割込を許可
    }
    // TIMER2割込(キーデータ受信)
    else if (( PIE1bits.TMR2IE == 1)&&(PIR1bits.TMR2IF == 1))
    {
        PIR1bits.TMR2IF = 0;    // 次の割込のためクリア
        if ((cKeyBits == 0)||(cKeyBits == 13)) // スタート/パリティビット
        {
            PORTB = PORTB;      // 空読み
        }
        else if (cKeyBits == 14) // ストップビット
        { 
            cRcvFlag = 1;       // キー受信フラグ ON
            PORTB = PORTB;  // 空読み
            INTCONbits.RABIF = 0;   // Portからの割込みフラグをクリア
            INTCONbits.RABIE = 1;   // Port入力変更時の割込み有効
            
            PIE1bits.TMR2IE = 0;    // Timer2割込を停止
        }
        else if(cKeyBits <=4)   // データ受信処理(Row)
        {   // Row(I/Oポート番号相当))
            KeyData.Row >>= 1;      // 1ビットシフト
            KeyData.RMSB = PORTBbits.RB7;
        }
        else                    // データ受信処理(Clm)
        {   // Clm(キー押下状態 )
            KeyData.Clm >>= 1;      // 1ビットシフト
            KeyData.CMSB = PORTBbits.RB7;
        }
        cKeyBits++;
     }

    #if defined(USB_INTERRUPT)
        USBDeviceTasks();
    #endif
}
