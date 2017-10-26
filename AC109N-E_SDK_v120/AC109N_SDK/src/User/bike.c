
#include "config.h"
#include "key.h"
#include "iic.h"

#include "bl55072.h"
#include "display.h"
#include "bike.h"

#define ContainOf(x) (sizeof(x)/sizeof(x[0]))

const unsigned int  BatStatus48[] AT(BIKE_TABLE_CODE) = {420,427,432,439,444,448,455,459,466,470,0xFFFF};
const unsigned int  BatStatus60[] AT(BIKE_TABLE_CODE) = {520,526,533,540,547,553,560,566,574,580,0xFFFF};

unsigned int _xdata tick_100ms=0;
unsigned int _xdata speed_buf[16];
unsigned int _xdata vol_buf[32];
unsigned int _xdata temp_buf[4];

BIKE_STATUS _xdata bike;
__no_init BIKE_CONFIG _xdata config;


unsigned int Get_SysTick(void) AT(BIKE_CODE)
{
	unsigned int tick;
	
	EA = 0;
	tick = bike.Tick;
	EA = 1;
	
	return tick;
}

unsigned int Get_ElapseTick(unsigned int pre_tick) AT(BIKE_CODE)
{
	unsigned int tick = Get_SysTick();

	if ( tick >= pre_tick )	
		return (tick - pre_tick);
	else
		return (0xFFFF - pre_tick + tick);
}

#if 0
const long NTC_B3450[29][2] =
{
	251783,	-400,	184546,	-350,	137003,	-300,	102936,	-250,	78219,	-200,
	60072,	-150,	46601,	-100,	36495,	-50,	28837,	0,		22980,	50,
	18460,	100,	14942,	150,	12182,	200,	10000,	250,	8263,	300,
	6869,	350,	5745,	400,	4832,	450,	4085,	500,	3472,	550,
	2965,	600,	2544,	650,	2193,	700,	1898,	750,	1649,	800,
	1439,	850,	1260,	900,	1108,	950,	977,	1000
};

long NTCtoTemp(long ntc) AT(BIKE_CODE)
{
	unsigned char i,j;

	if ( ntc > NTC_B3450[0][0] ){
		return NTC_B3450[0][1];
	} else {
		for(i=0;i<sizeof(NTC_B3450)/sizeof(NTC_B3450[0][0])/2-1;i++){
			if ( ntc <= NTC_B3450[i][0] && ntc > NTC_B3450[i+1][0] )
				break;
		}
		if ( i == sizeof(NTC_B3450)/sizeof(NTC_B3450[0][0])/2-1 ){
			return NTC_B3450[28][1];
		} else {
			for(j=0;j<50;j++){
				if ( NTC_B3450[i][0] - (j*(NTC_B3450[i][0] - NTC_B3450[i+1][0])/50) <= ntc )
					return NTC_B3450[i][1] + j;
			}
			return NTC_B3450[i+1][1];
		}
	}
}

int GetTemp(void) AT(BIKE_CODE)
{
	static unsigned char index = 0;
	long temp;
	unsigned char i;

	temp = AD_var.wADValue[AD_BIKE_TEMP]>>6;

	temp_buf[index++] = temp;
	if ( index >= ContainOf(temp_buf) )
		index = 0;
	for(i=0,temp=0;i<ContainOf(temp_buf);i++)
		temp += temp_buf[i];
	temp /= ContainOf(temp_buf);

	temp = 10000*1024UL/(1024-temp)-10000;
	temp = NTCtoTemp(temp);
	if ( temp > 999  ) temp =  999;
	if ( temp < -999 ) temp = -999;
	
	return temp;
}
#endif

int GetVol(void) AT(BIKE_CODE)
{
	static unsigned char index = 0;
	long vol;
	unsigned char i;

	vol = (unsigned long)(AD_var.wADValue[AD_BIKE_VOL]>>6)*1033UL/1024UL;   //ADC/1024*103.3/3.3V*3.3V

	vol_buf[index++] = vol;
	if ( index >= ContainOf(vol_buf) )
		index = 0;
	for(i=0,vol=0;i<ContainOf(vol_buf);i++)
		vol += vol_buf[i];
	vol /= ContainOf(vol_buf);

	return vol;
}

void GetVolSample(void)
{
	static unsigned char index = 0;
	
    vol_buf[index++] = AD_var.wADValue[AD_BIKE_VOL];
    if ( index >= ContainOf(vol_buf) )
      index = 0;
}

unsigned char GetVolStabed(unsigned int* vol) AT(BIKE_CODE)
{
	unsigned long mid;
	//static int buf[32];
	static unsigned char index = 0;
	unsigned char i;
	
	for(i=0,mid=0;i<ContainOf(vol_buf);i++)
		mid += vol_buf[i];
	mid /= ContainOf(vol_buf);

	*vol = (mid>>6)*1033UL/1024UL;   //ADC/1024*103.3/3.3V*3.3V
	//*vol = (unsigned long)(AD_var.wADValue[AD_BIKE_VOL]>>6)*1033UL/1024UL;   //ADC/1024*103.3/3.3V*3.3V

    //deg("GetVolStabed %u\n",AD_var.wADValue[AD_BIKE_VOL]>>6);

	for(i=0,mid=0;i<32;i++)	mid += vol_buf[i];
	mid /= 32;
	for( i=0;i<32;i++){
		if ( mid > 20 && ((mid *100 / vol_buf[i]) > 101 ||  (mid *100 / vol_buf[i]) < 99) )
		//if ( mid > 5 && ((mid *100 / buf[i]) > 102 ||  (mid *100 / buf[i]) < 98) )
			return 0;
	}
	
	return 1;
}

unsigned char GetSpeed(void) AT(BIKE_CODE)
{
	static unsigned char index = 0;
	unsigned int speed,vol;
	unsigned char i;

	vol = AD_var.wADValue[AD_BIKE_SPEED]>>6;
    //deg("wADValue %u ",AD_var.wADValue[AD_BIKE_SPEED]>>6);
 	
	speed_buf[index++] = vol;
	if ( index >= ContainOf(speed_buf) )
		index = 0;

	for(i=0,vol=0;i<ContainOf(speed_buf);i++)
		vol += speed_buf[i];
	vol /= ContainOf(speed_buf);
    vol = (unsigned long)vol*1033/1024;
    //deg("vol %u\n",vol);
	
	if ( config.SysVoltage	== 48 ){	
      if ( vol < 210 ){
		speed = (unsigned long)vol*182UL/1024UL;        //ADC/1024*103.3/3.3*3.3V/21V*37 KM/H
      } else if ( vol < 240 ){
		speed = (unsigned long)vol*18078UL/102400UL;    //ADC/1024*103.3/3.3*3.3V/24V*42 KM/H
      } else/* if ( vol < 270 )*/{
		speed = (unsigned long)vol*18364UL/102400UL;    //ADC/1024*103.3/3.3*3.3V/27V*48 KM/H
      }
	} else if ( config.SysVoltage	== 60 ) {
      if ( vol < 260 ){
		speed = (unsigned long)vol*15098UL/102400UL;   //ADC/1024*103.3/3.3*3.3V/26V*38 KM/H
      } else if ( vol < 300 ){
		speed = (unsigned long)vol*15151UL/102400UL;   //ADC/1024*103.3/3.3*3.3V/30V*44 KM/H
      } else/* if ( vol < 335 )*/{
		speed = (unsigned long)vol*15110UL/102400UL;   //ADC/1024*103.3/3.3*3.3V/33.5V*49 KM/H
      }
	}
	if ( speed > 99 )
		speed = 99;
	
  return speed;
}

#define READ_TURN_LEFT()		(P32)
#define READ_TURN_RIGHT()		(P31)

void LRFlash_Task(void)
{
	static unsigned char left_on=0	,left_off=0;
	static unsigned char right_on=0	,right_off=0;
	static unsigned char left_count=0,right_count=0;

	if ( READ_TURN_LEFT() ){	//ON
        left_off = 0;
        if ( left_on ++ > 10 ){		//200ms 滤波
            if ( left_on > 100 ){
          	    left_on = 101;
                bike.bLFlashType = 0;
            }
           	if ( left_count < 0xFF-50 ){
	            left_count++;
            }
			bike.bLeftFlash	= 1;
			bike.bTurnLeft 	= 1;
        }
	} else {					//OFF
        left_on = 0;
        if ( left_off ++ == 10 ){
        	left_count += 50;	//500ms
			bike.bLeftFlash	= 0;
        } else if ( left_off > 10 ){
	        left_off = 11;
            bike.bLFlashType = 1;
            if ( left_count == 0 ){
				bike.bTurnLeft = 0;
            } else
				left_count --;
		}
	}
	
	if ( READ_TURN_RIGHT() ){	//ON
        right_off = 0;
        if ( right_on ++ > 10 ){
            if ( right_on > 100 ){
          	    right_on = 101;
                bike.bRFlashType = 0;
            }
           	if ( right_count < 0xFF-50 ){
				right_count++;
            }
			bike.bRightFlash	= 1;
			bike.bTurnRight 	= 1;
        }
	} else {					//OFF
        right_on = 0;
        if ( right_off ++ == 10 ){
        	right_count += 50;	//500ms
			bike.bRightFlash = 0;
        } else if ( right_off > 10 ){
	        right_off = 11;
            bike.bRFlashType = 1;
            if ( right_count == 0 ){
				bike.bTurnRight = 0;
            } else
				right_count --;
		}
	}
}

void Light_Task(void) AT(BIKE_CODE)
{
	unsigned char speed_mode=0;
	
	//if ( bike.YXTERR )
    {
		P3PU 	&=~(BIT(2)|BIT(1)|BIT(0));
		P3PD 	&=~(BIT(2)|BIT(1)|BIT(0));
		P3DIE 	|= (BIT(2)|BIT(1)|BIT(0));
		P3DIR 	|= (BIT(2)|BIT(1)|BIT(0));
	
		if( P30 ) {
		  bike.NearLight = 1;
		  P2PU  |= BIT(0);  //背光调节
		  P2HD  |= BIT(0);
		  P2DIE |= BIT(0);
		  P2DIR &=~BIT(0);
		  P20    = 0;
		} else {
		  bike.NearLight = 0;
		  P2PU  |= BIT(0);  //背光调节
		  P2HD  |= BIT(0);
		  P2DIE |= BIT(0);
		  P2DIR &=~BIT(0);
		  P20    = 1;
		}
		//if( P31 ) bike.bTurnRight = 1; else bike.bTurnRight = 0;
		//if( P32 ) bike.bTurnLeft  = 1; else bike.bTurnLeft  = 0;

		bike.PHA_Speed 	= (unsigned long)GetSpeed();
		bike.Speed 		= (unsigned long)bike.PHA_Speed*1000UL/config.SpeedScale;
    	//deg("PHA_Speed=%u,SpeedScale=%u,Speed=%u\n",bike.PHA_Speed,config.SpeedScale,bike.Speed);
    }	
}

void HotReset(void) AT(BIKE_CODE)
{
	if (config.bike[0] == 'b' &&
		config.bike[1] == 'i' &&
		config.bike[2] == 'k' &&
		config.bike[3] == 'e' ){
		bike.HotReset = 1;
	} else {
		bike.HotReset = 0;
	}
}

void WriteConfig(void) AT(BIKE_CODE)
{
	unsigned char *cbuf = (unsigned char *)&config;
	unsigned char i;

	config.bike[0] = 'b';
	config.bike[1] = 'i';
	config.bike[2] = 'k';
	config.bike[3] = 'e';
	for(config.Sum=0,i=0;i<sizeof(BIKE_CONFIG)-1;i++)
		config.Sum += cbuf[i];
		
	for(i=0;i<sizeof(BIKE_CONFIG);i++)
		set_memory(BIKE_EEPROM_START+i,cbuf[i]);
}

void InitConfig(void) AT(BIKE_CODE)
{
	unsigned char *cbuf = (unsigned char *)&config;
	unsigned char i,sum;

	for(i=0;i<sizeof(BIKE_CONFIG);i++)
		cbuf[i] = get_memory(BIKE_EEPROM_START + i);

	for(sum=0,i=0;i<sizeof(BIKE_CONFIG)-1;i++)
		sum += cbuf[i];
		
	if (config.bike[0] != 'b' ||
		config.bike[1] != 'i' ||
		config.bike[2] != 'k' ||
		config.bike[3] != 'e' ||
		sum != config.Sum ){
		config.SysVoltage 	= 60;
		config.VolScale  	= 1000;
		config.TempScale 	= 1000;
		config.SpeedScale	= 1000;
		config.YXT_SpeedScale= 1000;
		config.Mile			= 0;
	}

	bike.Mile = config.Mile;
#if ( TIME_ENABLE == 1 )
	bike.HasTimer = 0;
#endif
	//bike.SpeedMode = SPEEDMODE_DEFAULT;
	bike.YXTERR = 1;
	bike.bPlayFlash = 0;
    bike.Tick = 0;
	
    P3PU    &=~(BIT(3)|BIT(4));
    P3PD    &=~(BIT(3)|BIT(4));
    P3DIE   |= (BIT(3)|BIT(4));
    P3DIR   |= (BIT(3)|BIT(4));

	if ( P33 == 0 ){
        config.SysVoltage = 48;
    } else {
        config.SysVoltage = 60;
    }
    bike.bLFlashType = P34;
    bike.bRFlashType = P34;
}

unsigned char GetBatStatus(unsigned int vol) AT(BIKE_CODE)
{
	unsigned char i;
	unsigned int const _code * BatStatus;

	switch ( config.SysVoltage ){
	case 48:BatStatus = BatStatus48;break;
	case 60:BatStatus = BatStatus60;break;
	default:BatStatus = BatStatus60;break;
	}

	for(i=0;i<ContainOf(BatStatus60);i++)
		if ( vol < BatStatus[i] ) break;
	return i;
}

#define TASK_INIT	0
#define TASK_STEP1	1
#define TASK_STEP2	2
#define TASK_STEP3	3
#define TASK_STEP4	4
#define TASK_EXIT	5

unsigned char MileResetTask(void) AT(BIKE_CODE)
{
	static unsigned int pre_tick=0;
	static unsigned char TaskFlag = TASK_INIT;
	static unsigned char lastLight = 0;
	static unsigned char count = 0;
	
	if ( Get_ElapseTick(pre_tick) > 10000 | bike.Braked | bike.Speed )
		TaskFlag = TASK_EXIT;

	switch( TaskFlag ){
	case TASK_INIT:
		if ( Get_SysTick() < 3000 && bike.bTurnRight == 1 ){
			TaskFlag = TASK_STEP1;
			count = 0;
		}
		break;
	case TASK_STEP1:
		if ( lastLight == 0 && bike.NearLight){
			pre_tick = Get_SysTick();
			if ( ++count >= 8 ){
				bike.MileFlash = 1;
				bike.Mile = config.Mile;
				TaskFlag = TASK_STEP2;
			}
		}
		lastLight = bike.NearLight;
		break;
	case TASK_STEP2:
		if ( bike.bTurnRight == 0 && bike.bTurnLeft == 1 ) {
			TaskFlag = TASK_EXIT;
			bike.MileFlash 	= 0;
			bike.FMile 		= 0;
			bike.Mile 		= 0;
			config.Mile 	= 0;
			WriteConfig();
		}
		break;
	case TASK_EXIT:
	default:
		bike.MileFlash = 0;
		break;
	}
	return 0;
}

void MileTask(void) AT(BIKE_CODE)
{
	static unsigned int time = 0;
	unsigned char speed;
	
	speed = bike.Speed;
	if ( speed > DISPLAY_MAX_SPEED )
		speed = DISPLAY_MAX_SPEED;
	
#ifdef SINGLE_TRIP
	time ++;
	if ( time < 20 ) {	//2s
		bike.Mile = config.Mile;
	} else if ( time < 50 ) { 	//5s
		if ( speed ) {
			time = 50;
			bike.Mile = 0;
		}
	} else if ( time == 50 ){
		bike.Mile = 0;
	} else
#endif	
	{
		time = 51;
		
		bike.FMile = bike.FMile + speed;
		if(bike.FMile >= 36000)
		{
			bike.FMile = 0;
			bike.Mile++;
			if ( bike.Mile > 99999 )	bike.Mile = 0;
			config.Mile ++;
			if ( config.Mile > 99999 )	config.Mile = 0;
			WriteConfig();
		}
	}
}

#if ( TIME_ENABLE == 1 )
void TimeTask(void) AT(BIKE_CODE)
{
	static unsigned char time_task=0,left_on= 0,NL_on = 0;
	static unsigned int pre_tick;
	
	if (!bike.HasTimer)
		return ;
	
	if (bike.Speed > 1)
		time_task = 0xff;
	
	switch ( time_task ){
	case 0:
		if ( Get_SysTick() < 5000 && bike.NearLight == 0 && bike.bTurnLeft == 1 ){
			pre_tick = Get_SysTick();
			time_task++;
		}
		break;
	case 1:
		if ( bike.bTurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 2:
		if ( bike.bTurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 3:
		if ( bike.bTurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 4:
		if ( bike.bTurnLeft == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 5:
		if ( bike.bTurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 6:
		if ( bike.bTurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
	case 7:
		if ( bike.bTurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 8:
		if ( bike.bTurnLeft == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
	case 9:
		if ( bike.bTurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 10:
		if ( bike.bTurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
	case 11:
		if ( bike.bTurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 12:
		if ( bike.bTurnLeft == 1 || bike.NearLight ){
			 time_task = 0xFF;
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000 ) {
				time_task= 0;
				bike.time_pos = 0;
				bike.time_set = 1;
				pre_tick = Get_SysTick();
			}
		}
		break;
	default:
		bike.time_pos = 0;
		bike.time_set = 0;
		time_task = 0;
		break;
	}

	if ( bike.time_set ){
		if ( bike.bTurnLeft ) { left_on = 1; }
		if ( bike.bTurnLeft == 0 ) {
			if ( left_on == 1 ){
				bike.time_pos ++;
				bike.time_pos %= 4;
				left_on = 0;
				pre_tick = Get_SysTick();
			}
		}
		if ( bike.NearLight ) { NL_on = 1; pre_tick = Get_SysTick(); }
		if ( bike.NearLight == 0 && NL_on == 1 ) {
			NL_on = 0;
			if ( Get_ElapseTick(pre_tick) < 5000 ){
				switch ( bike.time_pos ){
				case 0:
					bike.Hour += 10;
					bike.Hour %= 20;
					break;
				case 1:
					if ( bike.Hour % 10 < 9 )
						bike.Hour ++;
					else
						bike.Hour -= 9;
					break;
				case 2:
					bike.Minute += 10;
					bike.Minute %= 60;
					break;
				case 3:
					if ( bike.Minute % 10 < 9 )
						bike.Minute ++;
					else
						bike.Minute -= 9;
					break;
				default:
					bike.time_set = 0;
					break;
				}
			}
			RtcTime.RTC_Hours 	= bike.Hour;
			RtcTime.RTC_Minutes = bike.Minute;
			//PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		}
		if ( Get_ElapseTick(pre_tick) > 30000 ){
			bike.time_set = 0;
		}
	}		
	
	//PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
	bike.Hour 		= RtcTime.RTC_Hours%12;
	bike.Minute 	= RtcTime.RTC_Minutes;
}

#endif


unsigned char SpeedCaltTask(void)
{
	static unsigned int pre_tick=0;
	static unsigned char TaskFlag = TASK_INIT;
	static unsigned char lastLight = 0,lastSpeed = 0;
	static unsigned char count = 0;
    static signed char Speed_dec=0;
	
    if ( TaskFlag == TASK_EXIT )
      	return 0;
	if ( Get_ElapseTick(pre_tick) > 10000 || bike.Braked )
		TaskFlag = TASK_EXIT;

	switch( TaskFlag ){
	case TASK_INIT:
		if ( Get_SysTick() < 3000 && bike.bTurnLeft == 1 ){
			TaskFlag = TASK_STEP1;
			count = 0;
		}
		break;
	case TASK_STEP1:
		if ( lastLight == 0 && bike.NearLight){
			if ( ++count >= 8 ){
				TaskFlag = TASK_STEP2;
			}
			pre_tick = Get_SysTick();
		}
		lastLight = bike.NearLight;
		break;
	case TASK_STEP2:
		if ( bike.bTurnLeft == 0 && bike.bTurnRight == 0 ) {
			TaskFlag 	= TASK_STEP3;
			count 		= 0;
			Speed_dec 	= 0;
			bike.SpeedFlash = 1;
			pre_tick = Get_SysTick();
		}
		break;
	case TASK_STEP3:
        if ( config.SysVoltage == 48 )
			bike.Speed = 42;
        else if ( config.SysVoltage == 60 )
			bike.Speed = 44;

		if ( lastLight == 1 && bike.NearLight == 0 ){
			pre_tick = Get_SysTick();
			if ( bike.bTurnLeft == 1 ) {
				count = 0;
	            if ( bike.Speed + Speed_dec )
					Speed_dec --;
			} else if ( bike.bTurnRight == 1 ) {
				count = 0;
				Speed_dec ++;
			} else {
				if ( ++count >= 5 ){
					TaskFlag = TASK_EXIT;
					bike.SpeedFlash = 0;
					//if ( bike.Speed )
                    {
						if ( bike.YXTERR )
							config.SpeedScale 		= (unsigned long)bike.Speed*1000UL/(bike.Speed+Speed_dec);
						else
							config.YXT_SpeedScale 	= (unsigned long)bike.Speed*1000UL/(bike.Speed+Speed_dec);
						WriteConfig();
					}
				}
			}
		}
		lastLight = bike.NearLight;

		//if ( lastSpeed && bike.Speed == 0 ){
		//	TaskFlag = TASK_EXIT;
		//}
		//if ( bike.Speed )
		//	pre_tick = Get_SysTick();

        bike.Speed += Speed_dec;
        lastSpeed = bike.Speed;
		break;
	case TASK_EXIT:
	default:
		bike.SpeedFlash = 0;
		break;
	}
	return 0;
}

#if 0
void Calibration(void) AT(BIKE_CODE)
{
	unsigned char i;
	unsigned int vol;
	
	CFG->GCR = CFG_GCR_SWD;
	//短接低速、SWIM信号
	GPIO_Init(GPIOD, GPIO_PIN_1, GPIO_MODE_OUT_OD_HIZ_SLOW);

	for(i=0;i<32;i++){
		GPIO_WriteLow (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE1_PORT	, SPMODE1_PIN) ) break;
		GPIO_WriteHigh (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE1_PORT	, SPMODE1_PIN)  == RESET ) break;
	}
	if ( i == 32 ){
		for(i=0;i<0xFF;i++){
			if ( GetVolStabed(&vol) && (vol > 120) ) break;
			//IWDG_ReloadCounter();
		}
		bike.Voltage		= vol;
		//bike.Temperature	= GetTemp();
		//bike.Speed		= GetSpeed();

		config.VolScale		= (unsigned long)bike.Voltage*1000UL/VOL_CALIBRATIOIN;					//60.00V
		//config.TempScale	= (long)bike.Temperature*1000UL/TEMP_CALIBRATIOIN;	//25.0C
		//config.SpeedScale = (unsigned long)bike.Speed*1000UL/SPEED_CALIBRATIOIN;				//30km/h
		//config.Mile = 0;
		WriteConfig();
	}
	
	for(i=0;i<32;i++){
		GPIO_WriteLow (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE2_PORT	, SPMODE2_PIN) ) break;
		GPIO_WriteHigh (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE2_PORT	, SPMODE2_PIN)  == RESET ) break;
	}
	if ( i == 32 ){
		bike.uart = 1;
	} else
		bike.uart = 0;

	CFG->GCR &= ~CFG_GCR_SWD;
}
#endif

void bike_PowerUp(void)
{
	HotReset();
	if ( bike.HotReset == 0 ){
		BL55072_Config(1);
	} else {
		BL55072_Config(0);
	}
}

#define BIKE_INIT 		0
#define BIKE_RESET_WAIT 1
#define BIKE_SETUP		2
#define BIKE_RUN		3

void bike_task(void) AT(BIKE_CODE)
{
	unsigned char i;
	unsigned int tick;
	static unsigned int count=0;
	unsigned int vol=0;
	static unsigned int task=BIKE_INIT;	
	
	bike.Tick += 100;
	
	switch(task){
	case BIKE_INIT:

		for(i=0;i<32;i++){	GetVolSample(); }
	//	for(i=0;i<16;i++){	GetSpeed();	/*IWDG_ReloadCounter(); */ }
	//	for(i=0;i<4;i++) {	GetTemp();	/*IWDG_ReloadCounter(); */ }

		InitConfig();
		//Calibration();

	#if ( TIME_ENABLE == 1 )	
		//bike.HasTimer = !PCF8563_Check();
		//bike.HasTimer = PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
	#endif
  	
		GetVolStabed(&vol);
		bike.Voltage = (unsigned long)vol*1000UL/config.VolScale;
		bike.BatStatus 	= GetBatStatus(bike.Voltage);

		if ( bike.HotReset == 0 ) {
			task = BIKE_RESET_WAIT;
		} else {
			task = BIKE_SETUP;
		}
		break;
	case BIKE_RESET_WAIT:
		if ( Get_SysTick() > PON_ALLON_TIME ){
			BL55072_Config(0);
			task = BIKE_SETUP;
		}
		break;
    case BIKE_SETUP:
        task = BIKE_RUN;
	case BIKE_RUN:
		//tick = Get_SysTick();
		
		//if ( (tick >= tick_100ms && (tick - tick_100ms) >= 100 ) || \
		//	 (tick <  tick_100ms && (0xFFFF - tick_100ms + tick) >= 100 ) ) {
		//	tick_100ms = tick;
			count ++;

            if ( (count % 10) == 0 ){
				if ( GetVolStabed(&vol) )
					bike.Voltage = (unsigned long)vol*1000UL/config.VolScale;
            	//bike.Voltage    = GetVol()+400;
				bike.BatStatus 	= GetBatStatus(bike.Voltage);
            }
			//if ( (count % 10) == 0 ){
			//	bike.Temperature= (long)GetTemp()	*1000UL/config.TempScale;
			//	bike.Temperature= GetTemp();
			//}
		
			Light_Task();
			MileTask();
			
					
		#if ( TIME_ENABLE == 1 )	
			TimeTask();
		#endif

		#ifdef RESET_MILE_ENABLE	
			MileResetTask();
		#endif	

            SpeedCaltTask();

		#ifdef LCD_SEG_TEST
			if ( count >= 100 ) count = 0;
			bike.Voltage 	= count/10 + count/10*10UL + count/10*100UL + count/10*1000UL;
			bike.Temperature= count/10 + count/10*10UL + count/10*100UL;
			bike.Speed		= count/10 + count/10*10;
			bike.Mile		= count/10 + count/10*10UL + count/10*100UL + count/10*1000UL + count/10*10000UL;
			bike.Hour       = count/10 + count/10*10;
			bike.Minute     = count/10 + count/10*10;
		#endif

            MenuUpdate(&bike);
	
		//}
		break;
	default:
		task = BIKE_INIT;
		break;
	}
}

