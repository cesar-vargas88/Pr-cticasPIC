
#include <p18f4553.h>
#include <stdio.h>
#include "teclado.h"
#include "delay.h"

////
//Espera a que se dejen de presionar las teclas
void KB_Wait(void)
{
	while(KB_Hit());
}

////
//Comprueba si hay tecla presionada
unsigned char KB_Hit(void)
//retorna 1 si hay tecla presionada
//retorna 0 si no hay tecla presionada
//no es funcion bloqueante
{
	if(KB_Read() != 0xFF)
		return 0xFF;

	return 0x00;
}

////
//Retorna el codigo de la tecla
unsigned char KB_Read(void)
//retorna ell código de 0x00 al 0x0F de la tecla presionada
//retorna 0xFF si no hay tecla presionada
//no es funcion bloqueante
{
//----------------------------------------//
//             primera fila               //
//----------------------------------------//
	PORTD = (PORTD & 0xF0) | 0x01;
	delay_10us(10);

	//primera columna
	if(PORTDbits.RD4)
		return 0x00;

	//segunda columna
	if(PORTDbits.RD5)
		return 0x01;

	//tercera columna
	if(PORTDbits.RD6)
		return 0x02;

	//cuarta columna
	if(PORTDbits.RD7)
		return 0x03;

//----------------------------------------//
//             segunda fila               //
//----------------------------------------//
	PORTD = (PORTD & 0xF0) | 0x02;
	delay_10us(10);

	//primera columna
	if(PORTDbits.RD4)
		return 0x04;

	//segunda columna
	if(PORTDbits.RD5)
		return 0x05;

	//tercera columna
	if(PORTDbits.RD6)
		return 0x06;

	//cuarta columna
	if(PORTDbits.RD7)
		return 0x07;

//----------------------------------------//
//              tercera fila              //
//----------------------------------------//
	PORTD = (PORTD & 0xF0) | 0x04;
	delay_10us(10);

	//primera columna
	if(PORTDbits.RD4)
		return 0x08;

	//segunda columna
	if(PORTDbits.RD5)
		return 0x09;

	//tercera columna
	if(PORTDbits.RD6)
		return 0x0A;

	//cuarta columna
	if(PORTDbits.RD7)
		return 0x0B;

//----------------------------------------//
//              cuarta fila               //
//----------------------------------------//
	PORTD = (PORTD & 0xF0) | 0x08;
	delay_10us(10);

	//primera columna
	if(PORTDbits.RD4)
		return 0x0C;

	//segunda columna
	if(PORTDbits.RD5)
		return 0x0D;

	//tercera columna
	if(PORTDbits.RD6)
		return 0x0E;

	//cuarta columna
	if(PORTDbits.RD7)
		return 0x0F;
	return 0xFF;
}

////
//Retorna el codigo de la tecla
unsigned char KB_Get(void)
//retorna ell código de 0x00 al 0x0F de la tecla presionada
//no retorna hasta que se presiona una tecla
//si es funcion bloqueante
{
	while(!KB_Hit());

	return KB_Read();
}
