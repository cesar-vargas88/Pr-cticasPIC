#include <p18f4553.h>
#include <stdio.h>
#include <stdlib.h> 
#include "interface.h"
#include "command.h"
#include "AtWareGraphics.h"
#include "teclado.h"

//////////////////////////////////
//			Graficos			//
//////////////////////////////////

/*
Esta función es invocada por todas las funciones de salida estándar <stdio.h>
*/
int _user_putc(char c)
{
	//Realizar la escritura del caracter proveniente de STDOUT mediante la biblioteca gráfica.
	//El cursor de caracteres GLCD_Column y GLCD_Line puede cambiarse en cualquier momento a manera de gotoxy antes de escribir
	return ATGConsolePutChar(c);
}

/*
 ----------- GRAPHICS LCD HARDWARE ABSTRACTION LAYER --------------------
 By Cornejo.
 
 Este es un ejemplo de HAL funcionando en mi configuración local. MPU 48MHz, GLCD, a máxima velocidad estable.
 
 Indico pues, será necesario ver los sincrogramas y los tiempos necesarios que deberán cumplirse para 
 que la comunicación sea estable. Factores como la longitud de los cables y las capacitancias parásitas
 pueden aumentar los requerimientos de temporización. Por favor, experimenten. Traten con 6 nops, y vayan reduciendo
 hasta lograr un mínimo estable. Esto debido a que los chips son ensamblados por diferentes fabricantes de módulos.
 
 No usar ESS_DelayMS dentro de la implementación de una HAL un milisegundo es una eternidad.... Las operaciones gráficas son de alta frecuencia y 
 computacionalmente costosos. A menos que quieran ver como se escribe pixel por pixel.
 
*/

//BUS DE CONTROL (PINOUT DEFINITION)
#define GLCD_CS1 PORTBbits.RB7   
#define GLCD_CS2 PORTBbits.RB6
#define GLCD_DI  PORTBbits.RB5
#define GLCD_RW  PORTBbits.RB4
#define GLCD_E	 PORTBbits.RB3
#define GLCD_TRIS_CS1 TRISBbits.TRISB7
#define GLCD_TRIS_CS2 TRISBbits.TRISB6
#define GLCD_TRIS_DI  TRISBbits.TRISB5
#define GLCD_TRIS_RW  TRISBbits.TRISB4
#define GLCD_TRIS_E	  TRISBbits.TRISB3
#define GLCD_TRIS_OUTPUT_MASK 0x07      //Bits 7:3 del puerto B como salidas
//BUS DE DATOS (PINOUT DEFINITION)
#define GLCD_TRIS_DATA TRISD
#define GLCD_PORT_DATA PORTD


//HAL de lectura de datos del GLCD
void User_ATGReadData(void)
{
	do {User_ATGReadStatus();} while(g_ucGLCDDataIn&GLCD_STATUSREAD_BUSY);
	GLCD_RW=1;
	GLCD_DI=1;
	GLCD_E=1;
	Nop();
	GLCD_E=0;
	do {User_ATGReadStatus();} while(g_ucGLCDDataIn&GLCD_STATUSREAD_BUSY);
	GLCD_RW=1;
	GLCD_DI=1;
	GLCD_E=1;
	Nop();
	Nop();
	g_ucGLCDDataIn=GLCD_PORT_DATA;
	GLCD_E=0;
	GLCD_CS1=1;
	GLCD_CS2=1;
}
//HAL  de escritura de datos al GLCD
void  User_ATGWriteData(void)
{
	do {User_ATGReadStatus();} while(g_ucGLCDDataIn&GLCD_STATUSREAD_BUSY);
	GLCD_PORT_DATA=g_ucGLCDDataOut;
	GLCD_TRIS_DATA=0x00;
	GLCD_RW=0;
	GLCD_DI=1;
	GLCD_E=1;
	Nop();
	GLCD_E=0;
	GLCD_CS1=1;
	GLCD_CS2=1;
}
//HAL de escritura de comandos al GLCD
void  User_ATGWriteCommand(void)
{
	do {User_ATGReadStatus();} while(g_ucGLCDDataIn&GLCD_STATUSREAD_BUSY);
	GLCD_PORT_DATA=g_ucGLCDDataOut;
	GLCD_TRIS_DATA=0x00;
	GLCD_RW=0;
	GLCD_DI=0;
	GLCD_E=1;
	Nop();
	GLCD_E=0;
	GLCD_CS1=1;
	GLCD_CS2=1;
}
//HAL de lectura del bit de BUSY del GLCD(g_ulGLCDDevChild)
void User_ATGReadStatus(void)
{
	GLCD_TRIS_DATA=0xff;
	if(g_ucGLCDDevChild)
		GLCD_CS1=0;
	else 
		GLCD_CS2=0;
	GLCD_RW=1;
	GLCD_DI=0;
	GLCD_E=1;
	Nop();
	Nop();
	g_ucGLCDDataIn=GLCD_PORT_DATA;
	GLCD_E=0;
}

//Ejemplo de un Sprite definido por el usuario almacenado en rom.
rom unsigned char Sprite[8]=
{
	0x0F,
	0x0F,
	0x0F,
	0x0F,
	0xF0,
	0xF0,
	0xF0,
	0xF0
};
//La lista de vertices almacenada en RAM
struct POINT Triangle[]=
{
	{2,  15},
	{8,  50},
	{-30,50},
	{2,  15}
};




//////////////////////////////////////////////
//			Control Motor 3DScan			//
//////////////////////////////////////////////

int g_nDirection = 0;	
int g_nEnable	 = 0;	
int g_nStep 	 = 1;
int g_nDelay	 = 20;
	
void StepperMotor(int Step)
{
	ADCON1	=	0xff;
	PORTA	=   0x00;
	TRISA	=	0x00;
		
	switch(Step)
	{
		case 1:
			PORTA = 0x11;
			break;
		case 2:
			PORTA = 0x09;
			break;
		case 3:
			PORTA = 0x05;
			break;
		case 4:
			PORTA = 0x03;
			break;
	}
}

//////////////////////////////////////////
//			Comunicación USB			//
//////////////////////////////////////////

void HighPriorityISR(void);
void LowPriorityISR(void);
#pragma code HIGH_INTERRUPT_VECTOR=0x2008
void HighInterruptHook(void)
{
	_asm goto HighPriorityISR _endasm 
} 
#pragma code LOW_INTERRUPT_VECTOR=0x2018
void LowInterruptHook(void)
{
	_asm goto LowPriorityISR _endasm
}

#pragma code
#pragma interrupt HighPriorityISR
void HighPriorityISR(void)
{

}
#pragma interruptlow LowPriorityISR
void LowPriorityISR(void)
{
}

/*
	Función de relevo del manipilador de comandos.
	
	USBOverlayCommandHandler
	
	Esta función es invocada por @Ware ESS cuando se recibe un comando no conocido por @Ware ESS
	con la estructura PICCOMMAND desde el host. Es decir, se trata de un comando de usuario.
	
	El usuario define nuevos códigos de comando según sus necesidades y es en 
	ésta sección de código donde se implementan las acciones para esos nuevos 
	códigos de comandos.
	
	Véase el archivo "command.h" para conocer el conjunto de códigos de comandos 
	ya implementados por @Ware ESS. Esto le ayudará a definir nuevos y distintos códigos de comando.
	
	La función retorna:
	 	1: Si el comando es desconocido incluso por la aplicación.
	 	0: Si el comando es procesado y reconocido por la aplicación.
	 	
	 
	Se retorna al HOST el comando:
	 	ERROR_PIC_UNIMPLEMENTED_COMMAND: Si el comando no es reconocido
	 	(Lo anterior se hace automáticamente retornando simplemente 1.)
	 	
	 	ERROR_PIC_SUCCESS: Si el comando ha sido procesado exitosamente.
	 	(Se debe indicar este código de error explícitamente antes de retornar 0.)
	 	
	Si la respuesta es exitosa: ERROR_PIC_SUCCESS
		Entonces, el PIC puede retornar hasta 16 bytes con información
		adicional de respuesta. Estos resultados se cargan en s_pPicCmd
		
		Al formar una respuesta válida siga estos pasos:

			1) Cargar el codigo de error de exito
				s_pPicCmd->m_bCommandCode=ERROR_PIC_SUCCESS
			2) Cargar la longitud de la respuesta en bytes (0 hasta 16)
				s_pPicCmd->m_bParamsBufferSize=<tamaño respuesta>
			3) Cargar la información de la respuesta (si la hay)
				s_pPicCmd->m_abParamsBuffer[i]=<Dato i-ésimo>
			4) Retornar 0 
				
	PRECAUCIÓN:
	¡LA APLICACIÓN NO DEBE INVOCAR NUNCA ESTA FUNCIÓN! 
	
	Sólo @Ware ESS invoca esta función cuando arriben conmandos 
	provenientes del host. 
	(Este tipo de funciones se les conoce como "callback", ya un sistema operativo
	es el único responsable de invocarlas)
	
	
	Sírvase como muestra.
*/ 
char USBOverlayCommandHandler(struct TRANSCEIVER_STATE_MACHINE *pM)
{
	//Puntero a la tabla virtual en rom del transmisor/receptor USB (véase "command.h")
	static rom struct TRANSCEIVER_VIRTUAL_CONST_TABLE* s_vtbl;
	//Puntero al comando recibido
	static struct PICCOMMAND* s_pPicCmd;
	
	//Extraer el puntero a la tabla virtual
	s_vtbl=pM->m_vtbl;
	//Extraer el puntero al comando
	s_pPicCmd=s_vtbl->m_pPicCmd;
	
	//Decodificar el comando
	switch(s_pPicCmd->m_bCommandCode)
	{
		//Procesar comandos definidos por el usuario.
		
		/*
			Supóngase que existe un comando con código 0x33 definido por usted,
			en alguna parte de su proyecto así:
			
			#define COMMAND_PIC_MYCOMMAND 0x33
			
			También supóngase que se necesita retorna un valor entero de 32 bits como respuesta
			cuyo valor se encuentra en una variable declarada como "long lRespuesta" en alguna parte.
			
			Luego, impleméntese dentro de ésta selección múltiple (switch) siguiendo el procedimiento
			ya descrito el código para notificar la respuesta al host:
			
			case COMMAND_PIC_MYCOMMAND:
				s_pPicCmd->m_bCommandCode=ERROR_PIC_SUCCESS;    		//Paso 1: código de suceso
				s_pPicCmd->m_bParamsBufferSize=sizeof(long);			//Paso 2: sizeof(long) es 4 bytes
				*((long*)(s_pPicCmd->m_abParamsBuffer))=lRespuesta; 	//Paso 3: copia los 4 bytes de la respuesta
				return 0;												//Paso 4: retornar 0 y enviar la respuesta al host
		*/
		case  COMMAND_SEND_STRING:
		{
			int x = 0;
			while(s_pPicCmd->m_abParamsBuffer[x] != '\n' && x <16)
			{
				_user_putc(s_pPicCmd->m_abParamsBuffer[x]);
				x++;
			}
			    s_pPicCmd->m_bCommandCode=ERROR_PIC_SUCCESS;
			    s_pPicCmd->m_bParamsBufferSize=sizeof(char);
			    *((char*)s_pPicCmd->m_abParamsBuffer)=0x00000001;
			    return 0; 
		}
		case  COMMAND_READ_PI:
		{
			switch(g_nDirection)
			{
				case 0:
				{
					if(g_nStep == 4)
						g_nStep = 1;
					else
						g_nStep++;
					break;
				}	
				case 1:
				{
					if(g_nStep == 1)
						g_nStep = 4;
					else
						g_nStep--;
					break;
				}	
			}	
			    
			 s_pPicCmd->m_bCommandCode=ERROR_PIC_SUCCESS;
			 return 0; 
		}
	}	
	return 1; //Comando no manipulado o no reconocido. @Ware ESS generará la respuesta ERROR_PIC_UNIMPLEMETED_COMMAND
}	

//////////////////////////////
//			Teclado			//
//////////////////////////////

unsigned char ucTecla=0x00;
rom unsigned char ucTeclaPresionada[16]={	'1','2','3','A',
								'4','5','6','B',
								'7','8','9','C',
								'*','0','#','D'};

void Teclado()
{
	PORTD=0x00;
	TRISD=0xf0;
		
	if(KB_Hit() == 0xff)
	{
		ucTecla = KB_Get();
		PORTD=0x00;
		TRISD=0x00;
		_user_putc(ucTeclaPresionada[ucTecla]);
		ESS_Delay(400);
	}	
}

//////////////////////////////////
//			Calculadora			//
//////////////////////////////////


unsigned char g_cNumero[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};			//Arreglo para digitos del numero ingresado
unsigned char g_cIndiceNumero = 0x00;	
long g_lPilaNumeros[8] = {0,0,0,0,0,0,0,0};
unsigned char g_cIndicePilaNumeros = 0x00;
long long g_llResultado = 0;
char g_cDivisionCero = 0;
void DibujarPantallaCalculadora()
{
	int i = 0;
	unsigned char 	cCantidadDigitosEnNumero = g_cIndiceNumero;	
	unsigned char 	cCantidadNumerosEnPila = g_cIndicePilaNumeros;

	PORTD=0x00;
	TRISD=0x00;
	
	//Limpiar el GLCD con el color de brocha
	ATGClear();							
												
	g_Column = 0;
	g_Line	 = 3;
	

	
	while(cCantidadDigitosEnNumero)
	{
		_user_putc(g_cNumero[i]);
		i++;
		cCantidadDigitosEnNumero--;
	}
	
	
	
	g_Line	 = 0;
	i = 0;
	while(cCantidadNumerosEnPila)
	{
		g_Column = 10;
		printf("%ld",g_lPilaNumeros[i]);
		g_Line++;		
		i++;
		cCantidadNumerosEnPila--;
	}	
}
long  RegresaNumero(void)
{
	long lResultado = 0;
	long mul = 1;

	while(g_cIndiceNumero)
	{
		if(mul == 0)
		{
			lResultado += (((long)(g_cNumero[g_cIndiceNumero-1]))-48);
			g_cIndiceNumero--;
			mul=1;
		}
		else
		{
			if(g_cIndiceNumero == 1 && lResultado > 147483647 && g_cNumero[0] > 1)
			{
				//Limpiar el GLCD con el color de brocha
				ATGClear();							
																				
				g_Column = 0;
				g_Line	 = 2;
				g_cIndiceNumero--;
				printf("El numero no debe ser mayor a 2147483647.");
				ESS_Delay(1000);
				return 0;
			}
			else
			{
				lResultado += (((long)(g_cNumero[g_cIndiceNumero-1]))-48)*mul;
				g_cIndiceNumero--;
				mul*=10;
			}
		}
	}
	
	g_Column = 0;
	g_Line	 = 3;
	//Limpiar el GLCD con el color de brocha
	ATGClear();	
							
	printf("Numero almacenado en la pila.");
	ESS_Delay(1000);
								
	return lResultado;
}

void CalculadoraRPN(void)
{	
	while(1)
	{
		PORTD=0x00;	
		TRISD=0xf0;
		
		if(KB_Hit() == 0xff)
		{
			ucTecla = KB_Get();
			switch(ucTeclaPresionada[ucTecla])
			{
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				{
					if(g_cIndicePilaNumeros < 2)
					{
							//Limpiar el GLCD con el color de brocha
							ATGClear();							
																				
							g_Column = 0;
							g_Line	 = 2;
							
							printf("Se necesitan por lo menos dos Numeros en la pila.");
							ESS_Delay(1000);
					}					
					else
					{
						switch(ucTeclaPresionada[ucTecla])
						{
							case 'A':				//Suma
							{
								g_llResultado = 	(long long) g_lPilaNumeros[g_cIndicePilaNumeros-2] + (long long) g_lPilaNumeros[g_cIndicePilaNumeros-1];
								break;
							}
							case 'B':				//Resta
							{
								g_llResultado = 	(long long) g_lPilaNumeros[g_cIndicePilaNumeros-2] - (long long) g_lPilaNumeros[g_cIndicePilaNumeros-1];
								break;
							}
							case 'C':				//Multiplicación
							{
								g_llResultado = 	(long long) g_lPilaNumeros[g_cIndicePilaNumeros-2] * (long long) g_lPilaNumeros[g_cIndicePilaNumeros-1];
								break;
							}case 'D':				//División
							{
								if(g_lPilaNumeros[g_cIndicePilaNumeros-1] == 0)
									g_cDivisionCero = 1;
								else
									g_llResultado = 	(long long) g_lPilaNumeros[g_cIndicePilaNumeros-2] / (long long) g_lPilaNumeros[g_cIndicePilaNumeros-1];
								break;
							}
						}
						if (g_cDivisionCero == 1)
						{
							//Limpiar el GLCD con el color de brocha
							ATGClear();							
																				
							g_Column = 0;
							g_Line	 = 2;
							
							printf("No puede dividir entre cero, intente de nuevo.");
							ESS_Delay(1000);
							g_cDivisionCero = 0;
						}
						else if((g_llResultado >= -2147483647) && (g_llResultado <= 2147483647))
						{
							g_lPilaNumeros[g_cIndicePilaNumeros-2] = (long) g_llResultado;
							g_cIndicePilaNumeros--;
						}
						else
						{
							//Limpiar el GLCD con el color de brocha
							ATGClear();							
																				
							g_Column = 0;
							g_Line	 = 2;
							
							printf("Resultado invalido, intente de nuevo.");
							ESS_Delay(1000);
						}
					}
					DibujarPantallaCalculadora();
					break; 
				}
				case '#':
				{
					if(g_cIndiceNumero > 0)
					{
						if(g_cIndicePilaNumeros == 8)
						{	
							g_cIndiceNumero = 0;
							//Limpiar el GLCD con el color de brocha
							ATGClear();							
																				
							g_Column = 0;
							g_Line	 = 2;
							
							printf("La pila esta llena. No se puede almacenar el numero.");
							ESS_Delay(1000);
							DibujarPantallaCalculadora();				
						}
						else
						{
							g_lPilaNumeros[g_cIndicePilaNumeros] = RegresaNumero();
							
							if(g_lPilaNumeros[g_cIndicePilaNumeros] != 0)
								g_cIndicePilaNumeros++;
							DibujarPantallaCalculadora();
						}
					}
					break;
				}
				case '*':
				{
					if(g_cIndiceNumero > 0)
					{
						g_cNumero[g_cIndiceNumero-1] = ' ';
						g_cIndiceNumero--;
						DibujarPantallaCalculadora();
					}
					break;
				}
				default:
				{ 
					if(g_cIndiceNumero == 0 && ucTeclaPresionada[ucTecla] == '0')
					{

					}
					else if(g_cIndiceNumero < 10)
					{
						g_cNumero[g_cIndiceNumero] = ucTeclaPresionada[ucTecla];
						g_cIndiceNumero++;
						DibujarPantallaCalculadora();
					}
					break;
				}	
			}
			ESS_Delay(400);
		}
	}
}



//////////////////////////////////
//			Puyo Puyo			//
//////////////////////////////////

///////////////////////////////
//	     Puyo Parameters	 //
///////////////////////////////

int DrawPuyo[2] = {1,2};
int Color[16][2] =  {{1,1},{1,2},{1,3},{1,4},
					 {2,1},{2,2},{2,3},{2,4},
					 {3,1},{3,2},{3,3},{3,4},
					 {4,1},{4,2},{4,3},{4,4}}; 
int RamColor = 0;
int PuyoRotation	= 0;
int PuyoImpact[2]	= {0,0};
int PilaPuyo[16][2];
int ContPila = 0;

/////////////////////
//		Board	   //
/////////////////////

int Board [6][12];
int PuyoBoardPosition[2][2];
int i;
int j;

/////////////////////
//	   Others	   //
/////////////////////

int push1		= 0;
int push		= 0;
int nCount		= 0;
int Game_Over	= 0;
int Refresh		= 0;
int appIsRunning = 0;

//////////////////////
//	   Constant		//
//////////////////////

#define right 0
#define left  1
#define up    2
#define down  3

struct POINT A,B;

//////////////////////
//		Cubos		//
//////////////////////

rom unsigned char SpriteBlanco[8]=
{
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00
};
rom unsigned char SpriteCubo1[8]=
{
	0xFF,
	0x81,
	0x81,
	0x99,
	0x99,
	0x81,
	0x81,
	0xFF
};
rom unsigned char SpriteCubo2[8]=
{
	0xFF,
	0xC3,
	0xA5,
	0x99,
	0x99,
	0xA5,
	0xC3,
	0xFF
};
rom unsigned char SpriteCubo3[8]=
{
	0xFF,
	0x81,
	0x81,
	0x81,
	0x81,
	0x81,
	0x81,
	0xFF
};
rom unsigned char SpriteCubo4[8]=
{
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF
};

/////////////////////
//	   Methods	   //
/////////////////////

void DrawBoard()
{
	PORTD = 0x00;
	TRISD = 0x00;

	//////////////////////////////////////
	//			Dibujamos Marco			//
	//////////////////////////////////////
	
	A.x=15;
	A.y=7;
	B.x=113;
	B.y=7;	
	ATGLine(&A,&B);
	
	A.x=112;
	A.y=8;
	B.x=112;
	B.y=56;	
	ATGLine(&A,&B);
	
	A.x=112;
	A.y=56;
	B.x=15;
	B.y=56;	
	ATGLine(&A,&B);
	
	A.x=15;
	A.y=56;
	B.x=15;
	B.y=7;	
	ATGLine(&A,&B);

	for(j=5;j>=0;j--)
	{
		for(i=11;i>=0;i--)
		{
			g_GLCDX=(i*8)+16;
			g_GLCDY=(j*8)+8;
			
			switch(Board[j][i])
			{
				case 0:					//Cubo blanco
				{
					ATGWriteCombineNx8Sprite(SpriteBlanco,8);	
					break;
				}
				case 1:					//Cubo 1
				{
					ATGWriteCombineNx8Sprite(SpriteCubo1,8);	
					break;
				}
				case 2:					//Cubo 2
				{
					ATGWriteCombineNx8Sprite(SpriteCubo2,8);
					break;
				}
				case 3:					//Cubo 3
				{
					ATGWriteCombineNx8Sprite(SpriteCubo3,8);
					break;
				}
				case 4:					//Cubo 4
				{
					ATGWriteCombineNx8Sprite(SpriteCubo4,8);
					break;
				}
				case 10:				//Cubo negro
				{
					ATGWriteCombineNx8Sprite(SpriteCubo4,8);
					break;
				}
			}	
		}
	}
}
void GameOver()
{
	Game_Over = 1;
	
	for (i = 0 ; i < 6 ; i++)
	{
		for (j = 0; j < 12 ; j++)
		{
			Board [i][j] = 10;									// Fill all with 0
		} 
	}
	DrawBoard();
}

void NewPuyo()
{
	PuyoImpact[0] = 0;	// Indicate that puyo isn´t crashed
	PuyoImpact[1] = 0;	// Indicate that puyo isn´t crashed

	PuyoRotation = 1;

	if (RamColor < 15)  RamColor++;
	else				RamColor=0;

	DrawPuyo[0] = Color[RamColor][0];
	DrawPuyo[1] = Color[RamColor][1];
	
	PuyoBoardPosition[0][0] = 2;		// Set xPosition of new Puyo
	PuyoBoardPosition[0][1] = 0;		// Set yPosition of new Puyo

	PuyoBoardPosition[1][0] = 2;		// Set xPosition of new Puyo
	PuyoBoardPosition[1][1] = 1;		// Set yPosition of new Puyo

	if (Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] == 0 )  // If the new Puyo have problems with the board Puyos, the game ends
	{
		Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
		Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
	}
	else					
	{
		Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
		Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
		GameOver();
	}
}

void NewGame()
{
	appIsRunning = 1;									// Boolean flag to state whether app is running or not.

	PuyoImpact[0]	= 1;
	PuyoImpact[1]	= 1;
	nCount			= 0;
	push			= 0;
	push1			= 0;
	PuyoRotation	= 1;
	RamColor		= 1;
	Game_Over = 0;
	
	for (i=0;i<6;i++)
		for(j=0;j<12;j++)
			Board[i][j] = 0;
	
}
void Moving(void)
{
	for (j = 11 ; j > 0 ; j --)
	{
		for (i = 0 ; i < 5 ; i++)
		{
			if (Board[i][j] == 0)
			{
				int cont = j;
				while (cont > 1)
				{
					if (Board[i][cont] != 0 )
					{
						Board[i][j] = Board[i][cont];
						Board[i][cont] = 0;
						cont = 0;
					}
					cont--;
				}
			}
		}
	}
}
void Clear()
{
	int x = 0;
	do
	{
		Board [PilaPuyo[x][0]] [PilaPuyo[x][1]] = 0;
		x++;
	}
	while(x < ContPila);
}

void floodfillrec(int x, int y, int origen)
{
	int permiso = 1;
	int cont;
	
	if(ContPila>0)
	{
		for(cont = 0 ; cont < ContPila-1 ; cont++)
		{
			if(PilaPuyo[cont][0] == x && PilaPuyo[cont][1] == y)
			{
				permiso = 0;
			}
		}
	}

	if (permiso)
	{
		//if square match at right
		if (x < 5 && Board[x+1][y] == Board[x][y] && origen != left)
		{
			if(ContPila == 0)
			{
				PilaPuyo[ContPila][0] = x;
				PilaPuyo[ContPila][1] = y;
				ContPila++;
			}
			PilaPuyo[ContPila][0] = x+1;
			PilaPuyo[ContPila][1] = y;
			ContPila++;
			floodfillrec(x+1,y,right);
		}
		//left
		if (x > 0 && Board[x-1][y] == Board[x][y] && origen!= right)
		{
			if(ContPila == 0)
			{
				PilaPuyo[ContPila][0] = x;
				PilaPuyo[ContPila][1] = y;
				ContPila++;
			}
			PilaPuyo[ContPila][0] = x-1;
			PilaPuyo[ContPila][1] = y;
			ContPila++;
			floodfillrec(x-1,y,left);
		}
		//up
		if (y > 0 && Board[x][y] == Board[x][y-1] && origen!= down)
		{
			if(ContPila == 0)
			{
				PilaPuyo[ContPila][0] = x;
				PilaPuyo[ContPila][1] = y;
				ContPila++;
			}
			PilaPuyo[ContPila][0] = x;
			PilaPuyo[ContPila][1] = y-1;
			ContPila++;
			floodfillrec(x,y-1,up);
		}
		//down
		if (y < 11 && Board[x][y] == Board[x][y+1] && origen!= up)
		{
			if(ContPila == 0)
			{
				PilaPuyo[ContPila][0] = x;
				PilaPuyo[ContPila][1] = y;
				ContPila++;
			}
			PilaPuyo[ContPila][0] = x;
			PilaPuyo[ContPila][1] = y+1;
			ContPila++;
			floodfillrec(x,y+1,down);
		}
	}
}
int floodfill()
{
	int erased = 0;

	for (j = 11 ; j >= 0 ; j--)
	{
		for (i = 0 ; i < 5 ; i++)
		{
			if (Board[i][j] != 0)
			{
				floodfillrec(i, j, -1);
				if (ContPila > 3)
				{
					Clear();
					Moving();
					erased = 1;
				}
				ContPila = 0;
			}
		}
	}

	return erased;
}
void MoveLeft()
{
	if (push == 2)
	{	
		Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = 0;					// Clear Puyo 1
		Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = 0;					// Clear Puyo 2

		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		if ((PuyoRotation == 1 || PuyoRotation == 3) && PuyoBoardPosition[0][0] > 0)
		{	
			if ( Board [PuyoBoardPosition[0][0] - 1] [PuyoBoardPosition[0][1]] == 0 &&
				 Board [PuyoBoardPosition[1][0] - 1] [PuyoBoardPosition[1][1]] == 0 &&
				 PuyoImpact[0] == 0 && PuyoImpact[1] == 0)
			{						
				PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] - 1;							// Forward y position of Puyo 1
				PuyoBoardPosition[1][0] = PuyoBoardPosition[1][0] - 1;							// Forward y position of Puyo 2
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 2 on Board
				Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			}
			else
			{							
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
				Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			}
		}
		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		if (PuyoRotation == 2 && PuyoBoardPosition[0][0] > 0 && 
			Board [PuyoBoardPosition[0][0] - 1] [PuyoBoardPosition[0][1]] == 0 &&
			PuyoImpact[0] == 0 && PuyoImpact[1] == 0)
		{	
			
			PuyoBoardPosition[1][0] = PuyoBoardPosition[1][0] - 1;							// Forward y position of Puyo 2
			PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] - 1;							// Forward y position of Puyo 1
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
		}

		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		if (PuyoRotation == 0 && PuyoBoardPosition[1][0] > 0 && 
			Board [PuyoBoardPosition[1][0] - 1] [PuyoBoardPosition[1][1]] == 0 &&
			PuyoImpact[0] == 0 && PuyoImpact[1] == 0)
		{	
			
			PuyoBoardPosition[1][0] = PuyoBoardPosition[1][0] - 1;							// Forward y position of Puyo 2
			PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] - 1;							// Forward y position of Puyo 1
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
		}
		else 
		{
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 2 on Board
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
		}
		push = 0;
	}
	push ++;
}
void MoveRight()
{
	if (push == 2)
	{	
		Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = 0;					// Clear Puyo 1
		Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = 0;					// Clear Puyo 2

		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		if ((PuyoRotation == 1 || PuyoRotation == 3) && PuyoBoardPosition[0][0] < 5)
		{	
			if ( Board [PuyoBoardPosition[0][0] + 1] [PuyoBoardPosition[0][1]] == 0 &&
				 Board [PuyoBoardPosition[1][0] + 1] [PuyoBoardPosition[1][1]] == 0 &&
				 PuyoImpact[0] == 0 && PuyoImpact[1] == 0)
			{						
				PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] + 1;							// Forward y position of Puyo 1
				PuyoBoardPosition[1][0] = PuyoBoardPosition[1][0] + 1;							// Forward y position of Puyo 2
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 2 on Board
				Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			}
			else
			{							
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
				Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			}
		}
		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		else if (PuyoRotation == 2 && PuyoBoardPosition[1][0] < 5 && 
			Board [PuyoBoardPosition[1][0] + 1] [PuyoBoardPosition[0][1]] == 0 &&
			PuyoImpact[0] == 0 && PuyoImpact[1] == 0)
		{	
			
			PuyoBoardPosition[1][0] = PuyoBoardPosition[1][0] + 1;							// Forward y position of Puyo 2
			PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] + 1;							// Forward y position of Puyo 1
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
		}

		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		else if (PuyoRotation == 0 && PuyoBoardPosition[0][0] < 5 && 
			Board [PuyoBoardPosition[0][0] + 1] [PuyoBoardPosition[0][1]] == 0 &&
			PuyoImpact[0] == 0 && PuyoImpact[1] == 0)
		{	
			
			PuyoBoardPosition[1][0] = PuyoBoardPosition[1][0] + 1;							// Forward y position of Puyo 2
			PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] + 1;							// Forward y position of Puyo 1
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
		}
		else 
		{
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 2 on Board
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
		}
		push = 0;
	}
	push ++;
}
void MoveDown()
{	
	if (push1 == 2)
	{	
		Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = 0;					// Clear Puyo 1
		Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = 0;					// Clear Puyo 2

		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		if (PuyoRotation == 1 && PuyoBoardPosition[1][1] < 11 && Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1] + 1] == 0)
		{
			PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1] + 1;							// Forward y position of Puyo 1
			PuyoBoardPosition[1][1] = PuyoBoardPosition[1][1] + 1;							// Forward y position of Puyo 2
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board	
		}

		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		else if ( (PuyoRotation == 2 || PuyoRotation == 0 ) && PuyoBoardPosition[1][1] < 11 && PuyoBoardPosition[0][1] < 11)
		{
			if (Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1] + 1] == 0)
			{
				PuyoBoardPosition[1][1] = PuyoBoardPosition[1][1] + 1;							// Forward y position of Puyo 2
				Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board	
			}
			else
			{		
				Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board	
				PuyoImpact[1] = 1;
			}
			if (Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1] + 1] == 0)
			{
				PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1] + 1;							// Forward y position of Puyo 1
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
			}
			else
			{
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
				PuyoImpact[0] = 1;
			}
		}

		// If Puyo 2 doesn't leave the board or collides with another Puyo, to advance a position
		else if (PuyoRotation == 3 && PuyoBoardPosition[0][1] < 11 && Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1] + 1] == 0)
		{
			PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1] + 1;							// Forward y position of Puyo 1
			PuyoBoardPosition[1][1] = PuyoBoardPosition[1][1] + 1;							// Forward y position of Puyo 2
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board	
		}
		else 
		{
			PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1];								// Forward y position of Puyo 1
			PuyoBoardPosition[1][1] = PuyoBoardPosition[1][1];								// Forward y position of Puyo 2
			Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
			Board [PuyoBoardPosition[1][0]] [PuyoBoardPosition[1][1]] = DrawPuyo[1];		// Draw new Puyo 2 on Board
			PuyoImpact[0] = 1;
			PuyoImpact[1] = 1;
		}
		push1 = 0;
	}
	push1 ++;
}
void Rotation() 
{
	if (push == 2)
	{	
		if (PuyoRotation == 1)  // If Puyo is one position
		{
			// If Puyo 1 doesn't leave the board or collides with another Puyo, to advance a position
			if (PuyoBoardPosition[0][0]  > 0 &&												
				Board [PuyoBoardPosition[0][0] - 1] [PuyoBoardPosition[0][1]] == 0 &&
				Board [PuyoBoardPosition[0][0] - 1] [PuyoBoardPosition[0][1] + 1] == 0)
			{						
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = 0;					// Clear Puyo 1
				PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] - 1;							// Forward x position of Puyo 1
				PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1] + 1;							// Forward y position of Puyo 1
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
				PuyoRotation = 2;
			}
		}
		else if (PuyoRotation == 2)  // If Puyo is two position
		{
			// If Puyo 1 doesn't leave the board or collides with another Puyo, to advance a position
			if (PuyoBoardPosition[0][1]  < 11 &&												
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1] + 1] == 0 &&
				Board [PuyoBoardPosition[0][0] + 1] [PuyoBoardPosition[0][1] + 1] == 0)
			{						
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = 0;					// Clear Puyo 1
				PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] + 1;							// Forward x position of Puyo 1
				PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1] + 1;							// Forward y position of Puyo 1
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
				PuyoRotation = 3;
			}
		}
		else if (PuyoRotation == 3)  // If Puyo is three position
		{
			// If Puyo 1 doesn't leave the board or collides with another Puyo, to advance a position
			if (PuyoBoardPosition[0][0] < 5 &&												
				Board [PuyoBoardPosition[0][0] + 1] [PuyoBoardPosition[0][1]] == 0 &&
				Board [PuyoBoardPosition[0][0] + 1] [PuyoBoardPosition[0][1] - 1] == 0)
			{						
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = 0;					// Clear Puyo 1
				PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] + 1;							// Forward x position of Puyo 1
				PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1] - 1;							// Forward y position of Puyo 1
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
				PuyoRotation = 0;
			}
		}
		else if (PuyoRotation == 0)  // If Puyo is zero position
		{
			// If Puyo 1 doesn't leave the board or collides with another Puyo, to advance a position
			if (PuyoBoardPosition[0][1]  > 0 &&												
				Board [PuyoBoardPosition[0][0]]     [PuyoBoardPosition[0][1] - 1] == 0 &&
				Board [PuyoBoardPosition[0][0] - 1] [PuyoBoardPosition[0][1] - 1] == 0)
			{						
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = 0;					// Clear Puyo 1
				PuyoBoardPosition[0][0] = PuyoBoardPosition[0][0] - 1;							// Forward x position of Puyo 1
				PuyoBoardPosition[0][1] = PuyoBoardPosition[0][1] - 1;							// Forward y position of Puyo 1
				Board [PuyoBoardPosition[0][0]] [PuyoBoardPosition[0][1]] = DrawPuyo[0];		// Draw new Puyo 1 on Board
				PuyoRotation = 1;
			}
		}
		push = 0;
	}
	push ++;
}

void PuyoPuyo()
{
	NewGame();
	while(1)
	{	
		if(appIsRunning && !Game_Over)
		{	
			//////////////////////////////////////////
			//		Configuración para teclado		//
			//////////////////////////////////////////

			if (PuyoImpact[0] == 1 && PuyoImpact[1] == 1 )				// If Puyo is placed, it creates a new 
			{
				while(floodfill());
	
				NewPuyo();
			}
			if (nCount == 400)			// Puyo falls one position each 5 ms
			{
				MoveDown();	
				DrawBoard();				// Call DrawBoard			
				nCount = 0;	
			}	
			nCount++;
		}
		
		PORTD=0x00;	
		TRISD=0xf0;
		
		if(KB_Hit() == 0xff)
		{
			ucTecla = KB_Get();
			
			switch(ucTeclaPresionada[ucTecla])
			{
				case '2':		//Rotar
				{
					Rotation();
					break;
				}
				case '4':		//Izquierda
				{
					MoveRight();
					break;
				}
				case '5':		//Pausa
				{
					if(appIsRunning)
						appIsRunning = 0;
					else
						appIsRunning = 1;
					break;
				}
				case '6':		//Derecha
					MoveLeft();
					break;
				case '8':		//Bajar
				{
					MoveDown();	
					break;	
				}
				case '*':		//Juego Nuevo
				{
					NewGame();
					break;
				}
			}
			DrawBoard();				// Call DrawBoard
		}
	}
}


void main(void)
{
	//////////////////////////////////
	//			Graficos			//
	//////////////////////////////////
											
	//Se le indica la salida estándar que debe usar la función definida por el usuario para imprimir un caracter
	stdout=_H_USER;

	//Bus de control GLCD (y otros periféricos) como salida 
	PORTB=0xc0;    
	TRISB=GLCD_TRIS_OUTPUT_MASK; 
	
	//Inicializar el Modulo GLCD (Limpieza del estado interno del GLCD tras el reset o power on)
	ATGInit();
	//Activar la polarización del cristal líquido. 
	ATGShow();
	
	//Establecer las tintas de brocha (relleno) y pluma (filete)
	g_GLCDBrush=GLCD_BLACK;
	g_GLCDPen=GLCD_WHITE;	
		
	//Limpiar el GLCD con el color de brocha
	ATGClear();

	//////////////////////////////////////////
	//			Comunicación USB			//
	//////////////////////////////////////////
	
	ESS_USBSetOverlayCommandHandler(USBOverlayCommandHandler);	


	while(1)
	{	
		//////////////////////////////////////////
		//			Comunicación USB			//
		//////////////////////////////////////////
		
		ESS_USBStatusLedTasks;
		ESS_USBTasks(NULL);	
		
		//////////////////////////////////////////
		//			Control Motor a pasos		//
		//////////////////////////////////////////
		
		StepperMotor(g_nStep);
		ESS_Delay(100);
	}
}

	/*	PORTD=0x00;	
		TRISD=0xf0;	
		
		//////////////////////////////////////////
		//			Comunicación USB			//
		//////////////////////////////////////////
		
		ESS_USBStatusLedTasks;
		ESS_USBTasks(NULL);	
		
		//////////////////////////////////////
		//			Juego Puyo Puyo			//
		//////////////////////////////////////
					
		//PuyoPuyo();	
		
		//////////////////////////////////////
		//			Calculadora RPN			//
		//////////////////////////////////////
		
	//	CalculadoraRPN();
	
		//////////////////////////////////////////
		//			Motor a pasos 3DScan		//
		//////////////////////////////////////////
		
	
		if (g_nEnable)
		{
			StepperMotor(g_nStep);
			ESS_Delay(10);
		}
				
		if(KB_Hit() == 0xff)
		{
			ucTecla = KB_Get();
			switch(ucTeclaPresionada[ucTecla])
			{
				case 'A':
					g_nDirection = 0;
					break;
				case 'B':
					g_nDirection = 1;
					break;
				case 'C':
					g_nDelay += 1;
					break;
				case 'D':
					g_nDelay -= 1;
					break;
				case '*':
					if(g_nEnable)
						g_nEnable = 0;
					else
						g_nEnable = 1;
					break;	
			}
		}*/