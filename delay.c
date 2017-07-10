
#include <p18f4553.h>
#include <stdio.h>
#include "delay.h"

////
//funcion de retraso en ms
void delay_ms(short int ms)
{
	while(ms--)
	{
		long x = 705;
		while(x) x--;
	}
}

////
//funcion de retraso en decenas de us
void delay_10us(short int us)
{
	while(us--)
	{
		char x = 10;
		while(x) x--;
	}
}
