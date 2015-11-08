//-----------------------------------------------------------------------------
//   File:      CyDown.c
//-----------------------------------------------------------------------------
#include "lp.h"
#include "lpregs.h"
#include "syncdly.h"            // Synchronization delay

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define DELAY_COUNT   0x9248*8L  // 延时计数
#define _IFREQ  48000            
#define _CFREQ  48000           

//-----------------------------------------------------------------------------
// 比较函数
//-----------------------------------------------------------------------------
#define   min(a,b) (((a)<(b))?(a):(b))
#define   max(a,b) (((a)>(b))?(a):(b))

//-----------------------------------------------------------------------------
// 全局变量
//-----------------------------------------------------------------------------
volatile BOOL   GotSUD;
         BOOL   Rwuen;
         BOOL   Selfpwr;
volatile BOOL   Sleep;                  // 休眠模式使能信号

// Structures defined in dscr.a51
WORD   pDeviceDscr;   
WORD   pDeviceQualDscr;
WORD   pHighSpeedConfigDscr;
WORD   pFullSpeedConfigDscr;   
WORD   pConfigDscr;
WORD   pOtherConfigDscr;   
WORD   pStringDscr;   

//-----------------------------------------------------------------------------
// 函数声明
//-----------------------------------------------------------------------------
void SetupCommand(void);
void TD_Init(void);
void TD_Poll(void);
BOOL TD_Suspend(void);
BOOL TD_Resume(void);

BOOL DR_GetDescriptor(void);
BOOL DR_SetConfiguration(void);
BOOL DR_GetConfiguration(void);
BOOL DR_SetInterface(void);
BOOL DR_GetInterface(void);
BOOL DR_GetStatus(void);
BOOL DR_ClearFeature(void);
BOOL DR_SetFeature(void);
BOOL DR_VendorCmnd(void);
void DR_READ_FROM_ADDR(void);
void DR_WRITE_TO_ADDR(void);
void DR_READ_NAK_CNT(void);
void DR_READ_BYTES(void);
void DR_SET_ADDR(void);

// 端点映射
const char code  EPCS_Offset_Lookup_Table[] =
{
   0,    // EP1OUT
   1,    // EP1IN
   2,    // EP2OUT
   2,    // EP2IN
   3,    // EP4OUT
   3,    // EP4IN
   4,    // EP6OUT
   4,    // EP6IN
   5,    // EP8OUT
   5,    // EP8IN
};

#define epcs(EP) (EPCS_Offset_Lookup_Table[(EP & 0x7E) | (EP > 128)] + 0xE6A1)

//-----------------------------------------------------------------------------
// 主程序
//-----------------------------------------------------------------------------

void main(void)
{
   DWORD  i;
   WORD   offset;
   DWORD  DevDescrLen;
   DWORD  j=0;
   WORD   IntDescrAddr;
   WORD   ExtDescrAddr;

   // 初始化
   Sleep = FALSE;              // 禁止休眠模式
   Rwuen = FALSE;              // 禁止远程唤醒
   Selfpwr = FALSE;            // 禁止自供电
   GotSUD = FALSE;       

   // 初始化用户设备
   TD_Init();

   //定向USB描述符
   pDeviceDscr = (WORD)&DeviceDscr;
   pDeviceQualDscr = (WORD)&DeviceQualDscr;
   pHighSpeedConfigDscr = (WORD)&HighSpeedConfigDscr;
   pFullSpeedConfigDscr = (WORD)&FullSpeedConfigDscr;
   pStringDscr = (WORD)&StringDscr;

   if ((WORD)&DeviceDscr & 0xC000)
   {
      // 重定向描述符
      IntDescrAddr = INTERNAL_DSCR_ADDR;
      ExtDescrAddr = (WORD)&DeviceDscr;
      DevDescrLen = (WORD)&UserDscr - (WORD)&DeviceDscr + 2;
      for (i = 0; i < DevDescrLen; i++)
         *((BYTE xdata *)IntDescrAddr+i) = *((BYTE xdata *)ExtDescrAddr+i);

      // 更新描述符指针
      pDeviceDscr = IntDescrAddr;
      offset = (WORD)&DeviceDscr - INTERNAL_DSCR_ADDR;
      pDeviceQualDscr -= offset;
      pConfigDscr -= offset;
      pOtherConfigDscr -= offset;
      pHighSpeedConfigDscr -= offset;
      pFullSpeedConfigDscr -= offset;
      pStringDscr -= offset;
   }

   EZUSB_IRQ_ENABLE();            // 使能USB中断
   EZUSB_ENABLE_RSMIRQ();         // 远程唤醒中断

   INTSETUP |= (bmAV2EN | bmAV4EN);     // 使能INT 2 & 4 自动向量

   USBIE |= bmSUDAV | bmSUTOK | bmSUSP | bmURES | bmHSGRANT;   // 使能选择的中断
   EA = 1;                  // 使能8051全局中断

#ifndef NO_RENUM
   // 检查重列举
   if(!(USBCS & bmRENUM))
   {
       EZUSB_Discon(TRUE);   //重列举
   }
#endif

   // 连接
   USBCS &=~bmDISCON;

   CKCON = (CKCON&(~bmSTRETCH)) | FW_STRETCH_VALUE;

   //清Sleep标记
   Sleep = FALSE;

   //任务县城线程
   while(TRUE)               //主循环
   {
      //列举用户设备
      TD_Poll();

      if(GotSUD)
      {
         SetupCommand();          
         GotSUD = FALSE;          // 清SETUP标记
      }

      //检查并处理
      if (Sleep)
      {
         if(TD_Suspend())
         { 
            Sleep = FALSE;     //清Sleep标记
            do
            {
               EZUSB_Susp();         //空闲状态处理
            } while(!Rwuen && EZUSB_EXTWAKEUP());
            EZUSB_Resume();   
            TD_Resume();
         }   
      }
   }  // while(TRUE)
}

BOOL HighSpeedCapable()
{
   // 高速USB处理
   if (REVID & 0xF0)    //检查是fx2lp/fx1还是fx2.  
   {
      // fx2lp 或 fx1 的处理
      if (GPCR2 & bmHIGHSPEEDCAPABLE)
         return TRUE;
      else
         return FALSE;
   }
   else
   {
      // fx2 的处理
      return TRUE;
   }
}   

// 设备请求
void SetupCommand(void)
{
   void   *dscr_ptr;

   switch(SETUPDAT[1])
   {
      case SC_GET_DESCRIPTOR:                  //获得描述符
         if (DR_GetDescriptor())
		 {
            switch(SETUPDAT[3])         
            {
               case GD_DEVICE:            //设备
                  SUDPTRH = MSB(pDeviceDscr);
                  SUDPTRL = LSB(pDeviceDscr);
                  break;

               case GD_DEVICE_QUALIFIER:            //设备限定
			   	  if (HighSpeedCapable())
				  {
	                  SUDPTRH = MSB(pDeviceQualDscr);
	                  SUDPTRL = LSB(pDeviceQualDscr);
				  }
				  else
				  {
					  EZUSB_STALL_EP0();
				  }
				  break;

               case GD_CONFIGURATION:         // 配置
                  SUDPTRH = MSB(pConfigDscr);
                  SUDPTRL = LSB(pConfigDscr);
                  break;

               case GD_OTHER_SPEED_CONFIGURATION:  // 其它速率配置
                  SUDPTRH = MSB(pOtherConfigDscr);
                  SUDPTRL = LSB(pOtherConfigDscr);
                  break;

               case GD_STRING:            // 字符串
                  if(dscr_ptr = (void *)EZUSB_GetStringDscr(SETUPDAT[2]))
                  {
                     SUDPTRH = MSB(dscr_ptr);
                     SUDPTRL = LSB(dscr_ptr);
                  }
                  else
				  {
                     EZUSB_STALL_EP0();   // 中止端点0
				  }
                  break;

               default:            // 无效请求
                  EZUSB_STALL_EP0();      // 中止端点0
            }
		 } // if (DR_GetDescriptor())
         break;

      case SC_GET_INTERFACE:                  // 获得接口
         DR_GetInterface();
         break;

      case SC_SET_INTERFACE:                  // 设置接口
         DR_SetInterface();
         break;

      case SC_SET_CONFIGURATION:               //设置配置
         DR_SetConfiguration();
         break;

      case SC_GET_CONFIGURATION:               //获得配置
         DR_GetConfiguration();
         break;

      case SC_GET_STATUS:                  // 获得状态
         if (DR_GetStatus())
		 {
            switch(SETUPDAT[0])
            {
               case GS_DEVICE:            // 设备
                  EP0BUF[0] = ((BYTE)Rwuen << 1) | (BYTE)Selfpwr;
                  EP0BUF[1] = 0;
                  EP0BCH = 0;
                  EP0BCL = 2;
                  break;

               case GS_INTERFACE:         // 接口
                  EP0BUF[0] = 0;
                  EP0BUF[1] = 0;
                  EP0BCH = 0;
                  EP0BCL = 2;
                  break;

               case GS_ENDPOINT:         // 端点
                  EP0BUF[0] = *(BYTE xdata *) epcs(SETUPDAT[4]) & bmEPSTALL;
                  EP0BUF[1] = 0;
                  EP0BCH = 0;
                  EP0BCL = 2;
                  break;

               default:            //无效命令
                  EZUSB_STALL_EP0();    
            }
		 }  // if (DR_GetStatus())
         break;

      case SC_CLEAR_FEATURE:                  // 清除特性
         if (DR_ClearFeature())
		 {
            switch(SETUPDAT[0])
            {
               case FT_DEVICE:            // 设备
                  if(SETUPDAT[2] == 1)
                     Rwuen = FALSE;       // 禁止远程唤醒
                  else
                     EZUSB_STALL_EP0();
                  break;

               case FT_ENDPOINT:     
                  if(SETUPDAT[2] == 0)
                  {
                     *(BYTE xdata *) epcs(SETUPDAT[4]) &= ~bmEPSTALL;
                     EZUSB_RESET_DATA_TOGGLE( SETUPDAT[4] );
                  }
                  else
				  {
                     EZUSB_STALL_EP0();
				  }
                  break;
            }
		 }  // if (DR_ClearFeature())
         break;

      case SC_SET_FEATURE:                  // 设置特性
         if (DR_SetFeature())
		 {
            switch(SETUPDAT[0])
            {
               case FT_DEVICE:        
                  if(SETUPDAT[2] == 1)
                     Rwuen = TRUE;     
                  else if(SETUPDAT[2] == 2)
                     break;
                  else
                     EZUSB_STALL_EP0();  
                  break;

               case FT_ENDPOINT:     
                  *(BYTE xdata *) epcs(SETUPDAT[4]) |= bmEPSTALL;
                  break;

               default:
                  EZUSB_STALL_EP0();     
            }
		 }
         break;

	  case SC_READ_FROM_ADDR:
	    DR_READ_FROM_ADDR();
		break;

      case SC_WRITE_TO_ADDR:
	    DR_WRITE_TO_ADDR();
		break;

	  case SC_RED_NAK:
	    DR_READ_NAK_CNT();
		break;

	  case SC_READ_BYTES:
	    DR_READ_BYTES();
		break;

	  case SC_SET_ADDR:
	    DR_SET_ADDR();
		break;

      default:
		break;
   }

   EP0CS |= bmHSNAK;
}

// 唤醒中断
void resume_isr(void) interrupt WKUP_VECT
{
   EZUSB_CLEAR_RSMIRQ();
}

