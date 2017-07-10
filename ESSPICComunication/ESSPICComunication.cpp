
#include "stdafx.h"
#include <Windows.h>
#include "command.h"
#include <string>
#include <iostream>

using namespace std;

bool SendPICCommand(HANDLE hUSB,PICCOMMAND* pInCmd)
{
	unsigned char Zero=0;
	DWORD dwBytesWritten=0;
	WriteFile(hUSB,&Zero,1,&dwBytesWritten,NULL);  //Debes enviar un byte 0x00 antes de enviar un datagrama
	WriteFile(hUSB,&pInCmd->m_bCommandCode,1,&dwBytesWritten,NULL);
	WriteFile(hUSB,&pInCmd->m_bParamsBufferSize,1,&dwBytesWritten,NULL);
	if(pInCmd->m_bParamsBufferSize)
	{
		WriteFile(
			hUSB,
			pInCmd->m_abParamsBuffer,
			pInCmd->m_bParamsBufferSize,
			&dwBytesWritten,NULL);
	}
	return true;

}
bool ReceivePICCommand(HANDLE hUSB,PICCOMMAND* pOutCmd)
{
	DWORD dwBytesWritten=0;
	memset(pOutCmd->m_abParamsBuffer,0,16);
	ReadFile(hUSB,&pOutCmd->m_bCommandCode,1,&dwBytesWritten,NULL);
	ReadFile(hUSB,&pOutCmd->m_bParamsBufferSize,1,&dwBytesWritten,NULL);
	if(pOutCmd->m_bParamsBufferSize)
	{
		ReadFile(hUSB,pOutCmd->m_abParamsBuffer,
			pOutCmd->m_bParamsBufferSize,
			&dwBytesWritten,NULL);
	}
	return true;
}
int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hUSB=CreateFile(
		L"\\\\.\\com10",                //Nombre device
		GENERIC_READ|GENERIC_WRITE,    //Permisos de acceso
		0,							   //Sin compartir
		NULL,                          //Seguridad del usuario actual
		OPEN_EXISTING,				   //Debe existir                
		0,                             //Nada en especial
		NULL);                         //Acceso Mono hilo
	if(hUSB==INVALID_HANDLE_VALUE)
	{
		hUSB=CreateFile(
			L"\\\\.\\com9",                //Nombre device
			GENERIC_READ|GENERIC_WRITE,    //Permisos de acceso
			0,							   //Sin compartir
			NULL,                          //Seguridad del usuario actual
			OPEN_EXISTING,				   //Debe existir                
			0,                             //Nada en especial
			NULL);                         //Acceso Mono hilo
		if(hUSB==INVALID_HANDLE_VALUE)
		{
			hUSB=CreateFile(
				L"\\\\.\\com8",                //Nombre device
				GENERIC_READ|GENERIC_WRITE,    //Permisos de acceso
				0,							   //Sin compartir
				NULL,                          //Seguridad del usuario actual
				OPEN_EXISTING,				   //Debe existir                
				0,                             //Nada en especial
				NULL);                         //Acceso Mono hilo
			if(hUSB==INVALID_HANDLE_VALUE)
			{
				hUSB=CreateFile(
					L"\\\\.\\com7",                //Nombre device
					GENERIC_READ|GENERIC_WRITE,    //Permisos de acceso
					0,							   //Sin compartir
					NULL,                          //Seguridad del usuario actual
					OPEN_EXISTING,				   //Debe existir                
					0,                             //Nada en especial
					NULL);                         //Acceso Mono hilo
				if(hUSB==INVALID_HANDLE_VALUE)
				{
					printf("Error al intentar conectarse con el PIC");
				}
			}
		}
	}

	PICCOMMAND SendString={COMMAND_SEND_STRING,16};
	PICCOMMAND ResponseString;

	string CadenaEntrada;

	printf("\n\nIngrese el mensaje que desea enviar...  ");
	getline(cin,CadenaEntrada);

	int j = 0;

	for(int i = 0; i < CadenaEntrada.length() ; i++)
	{	
		SendString.m_abParamsBuffer[j] = CadenaEntrada[i];

		if(j == 15)
		{
			SendPICCommand(hUSB,&SendString);
			j = 0;
		}
		else
			j++;
		
		if(i == CadenaEntrada.length()-1)
		{
			SendString.m_abParamsBuffer[j] = '\n';
			SendPICCommand(hUSB,&SendString);
		}
	}

	ReceivePICCommand(hUSB,&ResponseString);

	switch(ResponseString.m_bCommandCode)
	{
	case ERROR_PIC_SUCCESS:
		printf("%c",*((char*)ResponseString.m_abParamsBuffer));
		break;
	case ERROR_PIC_UNIMPLEMENTED_COMMAND:
		printf("%s\n","El pic ha respondido, pero no ha ejecuta el comando ya que no lo reconoce");
		printf("%s\n","Verifique que la aplicación esté en ejecución");
		break;
	}
	CloseHandle(hUSB);
	system("PAUSE");
	return 0;
}

