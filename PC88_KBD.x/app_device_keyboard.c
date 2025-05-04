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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>
#include <string.h>

#include "system.h"
#include "usb.h"
#include "usb_device_hid.h"

//#include "app_led_usb_status.h"

// キー処理に関する変数
// キーのキャンセル処理フラグ
union KC
{
    struct{
        unsigned Fn6:1;
        unsigned Fn7:1;
        unsigned Fn8:1;
        unsigned Fn9:1;
        unsigned FnA:1;
        unsigned SR:1;
        unsigned SL:1;
        unsigned NUL:1;
    };
    unsigned char Dx;
};
static union KC KeyCancel;

union LK
{
    struct{
        unsigned NL:1;  // NumLock
        unsigned CL:1;  // CapsLock
        unsigned SL:1;  // ScrollLock
        unsigned:5;
    };
    unsigned char Dx;
};
static union LK KeyLockFlag;

const unsigned char KeyChgTable[16][8]={
{0x62,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F},
{0x60,0x61,0x55,0x57,0x67,0x85,0x63,0x00},
{0x2F,0x04,0x05,0x06,0x07,0x08,0x09,0x0A},
{0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12},
{0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A},
{0x1B,0x1C,0x1D,0x30,0x89,0x32,0x2E,0x2D},
{0x27,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24},
{0x25,0x26,0x34,0x33,0x36,0x37,0x38,0x87},
{0x4A,0x52,0x4F,0x00,0x00,0x00,0x00,0x00},
{0x48,0x3A,0x3B,0x3C,0x3D,0x3E,0x2C,0x29},
{0x2B,0x51,0x50,0x4D,0x46,0x56,0x54,0x00},
{0x4B,0x4E,0x00,0x00,0x00,0x00,0x00,0x00},
{0x3F,0x40,0x41,0x42,0x43,0x2A,0x49,0x4C},
{0x8A,0x8B,0x00,0x35,0x00,0x00,0x00,0x00},
{0x28,0x58,0x00,0x00,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};



#if defined(__XC8)
    #define PACKED
#else
    #define PACKED __attribute__((packed))
#endif

// *****************************************************************************
// *****************************************************************************
// Section: File Scope or Global Constants
// *****************************************************************************
// *****************************************************************************

//Class specific descriptor - HID Keyboard
const struct{uint8_t report[HID_RPT01_SIZE];}hid_rpt01={
{   0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x01,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0xFF,                    //   LOGICAL_MAXIMUM (255)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0xFF,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0}                          // End Collection
};


// *****************************************************************************
// *****************************************************************************
// Section: File Scope Data Types
// *****************************************************************************
// *****************************************************************************

/* This typedef defines the only INPUT report found in the HID report
 * descriptor and gives an easy way to create the OUTPUT report. */
typedef struct PACKED
{
    /* The union below represents the first byte of the INPUT report.  It is
     * formed by the following HID report items:
     *
     *  0x19, 0xe0, //   USAGE_MINIMUM (Keyboard LeftControl)
     *  0x29, 0xe7, //   USAGE_MAXIMUM (Keyboard Right GUI)
     *  0x15, 0x00, //   LOGICAL_MINIMUM (0)
     *  0x25, 0x01, //   LOGICAL_MAXIMUM (1)
     *  0x75, 0x01, //   REPORT_SIZE (1)
     *  0x95, 0x08, //   REPORT_COUNT (8)
     *  0x81, 0x02, //   INPUT (Data,Var,Abs)
     *
     * The report size is 1 specifying 1 bit per entry.
     * The report count is 8 specifying there are 8 entries.
     * These entries represent the Usage items between Left Control (the usage
     * minimum) and Right GUI (the usage maximum).
     */
    union PACKED
    {
        uint8_t value;
        struct PACKED
        {
            unsigned leftControl    :1;
            unsigned leftShift      :1;
            unsigned leftAlt        :1;
            unsigned leftGUI        :1;
            unsigned rightControl   :1;
            unsigned rightShift     :1;
            unsigned rightAlt       :1;
            unsigned rightGUI       :1;
        } bits;
    } modifiers;

    /* There is one byte of constant data/padding that is specified in the
     * input report:
     *
     *  0x95, 0x01,                    //   REPORT_COUNT (1)
     *  0x75, 0x08,                    //   REPORT_SIZE (8)
     *  0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
     */
    unsigned :8;

    /* The last INPUT item in the INPUT report is an array type.  This array
     * contains an entry for each of the keys that are currently pressed until
     * the array limit, in this case 6 concurent key presses.
     *
     *  0x95, 0x06,                    //   REPORT_COUNT (6)
     *  0x75, 0x08,                    //   REPORT_SIZE (8)
     *  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
     *  0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
     *  0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
     *  0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
     *  0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
     *
     * Report count is 6 indicating that the array has 6 total entries.
     * Report size is 8 indicating each entry in the array is one byte.
     * The usage minimum indicates the lowest key value (Reserved/no event)
     * The usage maximum indicates the highest key value (Application button)
     * The logical minimum indicates the remapped value for the usage minimum:
     *   No Event has a logical value of 0.
     * The logical maximum indicates the remapped value for the usage maximum:
     *   Application button has a logical value of 101.
     *
     * In this case the logical min/max match the usage min/max so the logical
     * remapping doesn't actually change the values.
     *
     * To send a report with the 'a' key pressed (usage value of 0x04, logical
     * value in this example of 0x04 as well), then the array input would be the
     * following:
     *
     * LSB [0x04][0x00][0x00][0x00][0x00][0x00] MSB
     *
     * If the 'b' button was then pressed with the 'a' button still held down,
     * the report would then look like this:
     *
     * LSB [0x04][0x05][0x00][0x00][0x00][0x00] MSB
     *
     * If the 'a' button was then released with the 'b' button still held down,
     * the resulting array would be the following:
     *
     * LSB [0x05][0x00][0x00][0x00][0x00][0x00] MSB
     *
     * The 'a' key was removed from the array and all other items in the array
     * were shifted down. */
    uint8_t keys[6];
} KEYBOARD_INPUT_REPORT;


/* This typedef defines the only OUTPUT report found in the HID report
 * descriptor and gives an easy way to parse the OUTPUT report. */
typedef union PACKED
{
    /* The OUTPUT report is comprised of only one byte of data. */
    uint8_t value;
    struct
    {
        /* There are two report items that form the one byte of OUTPUT report
         * data.  The first report item defines 5 LED indicators:
         *
         *  0x95, 0x05,                    //   REPORT_COUNT (5)
         *  0x75, 0x01,                    //   REPORT_SIZE (1)
         *  0x05, 0x08,                    //   USAGE_PAGE (LEDs)
         *  0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
         *  0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
         *  0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
         *
         * The report count indicates there are 5 entries.
         * The report size is 1 indicating each entry is just one bit.
         * These items are located on the LED usage page
         * These items are all of the usages between Num Lock (the usage
         * minimum) and Kana (the usage maximum).
         */
        unsigned numLock        :1;
        unsigned capsLock       :1;
        unsigned scrollLock     :1;
        unsigned compose        :1;
        unsigned kana           :1;

        /* The second OUTPUT report item defines 3 bits of constant data
         * (padding) used to make a complete byte:
         *
         *  0x95, 0x01,                    //   REPORT_COUNT (1)
         *  0x75, 0x03,                    //   REPORT_SIZE (3)
         *  0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
         *
         * Report count of 1 indicates that there is one entry
         * Report size of 3 indicates the entry is 3 bits long. */
        unsigned                :3;
    } leds;
} KEYBOARD_OUTPUT_REPORT;


/* This creates a storage type for all of the information required to track the
 * current state of the keyboard. */
typedef struct
{
    USB_HANDLE lastINTransmission;
    USB_HANDLE lastOUTTransmission;
//  unsigned char key;
    bool waitingForRelease;
} KEYBOARD;

// *****************************************************************************
// *****************************************************************************
// Section: File Scope or Global Variables
// *****************************************************************************
// *****************************************************************************
static KEYBOARD keyboard;

#if !defined(KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG)
    #define KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG
#endif
static KEYBOARD_INPUT_REPORT inputReport KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG;

#if !defined(KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG)
    #define KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG
#endif
static volatile KEYBOARD_OUTPUT_REPORT outputReport KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG;


// *****************************************************************************
// *****************************************************************************
// Section: Private Prototypes
// *****************************************************************************
// *****************************************************************************
static void APP_KeyboardProcessOutputReport(void);


//Exteranl variables declared in other .c files
extern volatile signed int SOFCounter;


//Application variables that need wide scope
KEYBOARD_INPUT_REPORT oldInputReport;
signed int keyboardIdleRate;
signed int LocalSOFCount;
static signed int OldSOFCount;
static KEYBOARD_OUTPUT_REPORT LockLED;



// *****************************************************************************
// *****************************************************************************
// Section: Macros or Functions
// *****************************************************************************
// *****************************************************************************
void APP_KeyboardInit(void)
{
    //initialize the variable holding the handle for the last
    // transmission
    keyboard.lastINTransmission = 0;
    
//    keyboard.key = 4;
    keyboard.waitingForRelease = false;

    //Set the default idle rate to 500ms (until the host sends a SET_IDLE request to change it to a new value)
    keyboardIdleRate = 500;

    //Copy the (possibly) interrupt context SOFCounter value into a local variable.
    //Using a while() loop to do this since the SOFCounter isn't necessarily atomically
    //updated and therefore we need to read it a minimum of twice to ensure we captured the correct value.
    while(OldSOFCount != SOFCounter)
    {
        OldSOFCount = SOFCounter;
    }

    //enable the HID endpoint
    USBEnableEndpoint(HID_EP, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //Arm OUT endpoint so we can receive caps lock, num lock, etc. info from host
    keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport, sizeof(outputReport) );
    
    // 一応クリアしておく
    memset(&inputReport, 0, sizeof(inputReport));
    oldInputReport = inputReport;
    KeyCancel.Dx = 0;
    KeyLockFlag.Dx = 0;
    LockLED.value = 0;
}

void APP_KeyboardTasks(void)
{
    signed int TimeDeltaMilliseconds;
    bool needToSendNewReportPacket;

    unsigned char IR_Count;
    unsigned char i,j;
    union ByteBit bTemp;
    uint8_t KDTemp[14]; // 6キー+8予備=14

    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop. */
    if( USBGetDeviceState() < CONFIGURED_STATE )
    {
        return;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop. */
    if( USBIsDeviceSuspended()== true )
    {
        return;
    }
    
    //Copy the (possibly) interrupt context SOFCounter value into a local variable.
    //Using a while() loop to do this since the SOFCounter isn't necessarily atomically
    //updated and we need to read it a minimum of twice to ensure we captured the correct value.
    while(LocalSOFCount != SOFCounter)
    {
        LocalSOFCount = SOFCounter;
    }

    //Compute the elapsed time since the last input report was sent (we need
    //this info for properly obeying the HID idle rate set by the host).
    TimeDeltaMilliseconds = LocalSOFCount - OldSOFCount;
    //Check for negative value due to count wraparound back to zero.
    if(TimeDeltaMilliseconds < 0)
    {
        TimeDeltaMilliseconds = (32767 - OldSOFCount) + LocalSOFCount;
    }
    //Check if the TimeDelay is quite large.  If the idle rate is == 0 (which represents "infinity"),
    //then the TimeDeltaMilliseconds could also become infinity (which would cause overflow)
    //if there is no recent button presses or other changes occurring on the keyboard.
    //Therefore, saturate the TimeDeltaMilliseconds if it gets too large, by virtue
    //of updating the OldSOFCount, even if we haven't actually sent a packet recently.
    if(TimeDeltaMilliseconds > 5000)
    {
        OldSOFCount = LocalSOFCount - 5000;
    }


    /* Check if the IN endpoint is busy, and if it isn't check if we want to send
     * keystroke data to the host. */
    if(HIDTxHandleBusy(keyboard.lastINTransmission) == false)
    {
        // ここから先は、PC-8801キーボードからの入力により設定する。
        if ((cRcvFlag == 1) ||  // キーデータ受信あり
            (KeyLockFlag.Dx != 0)) // キーロック処理によるイベント発生用
        {
            if (KeyInit == 0)   // 初期化処理(キーイベントは発生させない)
            {   // 最初にRow=0～14のデータが送られてくる。
                KeyState[KeyData.Row].Dx = KeyData.Clm;
                if (KeyData.Row == 14){ KeyInit = 1;}   // キー初期化 ON
            }
            else
            {   /* Clear the INPUT report buffer.  Set to all zeros. */
                memset(&inputReport, 0, sizeof(inputReport));
                memset(KDTemp,0,6); // 6キー分の一時置き場クリア
                IR_Count = 0;       // InputReportで返すキーの数
                KeyLockFlag.Dx = 0; // キーロックフラグはクリアしておく

                // Row=12,13のキーで、受け捨て準備を行う
                if (KeyData.Row == 13)
                {
                    if (KeyData.D0 == 0)
                    {   // 変換キーが押されたら、次に来るスペースは受け捨て
                        KeyCancel.SR = 1;
                    }
                    if (KeyData.D1 == 0)
                   {   // 決定キーが押されたら、次に来るスペースは受け捨て
                        KeyCancel.SL = 1;
                    }
                }
                if (KeyData.Row == 12)
                {
                    if (KeyData.D0 == 0)
                    {   // F6キーが押されたら、次に来るF1は受け捨て
                        KeyCancel.Fn6 = 1;
                    }
                    if (KeyData.D1 == 0)
                   {   // F7キーが押されたら、次に来るF2は受け捨て
                        KeyCancel.Fn7 = 1;
                    }
                    if (KeyData.D2 == 0)
                   {   // F8キーが押されたら、次に来るF3は受け捨て
                        KeyCancel.Fn8 = 1;
                    }
                    if (KeyData.D3 == 0)
                   {   // F9キーが押されたら、次に来るF4は受け捨て
                        KeyCancel.Fn9 = 1;
                    }
                    if (KeyData.D4 == 0)
                   {   // F10キーが押されたら、次に来るF5は受け捨て
                        KeyCancel.FnA = 1;
                    }
                }

                // 受け捨て処理
                if (KeyCancel.Dx != 0)
                {   // 何か受け捨て処理がある
                    if ((KeyCancel.SL==1)&&(KeyData.Row==9)&&(KeyData.D6==0))
                    {   // 変換キー押下後のスペース
                        KeyData.D6 = 1; // SPACEをなかったことにする
                        KeyCancel.SL = 0;   // キャンセルフラグ クリア
                    }
                    if ((KeyCancel.SR==1)&&(KeyData.Row==9)&&(KeyData.D6==0))
                    {   // 決定キー押下後のスペース
                        KeyData.D6 = 1; // SPACEをなかったことにする
                        KeyCancel.SR = 0;   // キャンセルフラグ クリア
                    }
                    if ((KeyCancel.Fn6==1)&&(KeyData.Row==9)&&(KeyData.D1==0))
                    {   // F6キー押下後のF1
                        KeyData.D1 = 1; // F1をなかったことにする
                        KeyCancel.Fn6 = 0;   // キャンセルフラグ クリア
                    }
                    if ((KeyCancel.Fn7==1)&&(KeyData.Row==9)&&(KeyData.D2==0))
                    {   // F7キー押下後のF2
                        KeyData.D2 = 1; // F2をなかったことにする
                        KeyCancel.Fn7 = 0;   // キャンセルフラグ クリア
                    }
                    if ((KeyCancel.Fn8==1)&&(KeyData.Row==9)&&(KeyData.D3==0))
                    {   // F8キー押下後のF3
                        KeyData.D3 = 1; // F3をなかったことにする
                        KeyCancel.Fn8 = 0;   // キャンセルフラグ クリア
                    }
                    if ((KeyCancel.Fn9==1)&&(KeyData.Row==9)&&(KeyData.D4==0))
                    {   // F9キー押下後のF4
                        KeyData.D4 = 1; // F4をなかったことにする
                        KeyCancel.Fn9 = 0;   // キャンセルフラグ クリア
                    }
                    if ((KeyCancel.FnA==1)&&(KeyData.Row==9)&&(KeyData.D5==0))
                    {   // F10キー押下後のF5
                        KeyData.D5 = 1; // F5をなかったことにする
                        KeyCancel.FnA = 0;   // キャンセルフラグ クリア
                    }
                }

                // 「カナ」キーの状態が変化したときの処理
                if ((KeyData.Row == 8) && (KeyData.D5 != KeyState[8].D5))
                {
                    if (((KeyData.D5 == 0) && (LockLED.leds.scrollLock == 0))||
                        ((KeyData.D5 == 1) && (LockLED.leds.scrollLock == 1)))
                    {   // 「カナ」キーが押されて、ScrollLock中ではない 又は、
                        // 「カナ」キーが離されて、ScrollLock中である
                        KDTemp[IR_Count]=0x47;  // ScrollLockキーを設定
                        IR_Count++;
                        KeyLockFlag.SL = 1; // イベント発生用(次の処理でキー無し送信)
                    }
                }                
                // 「CLR」キーの状態が押されたときの処理
                if ((KeyData.Row == 8) && (KeyData.D0 == 0) && (KeyData.D0 != KeyState[8].D0))
                {
                    if (KeyState[8].D6 == 0)
                    {   // シフトキーが押されていた場合のみNumLockとする。
                        // (シフトキーが押されてなければ、HOMEキー)
                        KDTemp[IR_Count]=0x53;  // NumLockキーを設定
                        IR_Count++;
                        KeyData.D0 = 1;     // キーを処理済
                        KeyLockFlag.NL = 1; // イベント発生用(次の処理でキー無し送信)
                    }
                }                

                // 「CAPS」キーの状態が変化したときの処理
                if ((KeyData.Row == 10) && (KeyData.D7 != KeyState[10].D7))
                {
                    if (((KeyData.D7 == 0) && (LockLED.leds.capsLock == 0))||
                        ((KeyData.D7 == 1) && (LockLED.leds.capsLock == 1)))
                    {   // 「CAPS」キーが押されて、CapsLock中ではない 又は、
                        // 「CAPS」キーが離されて、CapsLock中である
                        KDTemp[IR_Count]=0x39;  // CapsLockキーを設定
                        IR_Count++;
                        KeyLockFlag.CL = 1; // イベント発生用(次の処理でキー無し送信)
                    }
                }                
                
                KeyState[KeyData.Row].Dx = KeyData.Clm;    // キー状態更新

                // modify設定
                inputReport.modifiers.bits.leftShift=~KeyState[14].D2;  // Shift(L)
                inputReport.modifiers.bits.rightShift=~KeyState[14].D3; // Shift(R)
                inputReport.modifiers.bits.leftControl=~KeyState[8].D7; // Ctrl(L)
                inputReport.modifiers.bits.leftAlt=~KeyState[8].D4; // GRPH→Alt(L)
                inputReport.modifiers.bits.rightGUI=~KeyState[13].D2; // PC→Win(R)
                // CAPSキー押下時はShiftも設定する
                if (KeyLockFlag.CL == 1)
                {
                    inputReport.modifiers.bits.leftShift=1;  // Shift(L)
                }

                // 押されたキーに対応するコードを設定
                // (最大6キーまで。6キーを越えた分は無視)
                for (i=0 ; ((i<=14)&&(IR_Count<6)) ; i++)
                {
                    bTemp.Dx = KeyState[i].Dx;
                    for (j=0 ; j<=7 ; j++)
                    {
                        if (bTemp.D0 == 0)
                        {
                            KDTemp[IR_Count]=KeyChgTable[i][j];
                            if (KDTemp[IR_Count] != 0)
                            {
                                IR_Count++;
                            }
                        }
                        bTemp.Dx >>= 1;  // １ビット右シフト
                    }
                }
                if (IR_Count <=6)
                {   // ６キー以内なら、そのまま送信
                    memcpy(inputReport.keys,KDTemp,6);
                }
                else
                {   // ６キーを超えたら、エラーレポート
                    memset(inputReport.keys,0x01,6);
                }
            }
            cRcvFlag = 0;   // 受信フラグクリア
        }

        //Check to see if the new packet contents are somehow different from the most
        //recently sent packet contents.
        needToSendNewReportPacket = false;
        for(i = 0; i < sizeof(inputReport); i++)
        {
            if(*((uint8_t*)&oldInputReport + i) != *((uint8_t*)&inputReport + i))
            {
                needToSendNewReportPacket = true;
                break;
            }
        }

        //Check if the host has set the idle rate to something other than 0 (which is effectively "infinite").
        //If the idle rate is non-infinite, check to see if enough time has elapsed since
        //the last packet was sent, and it is time to send a new repeated packet or not.
        if(keyboardIdleRate != 0)
        {
            //Check if the idle rate time limit is met.  If so, need to send another HID input report packet to the host
            if(TimeDeltaMilliseconds >= keyboardIdleRate)
            {
                needToSendNewReportPacket = true;
            }
        }

        //Now send the new input report packet, if it is appropriate to do so (ex: new data is
        //present or the idle rate limit was met).
        if(needToSendNewReportPacket == true)
        {
            //Save the old input report packet contents.  We do this so we can detect changes in report packet content
            //useful for determining when something has changed and needs to get re-sent to the host when using
            //infinite idle rate setting.
            oldInputReport = inputReport;

            /* Send the 8 byte packet over USB to the host. */
            keyboard.lastINTransmission = HIDTxPacket(HID_EP, (uint8_t*)&inputReport, sizeof(inputReport));
            OldSOFCount = LocalSOFCount;    //Save the current time, so we know when to send the next packet (which depends in part on the idle rate setting)
        }

    }//if(HIDTxHandleBusy(keyboard.lastINTransmission) == false)


    /* Check if any data was sent from the PC to the keyboard device.  Report
     * descriptor allows host to send 1 byte of data.  Bits 0-4 are LED states,
     * bits 5-7 are unused pad bits.  The host can potentially send this OUT
     * report data through the HID OUT endpoint (EP1 OUT), or, alternatively,
     * the host may try to send LED state information by sending a SET_REPORT
     * control transfer on EP0.  See the USBHIDCBSetReportHandler() function. */
    if(HIDRxHandleBusy(keyboard.lastOUTTransmission) == false)
    {
        APP_KeyboardProcessOutputReport();

        keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport,sizeof(outputReport));
    }
    
    return;		
}

static void APP_KeyboardProcessOutputReport(void)
{
    // Num,Caps,Scroll Lock状態を表示

    PORTC = 0b00000111 & outputReport.value;
    LockLED.value = outputReport.value;     // ロック状態を退避
}

static void USBHIDCBSetReportComplete(void)
{
    /* 1 byte of LED state data should now be in the CtrlTrfData buffer.  Copy
     * it to the OUTPUT report buffer for processing */
    outputReport.value = CtrlTrfData[0];

    /* Process the OUTPUT report. */
    APP_KeyboardProcessOutputReport();
}

void USBHIDCBSetReportHandler(void)
{
    /* Prepare to receive the keyboard LED state data through a SET_REPORT
     * control transfer on endpoint 0.  The host should only send 1 byte,
     * since this is all that the report descriptor allows it to send. */
    USBEP0Receive((uint8_t*)&CtrlTrfData, USB_EP0_BUFF_SIZE, USBHIDCBSetReportComplete);
}


//Callback function called by the USB stack, whenever the host sends a new SET_IDLE
//command.
void USBHIDCBSetIdleRateHandler(uint8_t reportID, uint8_t newIdleRate)
{
    //Make sure the report ID matches the keyboard input report id number.
    //If however the firmware doesn't implement/use report ID numbers,
    //then it should be == 0.
    if(reportID == 0)
    {
        keyboardIdleRate = newIdleRate;
    }
}


/*******************************************************************************
 End of File
*/
