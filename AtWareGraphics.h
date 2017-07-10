/* ------------------------------------------------------
@Ware Graphics 1.0 (ATG) for the @Ware EduProto III
	Interface File

(c) 2009 @Ware Systems ESS Version 1.2
(c) 2010 @Ware Systems ESS Version 2.5
(c) 2013 @Ware Graphics Version 1.0

@Ware Graphics 1.0 part of the @Ware ESS SDK Files.


Author: M. Sc. Pedro Arturo Cornejo Torres
mailto:pcornejo@gdl.cinvestav.mx

Contenido:
	Este archivo contiene las definiciones para realizar
	síntesis gráfica en 2D desde una aplicación para PIC18FXXXX
--------------------------------------------------------*/

#ifndef ATWAREGRAPHICS_INCLUDED
#define ATWAREGRAPHICS_INCLUDED


#define GLCD_STATUSREAD_BUSY   			0x80
#define GLCD_STATUSREAD_ONOFF  			0x20
#define GLDC_STATUSREAD_RESET  			0x10

#define GLCD_CMD_SET_ADDRESSX(X)  		g_ucGLCDDataOut=(0xB8|(X))
#define GLCD_CMD_SET_ADDRESSY(Y)  		g_ucGLCDDataOut=(0x40|(Y))
#define GLCD_CMD_SET_ADDRESSZ(Z)  		g_ucGLCDDataOut=(0xC0|(Z))
#define GLCD_CMD_DISPLAY_ONOFF(Switch)	g_ucGLCDDataOut=(0x3e|(Switch))
#define GLCD_DISPLAY_ON					0x01
#define GLCD_DISPLAY_OFF				0x00


#define GLCD_BLACK						0x00			//Pixel Negro
#define GLCD_WHITE						0x01			//Pixel Blanco

#define GLCD_SRCCOPY					0x01
#define GLCD_NOTSRCCOPY					0x02
#define GLCD_OR 						0x04
#define GLCD_AND						0x08
#define GLCD_NOTSRCAND					0x10
#define GLCD_XOR						0x20
#define GLCD_SPRITE_RAM					0x80
#define GLCD_SPRITE_ROM					0x00

extern unsigned char g_ucGLCDDevChild;					//0 - bit 1, 1 - bit 2
extern unsigned char g_ucGLCDDataIn;
extern unsigned char g_ucGLCDDataOut;
extern unsigned char g_ucGLCDFlagsAndCombineOptions;
extern unsigned int	 g_GLCDX;							//Coordenadas del cursor x
extern unsigned int	 g_GLCDY;							//Coordenadas del cursor y
extern unsigned char g_GLCDPen;
extern unsigned char g_GLCDBrush;
extern char 		 g_Column;
extern char 		 g_Line;
extern void 		(*g_pfnCustomOutputMerger)(void);
extern char 		g_MinLimitY;
extern char			g_MaxLimitY;
extern rom unsigned char    g_LookUpPixel[]; //Máscaras de pixel
struct POINT
{
	int x,y;
};
struct LIMIT
{
	unsigned char left,right;
};

extern struct LIMIT g_Limits[];

void User_ATGReadData(void);
void User_ATGWriteData(void);
void User_ATGWriteCommand(void);
void User_ATGReadStatus(void);

void ATGInit(void);																				//Enciende Pantalla
void ATGSetPixel(void);																			//Trabaja con g_GLCDX, g_GLCDY y color g_GLCDPen
void ATGSetLimit(void);																			//Trabaja con g_GLCDX, g_GLCDY
void ATGLine(struct POINT* p0,struct POINT* p1);
void ATGLineLimits(struct POINT* p0,struct POINT* p1);
void ATGEllipse(int cx,int cy,int ra,int rb,unsigned char bFilled);
void ATGPoly(struct POINT* pPoints,unsigned char nPoints,unsigned char bFilled);
void ATGPolyROM(rom struct POINT *pPoints,unsigned char nPoints,unsigned char bFilled);
void ATGResetLimits(void);
void ATGFillLimits(void);
void ATGWriteCombineNx8Sprite(rom unsigned char* pSprite,unsigned char ucN);
void ATGSystemFontCharOut(char cAscii);															//Usa g_Column y g_Line
void ATGClear(void);
void ATGHide(void);
void ATGShow(void);
void ATGScrollConsole(void);
int  ATGConsolePutChar(char c);

#endif



