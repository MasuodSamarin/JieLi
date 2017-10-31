/*--------------------------------------------------------------------------*/
/**@file    FM_radio.c
   @brief   FM ģʽ��ѭ��
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "fm_radio.h"
#include "FM_API.h"
#include "hot_msg.h"
#include "msg.h"
#include "key.h"
#include "main.h"
#include "iic.h"
#include "UI_API.h"
#include "music_play.h"
#include "tff.h"
#include "dac.h"
#include "clock.h"
#include "sdmmc_api.h"

#include "bike.h"

extern ENUM_WORK_MODE _data work_mode;
_no_init FM_MODE_VAR _data fm_mode_var;
_no_init u8 _data scan_mode;
extern _no_init bool _data key_mute;    //music_play.c

/*----------------------------------------------------------------------------*/
/**@brief    FM ģʽ��ѭ��
   @param    ��
   @return   ��
   @note     void fm_play(void)
*/
/*----------------------------------------------------------------------------*/
void fm_play(void) AT(FM_CODE)
{
    u8 scan_counter,i;

    UI_menu(MENU_FM_MAIN);

    while (1)
    {
        u8 key = app_get_msg();

        switch (key)
        {
        case MSG_MUSIC_NEW_DEVICE_IN:
            work_mode = MUSIC_MODE;
            return ;
        case MSG_CHANGE_WORK_MODE:
            work_mode = MUSIC_MODE;
            return;

//        case MSG_STOP_SCAN:
//            scan_counter = 1;                   //������һ��Ƶ���ֹͣ
//            break;

        case MSG_FM_SCAN_ALL_INIT:
            if (scan_mode == FM_SCAN_STOP)
            {
                set_memory(MEM_CHAN, 0);
                set_memory(MEM_FRE, 0);
                clear_all_fm_point();
                fm_mode_var.bTotalChannel = 0;
                fm_mode_var.bFreChannel = 0;
                fm_mode_var.wFreq = MIN_FRE - 1;					//�Զ���������͵�Ƶ�㿪ʼ
                scan_counter = MAX_CHANNL;
                scan_mode = FM_SCAN_ALL;
            }
            else
            {
                scan_counter = 1;                   //������һ��Ƶ���ֹͣ
            }

        case MSG_FM_SCAN_ALL:
            flush_all_msg();
            if (fm_scan(scan_mode))
            {
                if (scan_mode == FM_SCAN_ALL)
                    delay_n10ms(100);                               //����1S
                else
                {
                    scan_mode = FM_SCAN_STOP;
                    break;
                }
            }

            scan_counter--;
            if (scan_counter == 0)
            {
                if (scan_mode == FM_SCAN_ALL)                 //ȫƵ���������������ŵ�һ��̨
                {
                    put_msg_lifo(MSG_FM_NEXT_STATION);
                    scan_mode = FM_SCAN_STOP;
                }
                else                            //���Զ����������ŵ�ǰƵ��
                {
                    scan_mode = FM_SCAN_STOP;
                    fm_module_mute(0);
                    break;
                }
            }
            else                                                    //����������ֻ��Ӧ�¼�
            {
                //put_msg_lifo(MSG_FM_SCAN_ALL);
                put_msg_fifo(MSG_FM_SCAN_ALL);
            }
			break;

        case MSG_FM_SCAN_ALL_DOWN:
            scan_mode = FM_SCAN_PREV;		
            put_msg_lifo(MSG_FM_SCAN_ALL);
            break;

        case MSG_FM_SCAN_ALL_UP:
            scan_mode = FM_SCAN_NEXT;
            put_msg_lifo(MSG_FM_SCAN_ALL);
			break;

        case MSG_FM_PREV_STEP:
            flush_all_msg();
            set_fre(FM_FRE_DEC);
            fm_mode_var.bFreChannel = get_channel_via_fre(fm_mode_var.wFreq - MIN_FRE);						//���Ҹ�Ƶ���Ƿ��м����
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
			set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
			fm_module_mute(0);
			UI_menu(MENU_FM_MAIN);
            break;

        case MSG_FM_NEXT_STEP:
            flush_all_msg();
            set_fre(FM_FRE_INC);
            fm_mode_var.bFreChannel = get_channel_via_fre(fm_mode_var.wFreq - MIN_FRE);						//���Ҹ�Ƶ���Ƿ��м����
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
			set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
			fm_module_mute(0);
			UI_menu(MENU_FM_MAIN);
            break;

        case MSG_FM_PREV_STATION:
            flush_all_msg();
            if (fm_mode_var.bTotalChannel == 0)
            {
                fm_module_mute(0);
                break;
            }
            fm_mode_var.bFreChannel -= 2;
        case MSG_FM_NEXT_STATION:
            if (fm_mode_var.bTotalChannel == 0)
            {
                fm_module_mute(0);
                break;
            }

            fm_mode_var.bFreChannel++;

            if ((fm_mode_var.bFreChannel == 0) || (fm_mode_var.bFreChannel == 0xff))
            {
                fm_mode_var.bFreChannel = fm_mode_var.bTotalChannel;
            }
            else if (fm_mode_var.bFreChannel > fm_mode_var.bTotalChannel)
            {
                fm_mode_var.bFreChannel = 1;
            }
            fm_mode_var.wFreq = get_fre_via_channle(fm_mode_var.bFreChannel) + MIN_FRE;				//����̨����Ƶ��
            set_fre(FM_CUR_FRE);
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
            set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
            fm_module_mute(0);
            UI_menu(MENU_FM_CHANNEL);
            break;

        case MSG_CH_SAVE:
            ch_save();
            break;

        case MSG_100MS:
            bike_task();
            break;
        case MSG_HALF_SECOND:
            LED_FADE_OFF();
            UI_menu(MENU_MAIN);
            break;

        case MSG_MUSIC_PP:
#ifdef UI_ENABLE
            if (UI_var.bCurMenu == MENU_INPUT_NUMBER)   //��ͣ�Ͳ���
            {
                put_msg_lifo(MSG_INPUT_TIMEOUT);
                break;
            }
#else
            break;
#endif

        case MSG_INPUT_TIMEOUT:
            /*�ɺ�����淵��*/
            if (input_number <= MAX_CHANNL)							//�������̨��
            {
                key = get_fre_via_channle(input_number);
                if (key != 0xff)
                {
                    fm_mode_var.wFreq = key + MIN_FRE;
                    fm_mode_var.bFreChannel = input_number;
                    set_fre(FM_CUR_FRE);
                    fm_module_mute(0);
                    UI_menu(MENU_FM_DISP_FRE);
                }
            }
            else if ((input_number >= MIN_FRE) && (input_number <= MAX_FRE)) //�������Ƶ��
            {
                fm_mode_var.wFreq = input_number;
                fm_mode_var.bFreChannel = get_channel_via_fre(fm_mode_var.wFreq - MIN_FRE);
                set_fre(FM_CUR_FRE);
                fm_module_mute(0);
            }
            input_number = 0;
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
			set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
            UI_menu(MENU_FM_DISP_FRE);
            break;
        case MSG_MUTE_UNMUTE:
          if ( key_mute ){
            key_mute = 0;
            //deg("MSG_UNMUTE\n");
            UI_menu(MENU_UNMUTE);
          } else {
            key_mute = 1;
            //deg("MSG_MUTE\n");
            UI_menu(MENU_MUTE);
          }
          dac_mute(key_mute);
          //fm_module_mute(key_mute);
          break;
        case MSG_STOP:
    		//for(i=0;i<100;i++)
	    		//delay_n10ms(100);

	        dac_mute(key_mute);
            fm_module_mute(1);
            set_memory(MEM_MEDIAMODE,work_mode);
            work_mode = OFF_MODE;
            UI_menu(MENU_STOP);
          return;
        default:
            ap_handle_hotkey(key);
            break;
        }

    }
}

/*----------------------------------------------------------------------------*/
/**@brief    FM ģʽ
   @param    ��
   @return   ��
   @note     void fm_mode(void)
*/
/*----------------------------------------------------------------------------*/
void fm_mode(void) AT(FM_CODE)
{
#ifdef SHARE_RTC_OSC_TO_FM
    P05_config(P05_OSC_CLK);
#endif

    if (init_fm_rev())
    {
        P2PD  |= BIT(5);
        P2DIE |= BIT(5);
        P2DIR &=~BIT(5);
        P25    = 0;
        //deg_puts("init_fm_rev ok\n");
        //P2HD &= ~0x7;
        //sd_chk_ctl(SET_SD_CHK_STEP, 255);
        fm_info_init();
        dac_channel_sel(DAC_AMUX1);
        system_clk_div(CLK_24M);
        dac_mute(0);
        fm_module_mute(0);
        key_mute = 0;
        set_eq(1, CLASSIC);
        fm_play();
        dac_channel_sel(DAC_DECODER);
        fm_rev_powerdown();
        //sd_chk_ctl(SET_SD_CHK_STEP, 20);
        //P2HD |= 0x7;
        P25    = 1;
    }
    else					// no fm module
    {
        work_mode++;
    }

#ifdef SHARE_RTC_OSC_TO_FM
    P05_config(P05_NORMAL_IO);
#endif
}
