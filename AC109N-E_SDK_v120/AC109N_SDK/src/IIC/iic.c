/*--------------------------------------------------------------------------*/
/**@file    iic.c
   @brief   IIC �ӿں���
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/#include "iic.h"
#include "RTC_API.h"
#include "sdmmc_api.h"
#include "play_file.h"
#include "device.h"
#include "IRTC.h"

bool iic_busy;

__code const u16 iic_io_tab[8] AT(TABLE_CODE)=
{
    (u16)iic_data_out,
    (u16)iic_data_in,
    (u16)iic_data_r,
    (u16)iic_data,
    (u16)iic_clk_out,
    (u16)iic_clk,
};


/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O �����������
    @param: void
    @return:void
    @author:Juntham
    @note:  void iic_data_out(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_data_out(void) AT(COMMON_CODE)
{
    P0DIR &= ~(1<<2);
}


/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O �������뺯��
    @param: void
    @return:void
    @author:Juntham
    @note:  void iic_data_in(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_data_in(void) AT(COMMON_CODE)
{
    P0DIR |= (1<<2);
    P0PU |= (1<<2);
}


/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O ��ȡ����
    @param: void
    @return:��ȡ��ֵ
    @author:Juntham
    @note:  bool iic_data_r(void)
*/
/*----------------------------------------------------------------------------*/
_near_func bool iic_data_r(void) AT(COMMON_CODE)
{
    return P02;
}

/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O ���ֵ���ú���
    @param: flag�������ƽ
    @return:void
    @author:Juntham
    @note:  void iic_data(bool flag)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_data(bool flag) AT(COMMON_CODE)
{
    P02 = flag;
}

/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O ���ֵ���ú���
    @param: flag�������ƽ
    @return:void
    @author:Juntham
    @note:  void iic_clk_out(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_clk_out(void) AT(COMMON_CODE)
{
    P0DIR &= ~(1<<3);
}

/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O ���ֵ���ú���
    @param: flag�������ƽ
    @return:void
    @author:Juntham
    @note:  void iic_clk(bool flag)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_clk(bool flag) AT(COMMON_CODE)
{
    P03 = flag;
}



/*----------------------------------------------------------------------------*/
/** @brief: IIC ģ���ʼ������
    @param: void
    @return:void
    @author:Juntham
    @note:  void iic_init(void)
*/
/*----------------------------------------------------------------------------*/
void iic_init(void) AT(IIC_CODE)
{
    set_iic_io((u16 __code *)iic_io_tab);       ///<���ýӿں���IO
}

void eeprom_verify(void) AT(IIC_CODE)
{
#ifdef CHECK_EEPROM_ON_POWER_ON   
	if ((get_memory(0) != 0x55)                 
        ||(get_memory(1) != 0xAA))
	{
        set_memory(0, 0x55);
        set_memory(1, 0xAA);
    }
    
    if ((get_memory(0) != 0x55)                 
        ||(get_memory(1) != 0xAA))
	{
        //���eeprom��Ч
    }
    else
    {
        //�����eeprom
        //my_puts("find eeprom\n");
    }
#endif
    
}
/*----------------------------------------------------------------------------*/
/** @brief: ������Ϣ���洢����EEPROM��
    @param: void
    @return:void
    @author:Juntham
    @note:  void set_memory(u8 addr, u8 dat)
*/
/*----------------------------------------------------------------------------*/
_near_func void set_memory(u8 addr, u8 dat) AT(COMMON_CODE)
{
#ifdef USE_EEPROM_MEMORY
    write_eerom(addr, dat);
#endif
#ifdef USE_IRTC_MEMORY
    write_IRTC_RAM(addr, dat);
#endif
}
/*----------------------------------------------------------------------------*/
/** @brief: ��ȡ������Ϣ��EEPROM��
    @param: void
    @return:void
    @author:Juntham
    @note:  u8 get_memory(u8 addr)
*/
/*----------------------------------------------------------------------------*/
_near_func u8 get_memory(u8 addr) AT(COMMON_CODE)
{
#ifdef USE_EEPROM_MEMORY
    return read_eerom(addr);
#endif
#ifdef USE_IRTC_MEMORY
    return read_IRTC_RAM(addr);
#endif
}


