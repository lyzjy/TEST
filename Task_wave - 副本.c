#include <task_wave_gui.h>
#include <task_param_gui.h>
#include <gui.h>
#include <i_kbd.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <freertos_cmd.h>
#include <task_key.h>
#include "semphr.h"
#include "task_wave_export.h"
#include <common.h>
#include <i_gui.h>
#include <i_ser.h>
static ID_TIPS_INDEX gTipsDispIndex = ID_TIPS_INDEX_MAX;
static volatile EventGroupHandle_t event_group = NULL;
#define TIMEOUT_CLOSE 5*1000
#define VOLTAGE_OPEN  0x01
#define TIME_OPEN    (VOLTAGE_OPEN << 1)
#define TRIGGER_OPEN (TIME_OPEN << 1)
#define TRIGGER_DEPTH_OPEN (TRIGGER_OPEN << 1)
#define TIPS_DISP_OPEN (TRIGGER_DEPTH_OPEN << 1)
#define MEASURE_PARA_DISP (TIPS_DISP_OPEN << 1)
#define HARDWARE_MEASURE_PARA_DISP (MEASURE_PARA_DISP << 1)

#define GUI_BUTTON_OFFCOLOR     	GUI_MAKE_COLOR(0xb0e6ff00)
#define GUI_BUTTON_BKCOLOR   		GUI_MAKE_COLOR(0x2D325000)
#define GUI_WAVE_BKCOLOR   		GUI_MAKE_COLOR(0x00808080)
#define GUI_BUTTON_ONCOLOR   		GUI_MAKE_COLOR(0x99349900)
#define GUI_OPTION_BLUE_COLOR   	GUI_MAKE_COLOR(0x00FF0000)
#define GUI_WAVE_BROWN_COLOR   	GUI_MAKE_COLOR(0xF80b0b00)


static void DrawDynWave(void*p);//绘制动态波形
void SetTriggerDepthHandle(int32_t val, int dispFlag);
static void DrawWave(int channel,int index,int Start, int End,int x0,int flag);
OscInfoT gOscInfo = {0};
typedef struct button_msg_st
{
	int x;
	int y;
	int width;
	int height;
	unsigned char *pstr;
}BUTTON_MSG_ST;

typedef struct option_box_st
{
	char ColPos;
	char RowPos;
	unsigned char *pname;
	unsigned char *popt[4];
}OPTION_BOX_ST;


BUTTON_MSG_ST WAVE_BUTTON[5]=
{
	{409,19,67,46,"CH1设置"},
	
	{409,71,67,46,"CH2设置"},
	{409,123,67,46,"波形计算"},
	{409,175,67,46,"采集模式"},
	{409,227,67,46,"时基模式"},
};
OPTION_BOX_ST WAVE_OPTION[5][8]=
{
	{
		{0,0,"触发模式",		"交替","","NULL"},
		{1,0,"通道选择",		"CH1","CH2","NULL"},
		{2,0,"类型",		"边沿","test2","test3","NULL"},
		{3,0,"边沿类型",		"上升","下降","test3","NULL"},
		{0,0,"AAA5",		"test1","test2","test3","NULL"},
		{1,0,"NULL",0,0,0},
		{2,0,"NULL",0,0,0},
		{3,0,"NULL",0,0,0},
	},
	{
		{0,0,"BBB1",		"test1","test2","NULL"},
		{1,0,"BBB2",		"test1","test2","NULL"},
		{2,0,"BBB3",		"test1","test2","test3","NULL"},
		{3,0,"BBB4",		"test1","test2","test3","NULL"},
		{0,0,"BBB5",		"test1","test2","test3","NULL"},	
		{1,0,"NULL",0,0,0},
		{2,0,"NULL",0,0,0},
		{3,0,"NULL",0,0,0},	
	},
	{
		{0,0,"CCC1",		"test1","test2","NULL"},
		{1,0,"CCC2",		"test1","test2","NULL"},
		{2,0,"CCC3",		"test1","test2","test3","NULL"},
		{3,0,"CCC4",		"test1","test2","test3","NULL"},
		{0,0,"CCC5",		"test1","test2","test3","NULL"},
		{1,0,"NULL",0,0,0},
		{2,0,"NULL",0,0,0},
		{3,0,"NULL",0,0,0},
	},
	{
		{0,0,"DDD1",		"test1","test2","NULL"},
		{1,0,"DDD2",		"test1","test2","NULL"},
		{2,0,"DDD3",		"test1","test2","NULL"},
		{3,0,"NULL",0,0,0},
		{0,0,"NULL",0,0,0},
		{1,0,"NULL",0,0,0},
	},
	{
		{0,0,"EEE1",		"test1","test2","NULL"},
		{1,0,"EEE2",		"test1","test2","NULL"},
		{2,0,"EEE3",		"test1","test2","test3","NULL"},
		{3,0,"EEE4",		"test1","test2","test3","NULL"},
		{0,0,"EEE5",		"test1","test2","test3","NULL"},		
		{1,0,"EEE6",		"test1","test2","test3","NULL"},		
		{2,0,"EEE7",		"test1","test2","test3","test4"},	
		{3,0,"NULL",0,0,0}, 		
	},
};

#if 1
int wave_gui_handle_init(void)
{
	if (event_group == NULL)
		event_group = xEventGroupCreate();
	return 0;
}
void wave_gui_send_msg(uint32_t val)
{	
	EventBits_t evetns;
	//WAVE_GUI_DEBUG("%s event_group:0x%x val:%d\n",__func__,event_group,val);
	if (event_group)
	{
		 evetns = xEventGroupSetBits(event_group,val);
		 WAVE_GUI_DEBUG("evetns:%d val:%d\n",evetns,val);
	}
}

int wave_gui_recv_msg(EventBits_t *val)
{
	EventBits_t event_bits;
	if (event_group == NULL)
		return -1;
	//exec_printf("param_recv_msg Enter\n");
	event_bits = xEventGroupWaitBits(event_group,	 /* The event group handle. */
											 WAVE_GUI_WAIT_EVENTS,		 /* The bit pattern the event group is waiting for. */
											 pdTRUE,		 /* BIT_0 and BIT_4 will be cleared automatically. */
											 pdFALSE,		 /* Don't wait for both bits, either bit unblock task. */
											 portMAX_DELAY); /* Block indefinitely to wait for the condition to be met. */
	WAVE_GUI_DEBUG("%s event_bits:0x%x\n",__func__,event_bits);
	if (val)
		*val = event_bits;
	return 0;
}
#endif
void int_call_back_handle(int cmd, void *val)
{
	EventBits_t EventBits = 0;
	switch (cmd)
	{
		case CMD_SEND_EVENTS:{
				// 关中断
	           	//FpgaSpiWrite(gFpgaFd,0x80600000);
	           	//EventBits = xEventGroupGetBitsFromISR(event_group);
	           	
				EventBits |= (EventBits_t)val;
	           	xEventGroupSetBitsFromISR(event_group,EventBits,NULL);
				//gpio_irq_disable(gTriggerPin);
				//FpgaSpiWrite(gFpgaFd,0x80600000);
				//WAVE_GUI_DEBUG("gFpgaFd:%d %s %d\n",gFpgaFd,__func__,(int)EventBits);
			}
			break;
		default:
			break;
		
	}
}
void ShowRealTimeWave(void)
{
	static GUI_MEMDEV_Handle hMD = NULL;
	int loop;
	int channel;
	const ChannelSelT *pChannelSel = GetChannelSel();
	const WaveCtrlDispT *pWaveCtrlDisp = GetWaveCtrlDisp();
	RealTimeWaveExec();
	if (GetGlowOnOff())
	{
		if (gUpdateGlow == 1)
		{
			GUI_MEMDEV_Draw_Pri(&hMD,pWaveCtrlDisp->pDispRect,pWaveCtrlDisp->DrawWaveFunc,pWaveCtrlDisp,0,0); //GUI无闪烁绘制波形
			gUpdateGlow = 0;
		}
		SAMPLE_METHOD_T tmpSampleMethod;
		int count;
		for (loop = 0; loop < pChannelSel->count; loop++)
		{
			channel = pChannelSel->channelIndexArr[loop];
			GUI_SetColor(GetWaveColorWithChannel(channel));
			tmpSampleMethod = GetSampleMethod(channel);
			count = GetSampleCount(tmpSampleMethod);
			for (int index = 0; index < count; index++)
			{
				DrawWave(channel,index,pWaveCtrlDisp->pToXWin[channel].Start,pWaveCtrlDisp->pToXWin[channel].End,pWaveCtrlDisp->pDispRect->x0+1,0);
			}
		}
	}
	else
	{
		GUI_MEMDEV_Draw_Pri(&hMD,pWaveCtrlDisp->pDispRect,pWaveCtrlDisp->DrawWaveFunc,pWaveCtrlDisp,0,0); //GUI无闪烁绘制波形
	}
}


void WAVE_FillRoundedRectColor(GUI_COLOR color, int x0, int y0, int x1, int y1, int r)
{
	GUI_SetColor(color);
	GUI_FillRoundedRect(x0,y0, x1, y1,r);
}

void WAVE_Draw3dRect(int x0, int y0, int x1, int y1, char r, char yShadow, char status)
{
		if(status == 0)
		{
			WAVE_FillRoundedRectColor(GUI_BUTTON_BKCOLOR, x0+2,y0+yShadow,x1,y1,r);
			WAVE_FillRoundedRectColor(GUI_BUTTON_OFFCOLOR, x0,y0,x1-2,y1-yShadow,r);
		}		
		else
		{
			WAVE_FillRoundedRectColor(GUI_WAVE_BKCOLOR, x0+2,y0+yShadow,x1,y1,r);
			WAVE_FillRoundedRectColor(GUI_BUTTON_ONCOLOR, x0,y0,x1-2,y1-yShadow,r);
		}		
}
void WAVE_Refresh3dRect(int x0, int y0, int x1, int y1, char r, char yShadow)
{
	WAVE_FillRoundedRectColor(GUI_BUTTON_OFFCOLOR, x0,y0,x1-2,y1-yShadow,r);
}
void WAVE_DispString(const char * s, int x, int xwidth, int y, GUI_COLOR color)
{
		GUI_SetTextMode(GUI_TM_TRANS);  //透明文本
   	 	GUI_SetColor(color);
		char *ps = s;
		char ssa[3];
		int TotalWidth = 0;
		int xstart = 0;
		const char HZwidth = 12;
		const char ASCALLwidth = 8;
		while(*s)
		{
			if(*s<127)
			{
				TotalWidth += ASCALLwidth;
				s++;
			}
			else
			{
				TotalWidth += HZwidth;
				s+=2;
			}
		}
		xstart = x + (xwidth/2) - (TotalWidth/2);	
		TotalWidth=0;
		while(*ps)
		{
			if(*ps < 127)
			{
	    		GUI_SetFont(&GUI_Font8x16);        //设置字体类型
	    		ssa[0]= *ps;
	    		ssa[1]= 0;
				ps++;
				GUI_DispStringAt(ssa, xstart + TotalWidth, y);
	    		TotalWidth += 8;
			}
			else
			{
				GUI_SetFont(&GUI_FontHZ12x12);//设置中文字体
				ssa[0]= *ps;
				ps++;
				ssa[1]=*ps;
				ssa[2]=0;
				ps++;
				GUI_DispStringAt(ssa, xstart + TotalWidth, y);
				TotalWidth += 12;
			}
		}
}


void testString(const char * s, int x, int y)
{
		GUI_SetTextMode(GUI_TEXTMODE_NORMAL);  //透明文本
   	 	GUI_SetColor(GUI_BLACK);
    	GUI_SetFont(GUI_FONT_24_ASCII);        //设置字体类型
		GUI_DispStringAt(s, x, y);
}

void WAVE_DrawMenu(BUTTON_MSG_ST *pbutton, char status)
{
	int x0 = pbutton->x;
	int y0 = pbutton->y;
	int width = pbutton->width;
	int height = pbutton->height;
	if(status == 0)
	{
		WAVE_Draw3dRect(x0,y0,x0+width,y0+height,6, 5, 0);	
	}
	else 
	{
		WAVE_Draw3dRect(x0,y0,x0+width,y0+height,6, 5, 1);	
	}
	WAVE_DispString(pbutton->pstr,x0, width, y0+13,GUI_BLACK);
} 

void WAVE_DrawOptionBox(OPTION_BOX_ST *poption, char status)
{
	const int OptionBoxPos[4][4]=
	{
		{1,285},{97,285},{193,285},{289,285},
	};
	const char width = 93;
	const char height = 33;
	int x0 = 	OptionBoxPos[poption->ColPos][0];
	int y0 = 	OptionBoxPos[poption->ColPos][1];
	if(status == 2)
	{
		WAVE_Draw3dRect(x0, y0, x0+width, y0+height,5, 2, 0);
	}
	else if(status == 1)
	{ 
		WAVE_Draw3dRect(x0, y0, x0+width, y0+height,5, 2, 1);
		if(strcmp(poption->pname,"NULL"))
		{
			WAVE_FillRoundedRectColor(GUI_OPTION_BLUE_COLOR, (x0+(width/2)-21), y0+1, (x0+(width/2)-21)+42, (y0+1)+16, 2);		
			(poption->RowPos)++;
			if(!strcmp(poption->popt[poption->RowPos],"NULL"))
			{
				poption->RowPos =0;
			}
			if(poption->RowPos == 4)
			{
				poption->RowPos =0;
			}
			WAVE_DispString(poption->popt[(poption->RowPos)],(x0+(width/2)-21), 42, y0+2, GUI_BLACK);
			WAVE_DispString(poption->pname,x0, width, y0+18, GUI_BLACK);
		}
		mdelay(500);
		WAVE_Draw3dRect(x0, y0, x0+width, y0+height,5, 2, 0);
	}
	else
	{
		WAVE_Refresh3dRect(x0, y0, x0+width, y0+height,5, 2);
	}
	if(strcmp(poption->pname,"NULL"))
	{
		WAVE_FillRoundedRectColor(GUI_OPTION_BLUE_COLOR, (x0+(width/2)-21), y0+1, (x0+(width/2)-21)+42, (y0+1)+16, 2);			
		GUI_SetColor(GUI_WHITE);
		WAVE_DispString(poption->popt[poption->RowPos],(x0+(width/2)-21), 42, y0+2, GUI_WHITE);	
		WAVE_DispString(poption->pname,x0,width, y0+18, GUI_BLACK);
	}
}

char WAVE_DrawPage(char ButtonFlag, char status)
{
	const char TotalPick[5]= {5,5,5,3,7};
	const char TotalPage[5]={2,2,2,1,2};
	const int x0 = 385;
	const int y0 = 285;
	const int width = 93;
	const int height = 33;
	static int oldButtonFlag = -1;
	static char nowPage = 0;
	char result = 0;
	unsigned char str[4]={0};
	if(status == 1)
	{
		nowPage ++;
		result += 4;
		
		if(TotalPage[ButtonFlag] < nowPage)
		{
			nowPage = 1;
			result = 0;
		}
	}
	if(oldButtonFlag != ButtonFlag)
	{
		nowPage = 1; 
		result =0;
		oldButtonFlag = ButtonFlag;
	}
	if(TotalPick[ButtonFlag] <= 4)
	{
		str[0]=0;
		nowPage =1;
		result = 0;
	}
	else
	{
		sprintf(str,"%d/%d", nowPage, TotalPage[ButtonFlag]);
	}
	if(status == 0)
	{
		WAVE_Draw3dRect(x0,y0,x0+width,y0+height,5, 2, status);	
	}		
	else
	{
		WAVE_Draw3dRect(x0,y0,x0+width,y0+height,5, 2, 1);	
		WAVE_DispString(str,x0, width ,y0+18, GUI_BLACK);
		mdelay(500);	
		WAVE_Draw3dRect(x0,y0,x0+width,y0+height,5, 2, 0);	
	}	
	WAVE_DispString(str, x0, width, y0+18, GUI_BLACK);
	return result;
}


void WAVE_MenuCtrl(char MenuPos, int FeatPos)
{
	static int oldMenuPos = -1;
	static char PosHead = 0;
	if(oldMenuPos == -1)
	{
		for(char i=0;i<5;i++)
		{
			if(i != MenuPos)
			{
				WAVE_DrawMenu(&WAVE_BUTTON[i],0);
			}
		}
		WAVE_DrawMenu(&WAVE_BUTTON[MenuPos],1);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][0],2);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][1],2);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][2],2);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][3],2);
		WAVE_DrawPage(0,0);
		oldMenuPos = MenuPos;
	}
	else if(oldMenuPos != MenuPos)
	{
		WAVE_DrawMenu(&WAVE_BUTTON[oldMenuPos],0);	
		WAVE_DrawMenu(&WAVE_BUTTON[MenuPos],1);

		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][0],0);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][1],0);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][2],0);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][3],0);
		WAVE_DrawPage(MenuPos,0);
		PosHead = 0;
		oldMenuPos = MenuPos;
	}
	if(FeatPos != 4 && FeatPos != -1)
	{
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][PosHead + FeatPos],1);
	}
	if(FeatPos == 4)
	{
		PosHead = WAVE_DrawPage(MenuPos,1);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][PosHead],0);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][PosHead+1],0);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][PosHead+2],0);
		WAVE_DrawOptionBox(&WAVE_OPTION[MenuPos][PosHead+3],0);
	}	
}

void ui_test()
{
	WAVE_MenuCtrl(0,-1);
	Ser_PortOpen(SER_PORT_NUM_COM1, "115200,N,8,1");
	char readlen=0;
	char readdata=0;
	char aa=0;
	char bb =0;//
	while(1)
	{
		Ser_PortRead(SER_PORT_NUM_COM1, 1,&readdata,2000,&readlen);
		switch(readdata)
		{
		case '1':
			aa = 0;
			WAVE_MenuCtrl(aa,-1);
			break;
		case '2':
			aa = 1;
			WAVE_MenuCtrl(aa,-1);
			break;
		case '3':
			aa = 2;
			WAVE_MenuCtrl(aa,-1);
			break;
		case '4':
			aa = 3;
			WAVE_MenuCtrl(aa,-1);
			break;
		case '5':
			aa = 4;
			WAVE_MenuCtrl(aa,-1);
			break;
		case 'a':
			bb = 0;
			WAVE_MenuCtrl(aa,bb);
			break;
		case 'b':
			bb = 1;
			WAVE_MenuCtrl(aa,bb);
			break;
		case 'c':
			bb = 2;
			WAVE_MenuCtrl(aa,bb);
			break;
		case 'd':
			bb = 3;
			WAVE_MenuCtrl(aa,bb);
			break;
		case 'e':
			bb = 4;
			WAVE_MenuCtrl(aa,bb);
			break;
		default :
			break;
		}
		readdata = 0xff;
	}
}


void task_wave(void *param)
{
	int i = 0,flag = 0,ret;
	uint32_t timeout = 10 * 1000,framecount;					 //无闪烁显示区域
	EventBits_t events;
	WAVE_STATE_T state = GetWaveState();
	
	TickType_t refreshParamAreaPreiod = 0;
	int averageCount = 0;
	GUI_RECT dispRect = {X_START_OFF-1,Y_START_OFF,WAVE_DISP_WIDE+X_START_OFF+1,WAVE_DISP_HIGH+Y_START_OFF+1};
	OscWaveParamT OscWaveParam = {
		.pDispRect = &dispRect,
		.int_call_back_handle = int_call_back_handle,
		.DrawWaveFunc = DrawDynWave,
	};
	
	const ChannelSelT *pChannelSel = GetChannelSel();
	TaskWaveInit((void*)&OscWaveParam);
	printf("TaskWaveInit success\n");
	SetFIFOTriggerDepth(GetTimeBaseFiFoPoints()>>1);//默认预触发深度

	refreshParamAreaPreiod = xTaskGetTickCount();
//#define GUI_WAVE_RECT_COLOR     	GUI_MAKE_COLOR(0xb0e6ff00)
//#define GUI_WAVE_RECT_BKCOLOR   	GUI_MAKE_COLOR(0x2D325000)
//#define GUI_WAVE_BKCOLOR   		GUI_MAKE_COLOR(0x256565600)
	GUI_SetBkColor(GUI_WAVE_BKCOLOR);
	GUI_Clear();
	#if 1
	for (;;)
	{
		if (false == GetIsAutoSetupState())//非AutoSetup模式下显示
		{
			ShowRealTimeWave();
		}
	wait:
		wave_gui_recv_msg(&events);
		if (events & TASK_WAVE_GUI_SUSPEND)
		{
			TaskWaveSuspend();
		}
		
		if (TIPS_DISP_CTRL & events)
		{
			/*TipsDispHandle();
			wave_gui_send_msg(WAVE_REFLASH);*/
		}
		
		if (WAVE_STATE_CTRL & events)//切换run/stop模式
		{
			TaskWaveRunOrStop();
			parma_send_msg(UPDATE_RUN_STOP_FLAG_FLAG);//发送更新run/stop标志
		}
		
		if (AUTO_SETUP_MODE & events)
		{
			if (TaskWaveAutoSetUp(events) == 0)
			{
				parma_send_msg(UPDATE_TRIGGER_METHOD_AREA|UPDATE_TRRIGGER_P
				
				OINT_AREA);
				events |= WAVE_UPDATE_PARAM;
			}
		}
		if (ZEARO_CORRECTION & events)
		{
			TaskWaveZearoCorrection();
		}
		if (VAL_CORRECTION_CTRL & events)//电压校正
		{
			//.....
		}
		if (PARAM_UPDATE_CTRL & events)//参数载入
		{
			TaskWaveParamUpdate();
		}
		if (SAMPLE_METHOD_CTRL & events)
		{
			TaskWaveSampleMethodCtrl(pChannelSel);
		}
		if (events & SAMPLE_DONE)//采样完成处理
		{
			if (GetIsAutoSetupState() == true)
				events |= AUTO_SETUP_MODE;//autosetup模式需要特殊处理
			ret = TaskWaveSampleData(events);
			
			//API_Dprintf("ret:%d\n",ret);
			if (ret == 0)
			{
				WaveHandle(SAMPLE_DONE);
				if (GetWaveState() == WAVE_STATE_SINGLE)//单次采样需发送STOP消息响应动作
				{
					wave_gui_send_msg(WAVE_STATE_CTRL);
					parma_send_msg(UPDATE_TRIGGER_POINT_POS_AREA);//
				}
				if (GetZearoCorrectionState() == true)
				{
					events |= WAVE_UPDATE_PARAM;//调整增益需要参数重新设置
					TaskWaveZearoCorrectionHandle();//开始零点矫正
				}
				if (GetIsAutoSetupState() == true)//进入自动设置状态，自动找寻合适幅值和频率
				{
					events = SoftAutoSetup(events);//软件自动设置
					if (GetIsAutoSetupState() == false)//不继续寻找最佳幅值
						parma_send_msg(UPDATE_BASIC_POINT_AREA|UPDATE_TRIGGER_METHOD_AREA);
				}
				if (GetIsAutoSetupState() == true || (xTaskGetTickCount() - refreshParamAreaPreiod >= (UPDATE_PARAM_DISP_AREA_PERIOD/portTICK_RATE_MS)))
				{
					for (int loop = 0; loop < pChannelSel->count; loop++)
					{
						int channel;
						channel = pChannelSel->channelIndexArr[loop];
						wave_points_calulate(channel,averageCount);						
					}
					if (GetIsAutoSetupState() == false)
					{
						if (++averageCount >= GetParamSampleNum())
						{
							averageCount = 0;
							refreshParamAreaPreiod = xTaskGetTickCount();
							parma_send_msg(UPDATE_PARAM_DISP_AREA);//更新参数
						}
					}
				}
			}
			else if (ret == -2)
			{
				goto wait;//等待硬件插值完成
			}
			//TRIGGER ENABLE
			//SPI_WR(0x80500001);
		}
		if (SET_WAVE_ROLL_MODE & events)
		{
			TaskWaveRollMode();
			parma_send_msg(UPDATE_RUN_STOP_FLAG_FLAG);//发送更新run/stop标志
		}
		if (SET_WAVE_AUTO_MODE & events)
		{
			TaskWaveAutoMode();
			parma_send_msg(UPDATE_RUN_STOP_FLAG_FLAG);//发送更新run/stop标志
		}
		if (SET_WAVE_NORMAL_MODE & events)
		{
			TaskWaveNormalMode();
			parma_send_msg(UPDATE_RUN_STOP_FLAG_FLAG);//发送更新run/stop标志
		}
		if (SET_WAVE_SINGLE_MODE & events)
		{
			TaskWaveSingleMode();
			parma_send_msg(UPDATE_RUN_STOP_FLAG_FLAG);//发送更新run/stop标志
		}
		if (WAVE_UPDATE_PARAM & events)//切换时基档位、电压档位、触发水平、触发深度、模式时、控制耦合方式 AC/DC采样唤醒调用
		{
			SetOscillParam(state,event_group,pChannelSel);
		}
		if (events & HARD_PARAM_LISTEN_CTRL)//控制开启关闭硬件测量参数功能
		{
			HardParamListen();
		}
		if (SEND_UPDATE_PARAM_DISP_AREA & events)
		{
			parma_send_msg(UPDATE_PARAM_DISP_AREA);
			if (events & WAVE_UPDATE_PARAM)
				goto wait;
		}
		#if 1
		if (events & WAVE_REFLASH)//刷新处理
		{
			/*UpdateTimeBase();
			ret = TaskWaveSampleData(events);
			if (ret != 0)
				goto wait;*/
			//WaveHandle(0);
			//exec_printf("toxwin start:%d end:%ld FromXWin start:%d end:%ld\n",ToXWin[0].Start,ToXWin[0].End,FromXWin[0].Start,FromXWin[0].End);
		}

		if (events & WAVE_ENLARGE)//波形放大处理
		{
			WaveHandle(WAVE_ENLARGE);
		}
		if (events & WAVE_LESSEN)//波形缩小处理
		{
			WaveHandle(WAVE_LESSEN);
		}
		if (events & WAVE_LEFT_SHIFT)//波形左移处理
		{
			WaveHandle(WAVE_LEFT_SHIFT);
		}
		if (events & WAVE_RIGHT_SHIFT)//波形右移处理
		{
			WaveHandle(WAVE_RIGHT_SHIFT);
		}
		#endif
		ui_test();
		//WAVE_FillRoundedRectColor(GUI_WAVE_BROWN_COLOR, 12, 258, 12+107, 258+26, 10);
		//WAVE_FillRoundedRectColor(GUI_WAVE_BROWN_COLOR, 119, 258, 119+107, 258+26, 10);
		//vTaskDelay(50/portTICK_RATE_MS);
	}
	#endif
}

void WaveAutoModeExecute(void)
{
	WaveAutoModeHandle();
	wave_gui_send_msg(WAVE_REFLASH);
	wave_gui_send_msg(SET_WAVE_AUTO_MODE|WAVE_UPDATE_PARAM);//切换状态
	parma_send_msg(UPDATE_PARAM_DISP_AREA|UPDATE_TRIGGER_METHOD_AREA|UPDATE_TRRIGGER_POINT_AREA);//更新参数显示区

}
void WaveNormalModeExecute(void)
{
	WaveNormalModeHandle();
	wave_gui_send_msg(WAVE_REFLASH);
	wave_gui_send_msg(SET_WAVE_NORMAL_MODE|WAVE_UPDATE_PARAM);//切换状态
	parma_send_msg(UPDATE_PARAM_DISP_AREA|UPDATE_TRIGGER_METHOD_AREA|UPDATE_TRRIGGER_POINT_AREA);//更新参数显示区
}

void WaveSingleModeExecute(void)
{
	WaveSingleModeHandle();
	wave_gui_send_msg(WAVE_REFLASH);
	wave_gui_send_msg(SET_WAVE_SINGLE_MODE|WAVE_UPDATE_PARAM);//切换状态
	parma_send_msg(UPDATE_PARAM_DISP_AREA|UPDATE_TRIGGER_METHOD_AREA|UPDATE_TRRIGGER_POINT_AREA);//更新参数显示区
}
void WaveRollModeExecute(void)
{
	WaveRollModeHandle();
	wave_gui_send_msg(WAVE_REFLASH);
	wave_gui_send_msg(SET_WAVE_ROLL_MODE|WAVE_UPDATE_PARAM);//切换状态
	parma_send_msg(UPDATE_PARAM_DISP_AREA|UPDATE_TRIGGER_METHOD_AREA|UPDATE_TRRIGGER_POINT_AREA);//更新参数显示区
}



static TipsDispT gTipsDispArry[] = {
	{ID_TIPS_INDEX_1NS,"1ns/div","1ns/div"},
	{ID_TIPS_INDEX_2NS,"2ns/div","2ns/div"},
	{ID_TIPS_INDEX_5NS,"5ns/div","5ns/div"},
	{ID_TIPS_INDEX_10NS,"10ns/div","10ns/div"},
	{ID_TIPS_INDEX_20NS,"20ns/div","20ns/div"},
	{ID_TIPS_INDEX_50NS,"50ns/div","50ns/div"},
	{ID_TIPS_INDEX_100NS,"100ns/div","100ns/div"},
	{ID_TIPS_INDEX_200NS,"200ns/div","200ns/div"},
	{ID_TIPS_INDEX_500NS,"500ns/div","500ns/div"},

	{ID_TIPS_INDEX_1US,"1us/div","1us/div"},
	{ID_TIPS_INDEX_2US,"2us/div","2us/div"},
	{ID_TIPS_INDEX_5US,"5us/div","5us/div"},
	{ID_TIPS_INDEX_10US,"10us/div","10us/div"},
	{ID_TIPS_INDEX_20US,"20us/div","20us/div"},
	{ID_TIPS_INDEX_50US,"50us/div","50us/div"},
	{ID_TIPS_INDEX_100US,"100us/div","100us/div"},
	{ID_TIPS_INDEX_200US,"200us/div","200us/div"},
	{ID_TIPS_INDEX_500US,"500us/div","500us/div"},

	{ID_TIPS_INDEX_1MS,"1ms/div","1ms/div"},
	{ID_TIPS_INDEX_2MS,"2ms/div","2ms/div"},
	{ID_TIPS_INDEX_5MS,"5ms/div","5ms/div"},
	{ID_TIPS_INDEX_10MS,"10ms/div","10ms/div"},
	{ID_TIPS_INDEX_20MS,"20ms/div","20ms/div"},
	{ID_TIPS_INDEX_50MS,"50ms/div","50ms/div"},
	{ID_TIPS_INDEX_100MS,"100ms/div","100ms/div"},
	{ID_TIPS_INDEX_200MS,"200ms/div","200ms/div"},
	{ID_TIPS_INDEX_500MS,"500ms/div","500ms/div"},

	{ID_TIPS_INDEX_1S,"1s/div","1s/div"},
	{ID_TIPS_INDEX_2S,"2s/div","2s/div"},
	{ID_TIPS_INDEX_5S,"5s/div","5s/div"},

	{ID_TIPS_INDEX_5MV,"5mv/div","5mv/div"},
	{ID_TIPS_INDEX_10MV,"10mv/div","10mv/div"},
	{ID_TIPS_INDEX_20MV,"20mv/div","20mv/div"},
	{ID_TIPS_INDEX_50MV,"50mv/div","50mv/div"},
	{ID_TIPS_INDEX_100MV,"100mv/div","100mv/div"},
	{ID_TIPS_INDEX_200MV,"200mv/div","200mv/div"},
	{ID_TIPS_INDEX_500MV,"500mv/div","500mv/div"},
	{ID_TIPS_INDEX_1V,"1v/div","1v/div"},
	{ID_TIPS_INDEX_2V,"2v/div","2v/div"},
	{ID_TIPS_INDEX_5V,"5v/div","5v/div"},
	{ID_TIPS_PROMPT, "Scroll mode cannot set trigger","滚动模式不能设置触发"},
	{ID_TIPS_INDEX_LIMIT,"Set to the limit","设置达到极限"},
};
void SetTipsDispIndex(ID_TIPS_INDEX index)
{
//	API_Dprintf("hear");
	gTipsDispIndex = index;
}
static const TipsDispT* GetCurrentTipsDisp(void)
{
	if (gTipsDispIndex >= ID_TIPS_INDEX_MAX)
		return NULL;
	return &gTipsDispArry[gTipsDispIndex];
}
static void TipsDispHandle(void)
{
	TipsDispT *pTipsDisp;
	int FontDistY;
	const char *tips;
	ID_TIPS_INDEX index = gTipsDispIndex;
	
	if (index < ID_TIPS_INDEX_MAX)
	{
		SwitchClose(SWITCH_OBJ_TIPS_DISP);
		//const GUI_FONT GUI_UNI_PTR* pTmpFont = GUI_GetFont();
		pTipsDisp = &gTipsDispArry[index];
		if (GetCurrentLanguage() == SOCSI_LANGUAGE_ENGLISH)
		{
			GUI_SetFont(&GUI_Font8x13_ASCII);
			tips = pTipsDisp->english;
		}
		else
		{
			tips = pTipsDisp->chinese;
		#if CONFIG_EMWIN
		#else
			GUI_SetFont(&GUI_FontHZ16x16);
		#endif
		}
		FontDistY = GUI_GetFontDistY();
		pTipsDisp->pos.x = X_START_OFF + (WAVE_DISP_WIDE>>1) - ((get_string_pix_length(tips)+1)>>1);
		pTipsDisp->pos.y = Y_START_OFF + (WAVE_DISP_HIGH>>1) - ((FontDistY+1)>>1);
		SwitchOpen(SWITCH_OBJ_TIPS_DISP);
		//GUI_SetFont(pTmpFont);
		//API_Dprintf("SWITCH_OBJ_TIPS_DISP:%d pTipsDisp:0x%x\n",GetSwitchState(SWITCH_OBJ_TIPS_DISP),pTipsDisp);
	}
	
}

void SetTriggerDepthHandle(int32_t val, int dispFlag)
{
	SetTriggerDepth(val);
	if (dispFlag)
	{
		SetTipsDispIndex(ID_TIPS_INDEX_LIMIT);
		wave_gui_send_msg(TIPS_DISP_CTRL);
	}
}

void WaveStopRunHandle(void)//波形run/stop快捷键处理
{
	if (GetWaveState() == WAVE_STATE_STOP)
	{
		SetOscConfig(OSC_CONFIG_MAX);
		wave_gui_send_msg(PARAM_UPDATE_CTRL);
		wave_gui_send_msg(WAVE_STATE_CTRL|WAVE_UPDATE_PARAM);//切换状态、设置FPAG参数重新进入stop前状态
	}
	else
	{
		wave_gui_send_msg(WAVE_STATE_CTRL);//切换状态
	}
}

void AutoSetupHandle(void)
{
	{
		if (GetWaveState() == WAVE_STATE_STOP)
		{
			wave_gui_send_msg(WAVE_STATE_CTRL);//切换状态、设置FPAG参数重新进入stop前状态
		}
		//wave_gui_send_msg(WAVE_UPDATE_PARAM);
		SetOscConfig(OSC_CONFIG_MAX);
		wave_gui_send_msg(AUTO_SETUP_MODE);
	}
}

void WaveGuiSuspend()
{
	wave_gui_send_msg(TASK_WAVE_GUI_SUSPEND);
}

void WaveGuiResume()
{
	SetOscConfig(OSC_CONFIG_MAX);
	wave_gui_send_msg(WAVE_UPDATE_PARAM);
}
static void DispMeasureParamArea(int channel,GUI_RECT *pRect,const uint8_t dispBuff[][24], int count)
{
	int i,x_offset = 4,y_offset = 4,FontDistY;
	if (dispBuff == NULL)
		return ;
	GUI_SetBkColor(GUI_BLACK);
	GUI_ClearRect(pRect->x0, pRect->y0, pRect->x1, pRect->y1);
	GUI_SetColor(GetWaveColorWithChannel(channel));
	FontDistY = GUI_GetFontDistY() + 8;
	for (i = 0; i < count; i++)
	{
		//API_Dprintf("i:%d ",i);
		//API_Dprintf("%s\n",dispBuff[i]);
		if (i == ((count+1)>>1))
			x_offset += 110;
		GUI_DispStringAt(dispBuff[i],pRect->x0+x_offset,pRect->y0 + (i%((count+1)>>1))*FontDistY);
	}
	pRect->y0 += (i>>1)*FontDistY;
}
static void DrawWave(int channel,int index,int Start, int End,int x0,int flag)
{
	int end = GetWaveDispPosDepth(channel),tmp,loop;
	wave_dis_t *pADCConvertedValue = GetADCConvertedValueBuff(channel,index);
	wave_dis_t *pADCVlaue2ScreenValue =  GetADCVlaue2ScreenValue(channel);
	tmp = End - Start;
	//API_Dprintf("start\n");        wr
	//API_Dprintf("GetAmpliAtten:%d GetAmpliGain:%d GetAmpliLastSelect:%d GetAmpliSelect:%d\n",GetAmpliAtten(),GetAmpliGain(),GetAmpliLastSelect(),GetAmpliLastSelect(),GetAmpliSelect());
	for (int i = 0; i < tmp; i++)
	{
		pADCVlaue2ScreenValue[i] = (pADCConvertedValue[i]*GetAmpliAtten(channel)*1.0*1000/ADC_PRECISION/GetAmpliGain(channel))/(GetAmpliLastSelect(channel)*1.0/(5*Y_SPACE)*GetAmpliSelect(channel)/GetAmpliLastSelect(channel)) + 0.5;
		pADCVlaue2ScreenValue[i] = GetBasicPointPos(channel) - pADCVlaue2ScreenValue[i];
		//API_Dprintf("%d:%d ",pADCConvertedValue[i],pADCVlaue2ScreenValue[i]);
	}
	//API_Dprintf("\n");
	if (tmp)
	{
	#if !CONFIG_EMWIN
		if (flag == 0)
			GUI_DrawGraphOnArea(pADCVlaue2ScreenValue,tmp,Start+x0,Y_START_OFF,WAVE_DISP_HIGH);
		else
	#endif
			GUI_DrawGraph(pADCVlaue2ScreenValue,tmp,Start+x0,Y_START_OFF);
	}
}
void DSO_DrawBakFrame(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,void *param)

{
	 const WaveCtrlDispT *pWaveCtrlDisp = (WaveCtrlDispT*)param;
	 int32_t TriggerDepthLine = GetTriggerDepth();
	 wave_dis_t TrackLine_TimeLeft,TrackLine_TimeRight;
	 int32_t TrackLine_Trigger = GetTriggerLineValue();
	 wave_dis_t TrackLine_VppMax,TrackLine_VppMin;
     uint16_t x;

     uint16_t y;
     GetY1Y2Value(&TrackLine_VppMax,&TrackLine_VppMin);
	 GetX1X2Value(&TrackLine_TimeLeft,&TrackLine_TimeRight);
    

     /* 填充背景 */
	
     GUI_SetBkColor(GUI_BLACK);

     GUI_ClearRect(x0, y0, x1, y1); 



     GUI_SetColor(GUI_WHITE);


     /* 绘制水平刻度点 */

     //for (y = 0; y < 9; y++)                                
	 for (y = 0; y <= Y_GRID; y++)
     {

         //for (x = 0; x < 61; x++)
		 for (x = 1; x <= X_GRID*5; x++)
         {
             GUI_DrawPoint(x0 + (x * X_SPACE), y0 + (y * 5 * Y_SPACE));
         }

     }
	
    


     //for (x = 0; x < 61; x++)
	 /*for (x = 0; x < X_GRID*5; x++)
     {

         GUI_DrawPoint(x0 + (x * X_SPACE), y1);

     }*/


     /* 绘制垂直刻度点 */

     for (x = 0; x <= X_GRID; x++)                               

     {

         for (y = 0; y <= Y_GRID*5; y++)

         {

              GUI_DrawPoint(x0 + (x * 5 * X_SPACE), y0 + (y * Y_SPACE));

         }

     }
 
	
     /*for (y = 0; y < Y_GRID*5; y++)

     {

         GUI_DrawPixel(x1, y0 + (y * Y_SPACE));

     }*/

    	
     /* 绘制最后脚上的那个点 */

     //GUI_DrawPixel(x1 - 1, y1 - 1);

    

     /* 绘制垂直中心刻度点 */

     for (y = 0; y < Y_GRID*5; y++)                               

     {   

         GUI_DrawPixel(x0 - 1 + ((x1-x0)>>1), y0 + (y * Y_SPACE));

         GUI_DrawPixel(x0 + 1 + ((x1-x0)>>1), y0 + (y * Y_SPACE));

     }

     

     GUI_DrawPixel(x0 - 1 + ((x1-x0+1)>>1), y1);

     GUI_DrawPixel(x0 + 1 + ((x1-x0+1)>>1), y1);

	if(

    

     /* 绘制水平中心刻度点 */

     for (x = 0; x < X_GRID*5; x++)                                    

     {   

         GUI_DrawPixel(x0 + (x * X_SPACE), y0 - 1 + ((y1-y0)>>1));

         GUI_DrawPixel(x0 + (x * X_SPACE), y0 + 1 + ((y1-y0)>>1));

     }

    

     GUI_DrawPixel(x1, y0 - 1 + ((y1-y0+1)>>1));

     GUI_DrawPixel(x1, y0 + 1 + ((y1-y0+1)>>1));
	int i,len;
	
	int channel,loop;
	if (GetGlowOnOff() == 0)//余晖关闭处理方式
	{
		int count;
		SAMPLE_METHOD_T tmpSampleMethod;
		for (loop = 0; loop < pWaveCtrlDisp->pChannelSel->count; loop++)
		{
			channel = pWaveCtrlDisp->pChannelSel->channelIndexArr[loop];
			GUI_SetColor(GetWaveColorWithChannel(channel));
			tmpSampleMethod = GetSampleMethod(channel);
			count = GetSampleCount(tmpSampleMethod);
			if (GetLastTimeBaseSelectIndex() <= 0x09)//时基小于0x9时峰值采样无效
				count = 1;
			for (int index = 0; index < count; index++)
			{
				DrawWave(channel,index,pWaveCtrlDisp->pToXWin[channel].Start,pWaveCtrlDisp->pToXWin[channel].End,pWaveCtrlDisp->pDispRect->x0+1,0);
			}
		}
	}

	GUI_SetColor(VOLTAGE_LINE_COLOR);
	if (GetSwitchState(SWITCH_OBJ_VOLTAGE) == true)
	{
		//GUI_SetColor(VOLTAGE_LINE_COLOR);
		GUI_SetFont(&GUI_Font8_1);

     	GUI_SetTextMode(GUI_TEXTMODE_TRANS);    
		GUI_DrawHLine(TrackLine_VppMax+y0,1+X_START_OFF,SAMPDEPTH-2+X_START_OFF);
		GUI_DispStringAt("y1",1+X_START_OFF,TrackLine_VppMax+2);
		GUI_DrawHLine(TrackLine_VppMin+y0 ,1+X_START_OFF,SAMPDEPTH-2+X_START_OFF);
		GUI_DispStringAt("y2",1+X_START_OFF,TrackLine_VppMin+2);
		//GUI_DispDecAt(TrackLine_VppMax,2+X_START_OFF,TrackLine_VppMax+6 ,3);
		//GUI_DispDecAt(TrackLine_VppMin,2+X_START_OFF,TrackLine_VppMin-8 ,3);	
	}
	if (GetSwitchState(SWITCH_OBJ_TRIGGER_LEVEL) == true)
	{
		TIMEOUT_SWITCH(SWITCH_OBJ_TRIGGER_LEVEL,TIMEOUT_CLOSE,TrackLine_Trigger);
		//GUI_SetColor(VOLTAGE_LINE_COLOR);
		GUI_SetFont(&GUI_Font8_1);

     	GUI_SetTextMode(GUI_TEXTMODE_TRANS);     
		GUI_DrawHLine(TrackLine_Trigger+y0 ,1+X_START_OFF,SAMPDEPTH-2+X_START_OFF); 
		GUI_DispStringAt("T",1+X_START_OFF,TrackLine_Trigger+2);
		//GUI_DispDecAt(TrackLine_VppMax,2+X_START_OFF,TrackLine_VppMax+6 ,3);
		//GUI_DispDecAt(TrackLine_VppMin,2+X_START_OFF,TrackLine_VppMin-8 ,3);
	}
	if (GetSwitchState(SWITCH_OBJ_TIME) == true)
	{
		GUI_BUTTON_BKCOLOR（&GUI_Font8_1);
		GUI_SetTextMode(GUI_BLACK);
		if(TrackLine_Timerleft
		//GUI_SetColor(VOLTAGE_LINE_COLOR);
		GUI_SetFont(&GUI_Font8_1);

    	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
		if (TrackLine_TimeLeft >= 0 && TrackLine_TimeLeft < WAVE_DISP_WIDE)//越界判断
		{
			GUI_DrawVLine(TrackLine_TimeLeft+x0 ,1+Y_START_OFF,WAVE_DISP_HIGH-2+Y_START_OFF);
			GUI_DispStringAt("x1",TrackLine_TimeLeft+x0+2,1+Y_START_OFF);
		}
		if (TrackLine_TimeRight >= 0 && TrackLine_TimeRight < WAVE_DISP_WIDE)//越界判断
		{
			GUI_DrawVLine(TrackLine_TimeRight+x0,1+Y_START_OFF,WAVE_DISP_HIGH-2+Y_START_OFF);
			GUI_DispStringAt("x2",TrackLine_TimeRight+x0+2,1+Y_START_OFF);
		}
		
	}
	if (GetSwitchState(SWITCH_OBJ_TRIGGER_DEPTH) == true)
	{
		TIMEOUT_SWITCH(SWITCH_OBJ_TRIGGER_DEPTH,TIMEOUT_CLOSE,TriggerDepthLine);
		if (TriggerDepthLine >= 0 && TriggerDepthLine < WAVE_DISP_WIDE)//越界判断
		{
			GUI_DrawVLine(TriggerDepthLine+x0,1+Y_START_OFF,WAVE_DISP_HIGH-2+Y_START_OFF);
		}
	}
	const GUI_POINT tTriggerPoints[] = 
	{
		{0, 0},{X_START_OFF-1,X_START_OFF-1},{(X_START_OFF-1)<<1,0},
	};
	GUI_FillPolygon(tTriggerPoints,sizeof(tTriggerPoints)/sizeof(tTriggerPoints[0]),x0+TriggerDepthLine-(X_START_OFF-1),y0);//画触发深度图标
	if (GetSwitchState(SWITCH_OBJ_TIPS_DISP) == true)
	{
		const TipsDispT*pTmpTipsDisp = GetCurrentTipsDisp();
		TIMEOUT_SWITCH(SWITCH_OBJ_TIPS_DISP,TIMEOUT_CLOSE,gTipsDispIndex);
		//API_Dprintf("pTmpTipsDisp:%x\n",pTmpTipsDisp);
		if (pTmpTipsDisp)
		{
			const char *tips;
			const GUI_FONT GUI_UNI_PTR* pTmpFont = GUI_GetFont();
			if (GetCurrentLanguage() == SOCSI_LANGUAGE_ENGLISH)
			{
				tips = pTmpTipsDisp->english;
				#if CONFIG_EMWIN
				#else
				GUI_SetFont(&GUI_Font8x13_ASCII);
				#endif
			}
			else
			{
				tips = pTmpTipsDisp->chinese;
				#if CONFIG_EMWIN
				#else
				GUI_SetFont(&GUI_FontHZ16x16);
				#endif
			}
			//API_Dprintf("tips:%s\n",tips);
			
			
			GUI_DispStringAt(tips,pTmpTipsDisp->pos.x,pTmpTipsDisp->pos.y);
			#if CONFIG_EMWIN
			#else
			GUI_SetFont(pTmpFont);
			#endif
		}
	}
	if (GetSwitchState(SWITCH_OBJ_VOLTAGE) == true || GetSwitchState(SWITCH_OBJ_TIME) == true)
	{
		void (*funcArry[4])(char*);
		if (GetSwitchState(SWITCH_OBJ_VOLTAGE) == true)
		{
			funcArry[0] = GetY1DisffAndDisp;
			funcArry[1] = GetY2DisffAndDisp;
			funcArry[2] = GetY1Y2DisffAndDisp;
			MeasureParamDispAear(GUI_BLACK,GUI_GREEN,x0+2,y1-2*(Y_SPACE*5)+2,2*X_SPACE*5-2+X_SPACE-2,2*(Y_SPACE*5)-4,funcArry,3);
		}
		else
		{
			funcArry[0] = GetX1DisffAndDisp;
			funcArry[1] = GetX2DisffAndDisp;
			funcArry[2] = GetX1X2DisffAndDisp;
			funcArry[3] = GetX1X2FreqAndDisp;
			MeasureParamDispAear(GUI_BLACK,GUI_GREEN,x0+2,y1-2*(Y_SPACE*5)+2-16,2*X_SPACE*5-2+X_SPACE-2,2*(Y_SPACE*5)-4+16,funcArry,4);
		}
	}
	int count = 0;
	if (GetSwitchState(SWITCH_OBJ_MEASURE_PARA_DISP) == true)
	{
		int maxSize;
		GUI_RECT ParamDispRect = {
			.x0 = 2.5*5*X_SPACE+X_START_OFF,
			.y0 = 5*Y_SPACE+Y_START_OFF,
			.x1 = (WAVE_DISP_WIDE>>1) + 2.5*5*X_SPACE+X_START_OFF,
			.y1 = WAVE_DISP_HIGH - 5*Y_SPACE,
		};
		uint8_t dispBuff[10][24];
		count = pWaveCtrlDisp->pChannelSel->count;
		//API_Dprintf("maxSize:%d\ count:%d\n",maxSize,count);
		for (loop = 0; loop < count; loop++)
		{
			channel = pWaveCtrlDisp->pChannelSel->channelIndexArr[loop];
			extern int CalcOscParam(int channel,uint8_t dispBuff[][24], int maxSize);
			maxSize = CalcOscParam(channel,dispBuff,10);
			DispMeasureParamArea(channel,&ParamDispRect,dispBuff,maxSize);
		}
	}
	if (GetSwitchState(SWITCH_OBJ_HARDWARE_MEASURE_PARA_DISP) == true)
	{
		char *Company[] = {"ns","us","ms", "s"};
		uint8_t **info = NULL,*pDispBuf;
		uint8_t dispBuf[32];	
		pDispBuf = dispBuf;
		info = &pDispBuf;
		int x_offset = x0+WAVE_DISP_WIDE-2*5*X_SPACE-2;
		int y_offset = y0+WAVE_DISP_HIGH-GUI_GetFontDistY()-2;
		count = pWaveCtrlDisp->pChannelSel->count;
		HardMeasureParamT tmpHardMeasureParam;
		for (loop = count-1; loop >= 0; loop--)
		{
			channel = pWaveCtrlDisp->pChannelSel->channelIndexArr[loop];
			GetHardMeasureParam(channel,&tmpHardMeasureParam);
			int i = 0;
			for (; i < 4; i++)
			{
				if (tmpHardMeasureParam.period < 1000)
					break;
				tmpHardMeasureParam.period /= 1000;
			}
			sprintf(dispBuf," P:%.3f%s",tmpHardMeasureParam.period,Company[i]);
			//API_Dprintf("dispBuf:%s\n",info[0]);
			MeasureParamDisp(GUI_BLACK,GetWaveColorWithChannel(channel),x_offset,y_offset,2*5*X_SPACE,GUI_GetFontDistY(),dispBuf);
			x_offset -= 2*5*X_SPACE;
		}
	}
}

void DrawDynWave(void*p)
{
	GUI_RECT *RectP = *((GUI_RECT**)p);
	DSO_DrawBakFrame(RectP->x0+1,RectP->y0,RectP->x1-1,RectP->y1,p);	
}
