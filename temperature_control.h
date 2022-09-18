#include <reg52.h>

#define uchar unsigned char
#define uint unsigned int

/*---------------------Utility Functions------------------------*/
void delay_ms(uchar);
void delay_us(uchar);


/*------------------Temperature Collection----------------------*/
bit init_DS18B20();
uchar read_DS18B20();
void  write_DS18B20(uchar);

void  convert_temp();
uint  read_temp();
void  get_temp();


/*--------------------Temperature Display----------------------*/
void display_pos(uchar, uchar);
void display();


/*------------Temperature Control and Motor Dirve---------------*/
void control_temp();


/*-------------------------Interrupt---------------------------*/
void init_T0();
void init_T1();