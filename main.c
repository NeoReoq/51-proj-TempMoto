#include "reg52.h"			 //此文件中定义了单片机的一些特殊功能寄存器
#include "lcd.h"
#include "temp.h"
#include "i2c.h"
#include <string.h>
typedef unsigned int u16;	  //对数据类型进行声明定义
typedef unsigned char u8;

u8 Temp[5]={'0','0','0','0'};
u8 trigTemp[5]={'0','0','0','0'};
u8 tempnow[6]={'t','e','m','p',':'};
u8 setting[10]={'s','e','t',' ','t','e','m','p',':'};
unsigned char* trigTempPtr;
u8 isSetMode;
u8 keyState;
u8 ptrIndex;
sbit moto=P1^0;
sbit k1=P3^0;
sbit k2=P3^1;
sbit k3=P3^2;
sbit k4=P3^3;
sbit k7=P3^5;
sbit k8=P3^6;
sbit kReset=P1^0;
sbit motto=P1^2;
sbit beep=P1^5;
sfr WDTRST=0xA6;
void UsartInit()
{
	SCON=0X50;			//设置为工作方式1
	TMOD=0X20;			//设置计数器工作方式2
	PCON=0X80;			//波特率加倍
	TH1=0XF3;				//计数器初始值设置，注意波特率是4800的
	TL1=0XF3;						//打开总中断
	TR1=1;					//打开计数器
}
void delay(u16 i)
{
	while(i--);	
}
void Di(){
	u8 i=100;
	while(i--)
	{
		beep=~beep;
		delay(100);	
	}
}
unsigned char *Itoa(unsigned int ni,int dd){	   //整数转字符串
	char i=0,j=0,temp[16],outstr[16];
	unsigned int n,num=ni;
	while(num>=dd){
		n=num%dd;
  		if(n>9)
			temp[i]=n+0x37;
		else 
			temp[i]=n+0x30;
  		num=num/dd;
  		i++;
  	}
  	n=num;
	if(n>9)
		temp[i]=n+0x37;
	else
		temp[i]=n+0x30;
  	j=0;
  	for(;i>=0;i--){
		outstr[j]=temp[i];
		j++;
	}
  	outstr[j]=0;
  	return outstr;
}
int GetRealTemp(temp){
	float tp;
	if(temp<0){
		temp=temp-1;
		temp=~temp;
		tp=temp;
		temp=tp*0.0625*100+0.5;
	}
	if(temp>0){
		tp=temp;
		temp=tp*0.0625*100+0.5;
	}
	return temp;
}
void SetTrigTempButton(){
	if(k1==0)
	{
		delay(1000);
		if(k1==0)
		{
			Di();
			keyState=1;
			if(*trigTempPtr<'9')
				*trigTempPtr=*trigTempPtr+1;
			else if(*trigTempPtr=='9')
				*trigTempPtr='0';
		}
		while(!k1);
	}				  
	if(k2==0)
	{
		delay(1000);
		if(k2==0)
		{ 	
			Di();
			keyState=1;
			if(*trigTempPtr>'0')
				*trigTempPtr=*trigTempPtr-1;
			else if(*trigTempPtr=='0')
				*trigTempPtr='9';
		}
		while(!k2);
	}
	if(k3==0)
	{
		delay(1000);
		if(k3==0)
		{	
			Di();
			keyState=1;
			if(ptrIndex>0){
				trigTempPtr=trigTempPtr-1;
				ptrIndex=ptrIndex-1;
			}
		}
		while(!k3);
	}
	if(k4==0)
	{
		delay(1000);
		if(k4==0)
		{	
			Di();
			keyState=1;
			if(ptrIndex<3){
				trigTempPtr=trigTempPtr+1;
				ptrIndex=ptrIndex+1;
			}	
		}
		while(!k4);
	}
}
void TempDisplay(){
	u8 i;
	int temp=GetRealTemp(Ds18b20ReadTemp());
	strcpy(Temp,Itoa(temp,10));
	for(i=0;i<5;i++){
		LcdWriteData(tempnow[i]);
	}
	for(i=0;i<4;i++)
	{
		if(i==2)
			LcdWriteData('.');
		LcdWriteData(Temp[i]);
	}	
	for(i=0;i<4; i++)
	{
		if(i==2)
			SBUF='.';
	 	SBUF=Temp[i];//将接收到的数据放入到发送寄存器
		while (!TI);			         //等待发送数据完成
		TI = 0;
	}
}
void intoSetMode(){
	u8 i;
	delay(5000);
	trigTemp[0]=At24c02Read(1);
	delay(5000);
	trigTemp[1]=At24c02Read(2);
	delay(5000);
	trigTemp[2]=At24c02Read(3);
	delay(5000);
	trigTemp[3]=At24c02Read(4);
	delay(5000);
	LcdInit();
	while(isSetMode){
		if(keyState){
			LcdInit();
			keyState=0;
			for(i=0;i<9;i++){
				LcdWriteData(setting[i]);
			}
			for(i=0;i<4;i++){
				if(i==2)
					LcdWriteData('.');
				LcdWriteData(trigTemp[i]);
			}
		}
		SetTrigTempButton();
		if(k8==0)
		{
			isSetMode=0;
			Di();
			At24c02Write(1,trigTemp[0]);
			delay(5000);
			At24c02Write(2,trigTemp[1]);
			delay(5000);
			At24c02Write(3,trigTemp[2]);
			delay(5000);
			At24c02Write(4,trigTemp[3]);
			delay(5000);
			At24c02Write(5,'n');
			LcdInit();
		}
	}
	TempDisplay();
}
void TrigMoto(){
	if(strcmp(trigTemp,Temp)<0){
		motto=1;
	} else{
		motto=0;
	}
}
void SetTrigTempInit(){
	isSetMode = 0; //未进入设置模式
	delay(5000);
	if(At24c02Read(10)!='y'){	//设置下次进入不会初始化trigTemp
		trigTemp[0]='3'; //默认设置触发温度为30度
		trigTemp[1]='3';
		trigTemp[2]='4';
		trigTemp[3]='0';
		delay(5000);
		At24c02Write(1,trigTemp[0]);
		delay(5000);
		At24c02Write(2,trigTemp[1]);
		delay(5000);
		At24c02Write(3,trigTemp[2]);
		delay(5000);
		At24c02Write(4,trigTemp[3]);
		delay(5000);
		At24c02Write(10,'y');
		delay(5000);
	}else{
		delay(5000);
		trigTemp[0]=At24c02Read(1);
		delay(5000);
		trigTemp[1]=At24c02Read(2);
		delay(5000);
		trigTemp[2]=At24c02Read(3);
		delay(5000);
		trigTemp[3]=At24c02Read(4);
		delay(5000);
	}
}

void SetTrigTemp(){
	if(k7==0){
		delay(1000);
		if(k7==0){
			isSetMode=1;
			trigTempPtr = trigTemp;
			ptrIndex=0;
			Di();
			keyState=1;
			intoSetMode();
		}
		while(!k7);
	}
	if(kReset==0){
		delay(1000);
		if(kReset==0){
			Di();
			trigTemp[0]='3';
			trigTemp[1]='3';
			trigTemp[2]='4';
			trigTemp[3]='0';
			At24c02Write(1,trigTemp[0]);
			delay(5000);
			At24c02Write(2,trigTemp[1]);
			delay(5000);
			At24c02Write(3,trigTemp[2]);
			delay(5000);
			At24c02Write(4,trigTemp[3]);
			delay(5000);
			At24c02Write(10,'n');
			delay(5000);
			keyState=1;
		}
		while(!kReset);
	}
}

void main(void)
{
	u8 count=0;
	motto=0;
	WDTRST=0x1E;;//初始化看门狗
	WDTRST=0xE1;//初始化看门
	LcdInit();
	SetTrigTempInit();
	UsartInit();
	while(1){
		TempDisplay();
		SetTrigTemp();
		delay(5000);
		if(count>5)
			TrigMoto();
		WDTRST=0x1E;;//喂狗指令
		WDTRST=0xE1;//喂狗指令
		LcdInit();
		count=count+1;
	}				
}
