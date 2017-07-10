
#ifndef _TECLADO_H
#define _TECLADO_H

////
//Espera a que se dejen de presionar las teclas
void KB_Wait(void);

////
//Comprueba si hay tecla presionada
unsigned char KB_Hit(void);
//retorna 1 si hay tecla presionada
//retorna 0 si no hay tecla presionada
//no es funcion bloqueante

////
//Retorna el codigo de la tecla
unsigned char KB_Read(void);
//retorna el código de 0x00 al 0x0F de la tecla presionada
//retorna 0xFF si no hay tecla presionada
//no es funcion bloqueante

////
//Retorna el codigo de la tecla
unsigned char KB_Get(void);
//retorna el código de 0x00 al 0x0F de la tecla presionada
//no retorna hasta que se presiona una tecla
//si es funcion bloqueante
#endif