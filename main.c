#include <reg52.h>
#include <intrins.h>
#include "temperature_control.h"

#define uchar unsigned char
#define uint unsigned int

// storage data to be displayed
uchar dat1, dat2, dat3, dat4;

// DQ port of temperature sensor
sbit  DQ = P3^0;

// storage temperature from sensor
uint  temp;
uint  upper, lower;
uchar temp_LSB;
uchar temp_MSB;

uint loop_cnt;

// input and enable ports of motor drive
sbit  IN1 = P1^7;
sbit  IN2 = P1^6;
sbit  EN1 = P1^5;

// number 0 ~ 9 , '-', 'L', 'H', 'C'
uchar code num[14] = { 0xc0, 0xf9, 0xa4,0xb0,0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0xbf, 0xc7, 0x89, 0xc6};


/*---------------------Utility Functions------------------------*/

// delay z ms
void delay_ms(uchar z)
{
	uchar x, y;
	for(x = z; x > 0; x--)
		for(y = 1000; y > 0; y--);
}

// delay z us
void delay_us(uchar z)
{
	for(; z > 0; z--);
}

/*
 * 1) execute delay_10us function need a "lcall" instruction, it run for 2 us.
 * 2) call NOP function 6 times to delay more 6 us.
 * 3) execute "RET" instruction  for 2 us.
 */
void delay_10us()
{
	_nop_( );
	_nop_( );
	_nop_( );
	_nop_( );
	_nop_( );
	_nop_( );
}

/*------------------Temperature Collection----------------------*/
/*
 * The steps of initialize temperature sensor (DS18B20):
 * 1) pull DQ to low.
 * 2) delay 480 us.
 * 3) pull DQ to high.
 * 4) delay and wait 80us. (if successed, sensor would feedback a low pulse from DQ.)
 * 5) if successed, delay more than 480 ms.
 */
bit init_DS18B20(void)
{
	bit init_flag = 0;

	DQ = 1;
	delay_us(8);
	DQ = 0;
	delay_us(80); 
	DQ = 1;
	delay_us(5); 
	init_flag = DQ;
	delay_us(20);
  
	return init_flag;
}

/*
 * The steps of reading byte from temperature sensor (DS18B20):
 * 1) pull DQ to low.
 * 2) delay 1 us.
 * 3) pull DQ to high, release bus to read data.
 * 4) delay 10 us.
 * 5) read the data bus to get a single bit.
 * 6) dalay 50 us.
 * 7) repeat step 1 ~ 6 to get the whole byte.
 */
uchar read_DS18B20(void)
{
	uchar i;
	uchar dat;
  
	for(i = 0; i < 8; i++)
	{
		DQ = 1;
		delay_us(1);
		DQ = 0;
		dat >>= 1;
		DQ = 1;
		if(DQ)
		{
			dat |= 0x80;
		}
		delay_us(4);
	}
	return dat;
}


/*
 * The steps of writing byte to temperature sensor (DS18B20):
 * 1) pull DQ to low.
 * 2) delay 20 us.
 * 3) write byte from 0 bit to 7 bit.
 * 4) delay 60 us.
 * 5) pull DQ to high.
 * 6) repeat step 1 ~ 5 to write the whole byte.
 * 7) delay more 40 us at last.
 */
void write_DS18B20(uchar dat)
{
	uchar i;
	for(i = 8; i > 0; i--)
	{
		DQ = 0;
		DQ = dat & 0x01;
		delay_us(5);
		DQ = 1;
		dat >>= 1;
	}

	delay_us(4);
}

/*
 * The steps of reading collected temperature:
 * 1) initialize temperature sensor.
 * 2) write in 0xcc to skip ROM command.
 * 3) write in 0x44 to issues convert temperature command.
 * 4) delay 1 ms ensure convert successfully.
 * 
 * 5) initialize temperature sensor again.
 * 6) write in 0xcc to skip ROM command.
 * 7) write in 0xbe to issues read scratchpad command.
 * 8) read the collected temperature in sequence.
 */
/* temperature convert */
void convert_temp()
{
	init_DS18B20();
	write_DS18B20(0xcc);
	write_DS18B20(0x44);
}

/* temperature read */
uint read_temp()
{
	uint temp_current;

	init_DS18B20();
	write_DS18B20(0xcc);
	write_DS18B20(0xbe);
	temp_LSB = read_DS18B20();
	temp_MSB = read_DS18B20();
	temp_current = (((temp_MSB << 8) | temp_LSB) >> 4) & 0x0f;  // & 0x0f to get absolute value
	return temp_current;
}

/* convert need 750ms, then read the temperatrue */
void get_temp()
{
	loop_cnt = 0;
	convert_temp();
	init_T1();
	while (loop_cnt > 750);
	temp = read_temp();
}


/*--------------------Temperature Display----------------------*/

/*
 * note:  used common anode digital tube
 * 1) pull the position selection port (pos1, pos2...) high.
 * 2) write the display data (dat1, dat2...) into segment selection port (P0)
 */
void display_pos(uchar p2, uchar dat)
{
	switch (p2)
	{
		case 1:
			P2 = 0x01;
			P0 = num[dat];
			break;
		case 2:
			P2 = 0x02;
			P0 = num[dat];
			break;
		case 3:
			P2 = 0x04;
			P0 = num[dat];
			break;
		case 4:
			P2 = 0x08;
			P0 = num[dat];
			break;
		default:
			break;
	}
}

void display()
{
	display_pos(4, dat4);
	delay_ms(5);

	display_pos(3, dat3);
	delay_ms(5);

	display_pos(2, dat2);
	delay_ms(5);

	display_pos(1, dat1);
	delay_ms(5);
}


/*------------Temperature Control and Motor Dirve---------------*/
/* 
 * 
 * 
 */
void control_temp()
{
	// dat1 = 13;
	dat2 = 10;
	dat3 = temp / 10;
	dat4 = temp % 10;
	display();

	if (temp < lower)
	{
		TR0  = 0;
		dat1 = 11;
		EN1  = 0;
	}
	else if (temp > upper)
	{
		TR0  = 0;
		dat1 = 12;
		IN1  = 0;
		IN2  = 1;
		EN1  = 1;
	}
	else
	{
		dat1 = 13;
		init_T0();
		IN1  = 0;
		EN1  = 1;
	}
}


/*-----------------------Main Functions-------------------------*/
void main()
{
	uchar j;
	lower = 20, upper = 25;

	while(1)
	{
		get_temp();
		for (j = 100; j >0 ; j--) control_temp();
	}
}


/*-------------------------Interrupt---------------------------*/
// Timer 0 
void T0_motor() interrupt 1												
{
	TH0 = (65536 - 10000) / 256;
	TL0 = (65536 - 10000) % 256;
	IN2 = ~IN2;
}

// Timer 1
void T1_temp_convert() interrupt 3
{
	TH0  = (65536 - 1000) / 256;
	TL0  = (65536 - 1000) % 256;
	loop_cnt++;
}

// Delay 100ms. Generating 50% duty cycle PWM square wave for motor speed control.
void init_T0()  
{
	TMOD = 0X01;
	TH0  = (65536 - 10000) / 256;       
	TL0  = (65536 - 10000) % 256;
	ET0  = 1;
	EA   = 1;
	TR0  = 1;
}

// Delay 1ms. Temperature convert need more than 750ms.  
void init_T1()
{
	TMOD = 0X10;
	TH0  = (65536 - 1000) / 256;
	TL0  = (65536 - 1000) % 256;
	ET1  = 1;
	EA   = 1;
	TR1  = 1;
}
