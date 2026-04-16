/******************** (C) COPYRIGHT 2023 SONiX *******************************
* COMPANY:		SONiX
* DATE:				2023/11
* AUTHOR:			SA1
* IC:					SN32F400
*____________________________________________________________________________
*	REVISION	Date				User		Description
*	1.0				2023/11/06	SA1			1. First version released
*																2. Compatible to CMSIS DFP Architecture in Keil MDK v5.X (http://www.keil.com/dd2/pack/)
*																3. Run HexConvert to generate bin file and show checksum after building.
*
*____________________________________________________________________________
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS TIME TO MARKET.
* SONiX SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR CONSEQUENTIAL 
* DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT OF SUCH SOFTWARE
* AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN 
* IN CONNECTION WITH THEIR PRODUCTS.
*****************************************************************************/

/*_____ I N C L U D E S ____________________________________________________*/
#include <SN32F400.h>
#include <SN32F400_Def.h>
#include "..\Driver\GPIO.h"
#include "..\Driver\CT16B0.h"
#include "..\Driver\WDT.h"
#include "..\Driver\Utility.h"
#include "..\Module\Segment.h"
#include "..\Module\KeyScan.h"
#include "..\Driver\I2C.h"
#include "..\Module\EEPROM.h"
#include "..\Module\Buzzer.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/
void PFPA_Init(void);
void NotPinOut_GPIO_init(void);


/*_____ D E F I N I T I O N S ______________________________________________*/
#ifndef	SN32F407					//Do NOT Remove or Modify!!!
	#error Please install SONiX.SN32F4_DFP.0.0.18.pack or version >= 0.0.18
#endif
#define	PKG						SN32F407				//User SHALL modify the package on demand (SN32F407)
#define	SEG_H		0x80
#define MODE_RUN        0   

#define MODE_SET_HH     1   
#define MODE_SET_MM     2   

#define MODE_ALARM_HH   3   
#define MODE_ALARM_MM   4   

#define	EEPROM_WRITE_ADDR			0xa0
#define	EEPROM_READ_ADDR			0xa1

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/
uint8_t hour = 0;
uint8_t hour_alarm = 0;
uint8_t minute = 0;
uint8_t minute_alarm = 0;
uint8_t second = 0;
uint16_t display = 0;
uint16_t display_alarm = 0;
uint16_t blink_500ms = 0;
uint8_t blink = 0;
uint16_t read_key;
uint8_t mode = 0;

uint8_t alarm_flag = 0; //0:idle //1:ring //2:off
uint16_t alarm_cnt = 0; 
uint8_t alarm_time = 0;

//---- EEPROM data and address
uint8_t eeprom_hour_addr = 0;
uint8_t eeprom_hour_data = 0;
uint8_t eeprom_minute_addr = 0;
uint8_t eeprom_minute_data = 0;



/*****************************************************************************
* Function		: main
* Description	: LED toggles based on soft delay
* Input			: None
* Output		: None
* Return		: None
* Note			: Connect LEDs to P2.0 and P2.1
*****************************************************************************/
int	main(void)
{
	//User can configure System Clock with Configuration Wizard in system_SN32F400.c
	SystemInit();
	SystemCoreClockUpdate();				//Must call for SN32F400, Please do NOT remove!!!

	//Note: User can refer to ClockSwitch sample code to switch various HCLK if needed.

	PFPA_Init();										//User shall set PFPA if used, do NOT remove!!!

	//1. User SHALL define PKG on demand.
	//2. User SHALL set the status of the GPIO which are NOT pin-out to input pull-up.
	NotPinOut_GPIO_init();

	//--------------------------------------------------------------------------
	//User Code starts HERE!!!
	//enable reset pin function
	SN_SYS0->EXRSTCTRL_b.RESETDIS = 0;
	
	GPIO_Init();								//initial gpio
	
	WDT_Init();									//Set WDT reset overflow time ~ 250ms
	
	CT16B0_Init();
	while (1)
    {
        __WDT_FEED_VALUE;

        //================ 1ms TASK =================
        if (timer_1ms_flag)
        {
            timer_1ms_flag = 0;

            blink_500ms++;
            if (blink_500ms >= 500)
            {
                blink_500ms = 0;
                blink ^= 1;
            }
            Digital_Scan();
						
						if(alarm_flag == 1)
						{
							
							if(alarm_cnt == 0)
							{
								set_buzzer_pitch(4);
							}
							else if(alarm_cnt == 500)
							{
								set_buzzer_pitch(50);
							}
							else if(alarm_cnt > 1000)
							{
								alarm_cnt = 0;
							}
							alarm_cnt++;
						}	
        }

        //================ 1s TIME =================
        if (timer_1s_flag)
        {
						if(alarm_flag == 1 )
						{
							if(alarm_time > 5)
							{
								alarm_flag = 2;
								alarm_time = 0;
							}
							else
							{
								alarm_time++;
							}
						}		
            timer_1s_flag = 0;
						uint8_t alarm_minute = minute_alarm;
						uint8_t alarm_hour = hour_alarm;
					
//						eeprom_read(EEPROM_READ_ADDR,eeprom_hour_addr,&alarm_hour,1);
//						eeprom_read(EEPROM_READ_ADDR,eeprom_minute_addr,&alarm_minute,1);
		
			
            if (mode == MODE_RUN)
            {
                second++;
                if (second >= 60)
                {
                    second = 0;
                    minute++;
                    if (minute >= 60)
                    {
                        minute = 0;
                        hour++;
                        if (hour > 23) hour = 0;
                    }
                }
            }
							
						if(alarm_flag == 2 && minute != alarm_minute)
							alarm_flag = 0;
						if(hour == alarm_hour && minute == alarm_minute && alarm_flag == 0 )
						{
								alarm_flag = 1;
						}	
							
            display = hour * 100 + minute;
						
					if(mode == MODE_ALARM_HH || mode == MODE_ALARM_MM)
						Digital_DisplayDEC(display_alarm);
					else
						Digital_DisplayDEC(display);
        }
        //================ KEY =================
        read_key = KeyScan();

        //================ KEY1 -> SET TIME MODE =================
        if (read_key == (KEY_PUSH_FLAG | KEY_1))
        {
            if (mode == MODE_RUN) mode = MODE_SET_HH;
            else if (mode == MODE_SET_HH) mode = MODE_SET_MM;
            else if (mode == MODE_SET_MM) mode = MODE_RUN;
        }

        //================ KEY16 -> ALARM MODE =================
				else if (read_key == (KEY_PUSH_FLAG | KEY_16))
				{
					if (mode == MODE_RUN) 
					{
						mode = MODE_ALARM_HH;
						hour_alarm = 0;
						minute_alarm = 0;
						display_alarm = hour_alarm * 100 + minute_alarm;
					}
					else if (mode == MODE_ALARM_HH) mode = MODE_ALARM_MM;
					else if (mode == MODE_ALARM_MM)
						{
							alarm_flag = 0; // reset flag to ring again
							
							eeprom_hour_addr = 0x80;
							eeprom_minute_addr = 0x81;
							
							eeprom_hour_data = hour_alarm;
							eeprom_minute_data = minute_alarm;
							
//							eeprom_write(EEPROM_WRITE_ADDR,eeprom_hour_addr,&eeprom_hour_data,1);
//							eeprom_write(EEPROM_WRITE_ADDR,eeprom_minute_addr,&eeprom_minute_data,1);

							mode = MODE_RUN;
							display = hour * 100 + minute;  // ? V� th�m d�ng n�y khi tho�t

						}
        }

        //================ KEY4 -> INCREASE VALUE =================
        else if (read_key == (KEY_PUSH_FLAG | KEY_4))
        {
            if (mode == MODE_SET_HH)
            {
                hour++;
                if (hour == 24) hour = 0;
            }
            else if (mode == MODE_SET_MM)
            {
                minute++;
                if (minute == 60) minute = 0;
            }
						else if (mode == MODE_ALARM_HH)
						{
								hour_alarm++;
								if (hour_alarm == 24) hour_alarm = 0;
								display_alarm = hour_alarm * 100 + minute_alarm;
						}
						else if (mode == MODE_ALARM_MM)
						{
								minute_alarm++;
								if (minute_alarm == 60) minute_alarm = 0;
								display_alarm = hour_alarm * 100 + minute_alarm;
						}
					  display = hour * 100 + minute;

        }
				
        //================ KEY8 -> DECREASE VALUE =================
        else if (read_key == (KEY_PUSH_FLAG | KEY_8))
        {
            if (mode == MODE_SET_HH)
            {
								hour--;
                if (hour == 255) hour = 23;
            }
            else if (mode == MODE_SET_MM)
            {
								minute--;
                if (minute == 255) minute = 59;
            }
						else if (mode == MODE_ALARM_HH)
						{
								hour_alarm--;
								if (hour_alarm == 255) hour_alarm = 23;
								display_alarm = hour_alarm * 100 + minute_alarm;
						}
						else if (mode == MODE_ALARM_MM)
						{
								minute_alarm--;
								if (minute_alarm == 255) minute_alarm = 59;
								display_alarm = hour_alarm * 100 + minute_alarm;
						}
						display = hour * 100 + minute;
        }
        //================ BLINK DISPLAY =================
        if (mode != MODE_RUN && blink)
        {
            if (mode == MODE_SET_HH || mode == MODE_ALARM_HH)
            {
                segment_buff[0] = 0;
                segment_buff[1] = 0;
            }
            else if (mode == MODE_SET_MM || mode == MODE_ALARM_MM)
            {
                segment_buff[2] = 0;
                segment_buff[3] = 0;
            }
        }

        segment_buff[1] |= SEG_H;
				
    }
}
/*****************************************************************************
* Function		: NotPinOut_GPIO_init
* Description	: Set the status of the GPIO which are NOT pin-out to input pull-up. 
* Input				: None
* Output			: None
* Return			: None
* Note				: 1. User SHALL define PKG on demand.
*****************************************************************************/
void NotPinOut_GPIO_init(void)
{
#if (PKG == SN32F405)
	//set P0.4, P0.6, P0.7 to input pull-up
	SN_GPIO0->CFG = 0x00A008AA;
	//set P1.4 ~ P1.12 to input pull-up
	SN_GPIO1->CFG = 0x000000AA;
	//set P3.8 ~ P3.11 to input pull-up
	SN_GPIO3->CFG = 0x0002AAAA;
#elif (PKG == SN32F403)
	//set P0.4 ~ P0.7 to input pull-up
	SN_GPIO0->CFG = 0x00A000AA;
	//set P1.4 ~ P1.12 to input pull-up
	SN_GPIO1->CFG = 0x000000AA;
	//set P2.5 ~ P2.6, P2.10 to input pull-up
	SN_GPIO2->CFG = 0x000A82AA;
	//set P3.0, P3.8 ~ P3.13 to input pull-up
	SN_GPIO3->CFG = 0x0000AAA8;
#endif
}

/*****************************************************************************
* Function		: HardFault_Handler
* Description	: ISR of Hard fault interrupt
* Input			: None
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
void HardFault_Handler(void)
{
	NVIC_SystemReset();
}
