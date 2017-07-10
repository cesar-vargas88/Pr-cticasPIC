/* ------------------------------------------------------
@Ware Executive Subjacent System (ESS) Interface File

(c) 2009 @Ware Systems ESS Version 1.2
(c) 2010 @Ware Systems ESS Version 2.5

@Ware ESS SDK File.

Author: M. Sc. Pedro Arturo Cornejo Torres
mailto:pcornejo@gdl.cinvestav.mx

Contenido:
	Este archivo contiene las definiciones para realizar
	llamadas al ESS desde una aplicación para PIC18FXXXX
--------------------------------------------------------*/
#if !defined(__INTERFACE_H)
#define __INTERFACE_H

#include "command.h"

union uESSCONTROL
{
	struct ESSCONTROLBITS
	{
		unsigned char g_bRunProgram:1;
		unsigned char g_bLowPower:1;
		unsigned char g_bStatusLEDEnable:1;
		unsigned char g_bUSBEnable:1;
		unsigned char g_bLoopProgram:1;
		unsigned char g_bAllowSuspend:1;
	}bits;
	unsigned char g_byteControl;
};

typedef rom void* (*GET_API_PFN)(char);
typedef char (*SYSTEM_CALL_PFN)(char,unsigned short long);

/*
	ESS API Access Address
	Use GET_API_PFN Pointer to Function type to perform a valid call
*/
#define GET_API_VECTOR 0x100
/*
	ESS System Call Address
	Use SYSTEM_CALL_PFN Pointer to Function type to perform a valid call
*/
#define SYSTEM_CALL_VECTOR 0x180

/*
	Macro helpers to perform an API Access or a System Call
*/

#define ESS_GetAPI ((GET_API_PFN)GET_API_VECTOR)
#define ESS_SystemCall ((SYSTEM_CALL_PFN)SYSTEM_CALL_VECTOR)

struct TRANSCEIVER_STATE_MACHINE;
struct TRANSCEIVER_VIRTUAL_CONST_TABLE;


/*
	Transceiver State Machine Structures 
*/
struct TRANSCEIVER_VIRTUAL_CONST_TABLE
{
	char m_id;
	//Read from transceiver
	unsigned char (*m_pfnRead)(void* a,unsigned char b);
	//Write to transceiver 
	unsigned char (*m_pfnWrite)(void* a,unsigned char b);
	//Terminal ready to send
	unsigned char (*m_pfnTxReady)(void);
	//Command Handler. Called when a packet is completed.
	void (*m_pfnCommandHandler)(struct TRANSCEIVER_STATE_MACHINE*);
	//Input buffer
	unsigned char* m_pInputBuffer;
	unsigned char  m_bInputBufferSize;
	//Memory of the command associated to this machine
	struct PICCOMMAND* m_pPicCmd; 
};
typedef char (*OverlayCommandHandler)(struct TRANSCEIVER_STATE_MACHINE*);

struct TRANSCEIVER_STATE_MACHINE
{
	ROM struct TRANSCEIVER_VIRTUAL_CONST_TABLE* m_vtbl;
	//Packet Machine State
	unsigned char m_bPacketState;
	//Packet Param Buffer Index
	unsigned char m_bPacketBufferIndex;
	//Machine State Auto reset down counter
	long m_lPacketAutoResetCounter;
	//Overlay for new commands
	OverlayCommandHandler m_pfnOverlayCommandHandler;
	
};	


/*
	This structure provides access to functionality by @Ware USB API. 
	
	Use ESS_GetAPI invoker helper to query for an USB_API structure 
	pointer. 
	
	The API is stored in rom space and it is identified by the
	constant 0 (zero).
	
	Example:

	rom struct USB_API* pUSBAPI; 
	pUSBAPI=(rom struct USB_API*)ESS_GetAPI(API_INDEX_USB);
	
*/
#define API_INDEX_USB 0
struct USB_API
{
	/* See USB_XXXXXXX Constants below for a description */
	volatile unsigned char* APIUSBDeviceState;
	/* Pointer to USB State Machine Structure */
	struct TRANSCEIVER_STATE_MACHINE* APIUSBTSM;
	/* Call it once per main iteration and before APIUSBProcessIncommingPacketsStateMachine */ 
	void (*APIUSBDeviceTasks)(void);
	/* Call it once per main iteration and after APIUSBProcessIncommingPacketsStateMachine*/
	void (*APIUSBCDCTxService)(void);
	/* Process an iteration of the USB State Machine Parser */
	void (*APIUSBProcessIncommingPacketsStateMachine)(struct TRANSCEIVER_STATE_MACHINE*);
	/* Update user interface notification LEDS. Called once per iteration */
	void (*APIStatusLedTasks)(void);	
};

/*
	This is structure contains access to functionality
	provided by @Ware USB API. 
	
	Use ESS_GetAPI invoker helper to query for an USART_API structure 
	pointer. 
	
	The API is stored in rom space and it is identified by the
	constant 0 (zero).
	
	Example:

	rom struct USART_API* pUSARTAPI; 
	pUSARTAPI=(rom struct USART_API*)ESS_GetAPI(API_INDEX_USART);
*/
#define API_INDEX_USART 1
struct USART_API
{	
	/* See USART_STATE_XXXXX constants below for a description*/
	volatile unsigned char* APIUSARTDeviceState;
	/* Pointer to USART State Machine Structure */
	struct TRANSCEIVER_STATE_MACHINE* APIUSARTTSM;
	/* Called once per main iteration and before APIUSARTTxTasks  */ 
	void (*APIUSARTRxTasks)(void);
	/* Called once per main iteration and after APIUSARTRxTasks */ 
	void (*APIUSARTTxTasks)(void);
	/* Called when USART state is USART_SATE_IDLE, in other states no performance will be archived
	   The function will process an iteration of the USART State Machine.	*/
	void (*APIUSARTProcessIncommingPacketsStateMachine)(struct TRANSCEIVER_STATE_MACHINE*);
	/* Called when USART Hardware and Software states need to be configured and initialized. 
	   
	   Speed is determined by 
	   BaudRate = Fosc/(16*(1+BRG))
	   
	   where Fosc is current MCU clock and BRG is the argument 0-255 when calling this function 
	   
	   This function also resets the USART Communication State Machine to USART_STATE_IDLE.
	*/
	void (*APIUSARTInit)(unsigned short);
};	


/*
	USART MACHINE STATES
	
	USART_STATE_UDLE : This state stands for no packet transmission or reception is in progress.
					   When this state is asserted once, an invocation to APIUSARTProcessIncommingPacketsStateMachine is requiered.
					   
					   When transmission is timed out, this will be the default state. A packet retry might be needed.
					   To reset from an inconvenient state, call to APIUSARTInit again.
	USART_STATE_TRANSMITTNG:
	USART_STATE_RECEIVING:
						These states stand for packet transmission or reception is in progress.
						When any of these states is asserted, more invocations to API_USARTRxTasks and APIUSARTTxTasks are requiered.
						
						When packet transaction is complete, the machine will be set to USART_STATE_IDLE.
						
*/
#if !defined USART_STATE_IDLE
	#define USART_STATE_IDLE 0u
#endif
#if !defined USART_STATE_TRANSMITTING
	#define USART_STATE_TRANSMITTING 1u
#endif
#if !defined USART_STATE_RECEIVING
	#define USART_STATE_RECEIVING 2u
#endif

	
/* Detached is the state in which the device is not attached to the bus.  When
in the detached state a device should not have any pull-ups attached to either
the D+ or D- line.  This defintions is a return value of the 
variable APIUSBDeviceState() */
#define USB_DETACHED_STATE          0x00

/* Attached is the state in which the device is attached ot the bus but the
hub/port that it is attached to is not yet configured. This defintions is a 
return value of the function APIUSBDeviceState */
#define USB_ATTACHED_STATE          0x01

/* Powered is the state in which the device is attached to the bus and the 
hub/port that it is attached to is configured. This defintions is a return 
value of the variable APIUSBDeviceState */
#define USB_POWERED_STATE           0x02

/* Default state is the state after the device receives a RESET command from
the host. This defintions is a return value of the variable APIUSBGetDeviceState
 */
#define USB_DEFAULT_STATE           0x04

/* Address pending state is not an official state of the USB defined states.
This state is internally used to indicate that the device has received a 
SET_ADDRESS command but has not received the STATUS stage of the transfer yet.
The device is should not switch addresses until after the STATUS stage is
complete.  This defintions is a return value of the variable
APIUSBGetDeviceState*/
#define USB_ADR_PENDING_STATE       0x08

/* Address is the state in which the device has its own specific address on the
bus. This defintions is a return value of the varaible APIUSBGetDeviceState.*/
#define USB_ADDRESS_STATE           0x10

/* Configured is the state where the device has been fully enumerated and is
operating on the bus.  The device is now allowed to excute its application
specific tasks.  It is also allowed to increase its current consumption to the
value specified in the configuration descriptor of the current configuration.
This defintions is a return value of the variable APIUSBGetDeviceState. */
#define USB_CONFIGURED_STATE        0x20

/*
	ESS_QueryESSControl()			Macro

	Query @Ware ESS Current Status and Control Register value

	Returns an unsigned char with ESSCONTROL value
*/
#define ESS_QueryESSCONTROL() ESS_SystemCall(0,0)

/*
	ESS_WriteESSControl(_essctrl)		Macro

	_essctrl:unsigned char. New ESSCONTROL register value.

	Updates @Ware ESS Status and Control Register with _essctrl byte value
*/
#define ESS_WriteESSCONTROL(_essctrl) ESS_SystemCall(1,(unsigned short long)_essctrl)


/*
	ESS_Delay(ms)

	Invokes a software delay. The argument is measured in milliseconds @ 48MHz

	ms:unsigned short long. Delaying time in milliseconds
	
	Consider timing changes if MCU frequency is lower than 48MHz. 
*/
#define ESS_Delay(ms) ESS_SystemCall(2,(unsigned short long)(ms))

/*

	ESS_USBTasks(_ptrTSM)	Macro
	

	Performs a system call to @Ware ESS USB Driver.
	This is done to execute all communication transactions with a USB Host.
	
	_ptrTSM: struct TRANSCEIVER_STATE_MACHINE* . An optional pointer to a TSM
	If _ptrTSM==NULL, then the default @Ware ESS TSM is executed.
	 
	This allows user application to use @Ware ESS its default host commands 
	implementations.
*/
#define ESS_USBTasks(_ptrTSM) ESS_SystemCall(3,(unsigned short long)(_ptrTSM))

/*
	ESS__USBStatusLedTasks()	Macro
	Performs basic USB Status Led Notifications
*/
#define ESS_USBStatusLedTasks ESS_SystemCall(4,0)

/*
	ESS_USBSetOverlayCommandHandler(_pfnOverLayCommandHandler)
	
	Sets a new USB Overlay CommandHandler.
	
	_pfnOverlayCommandHandler:OverlayCommandHandler. A pointer to a user defined function. 
	See OverlayCommandHandler type in Command.h header file
	
	User application can use an overlay command handler to intercept and 
	execute custom host commands. The user appplication must provide
	the customized command handler function.
*/
#define ESS_USBSetOverlayCommandHandler(_pfnOverlayCommandHandler) ESS_SystemCall(5,(unsigned short long)(_pfnOverlayCommandHandler))

/*
	ESS_USBGetDeviceState()		Macro.
	
	Query USB Device State.
	
	Returns a value that reflects USB Device Status. See the meaning of
	these values in this file. USB_XXXXX state values.
*/
#define ESS_USBGetDeviceStatus() ESS_SystemCall(6,0)

/*
	ESS_ProcessIncommingPacketsStateMachine(_ptrTSM)	Macro
	
	Invokes @Ware ESS Packet Parser State Machine. User must provide State Machine Structure.

	_ptrTSM:struct TRANSCEIVER_STATE_MACHINE* . A pointer to TSM memory.	
*/
#define ESS_ProcessIncommingPacketsStateMachine(_ptrTSM) ESS_SystemCall(7,(unsigned short long)(_ptrTSM))

/*
	ESS_SetCPUAndPeripheralsClockSpeed(_cpuspeed)	Macro
	
	Updates and reads current clock speed status.
	
	_cpuspeed:unsigned char. One of these values
	Desired Freq, Value Provided
	"31250Hz"	,0x00
	"125KHz"	,0x10
	"250KHz"	,0x20
	"500KHz"	,0x30
	"1MHz"		,0x40
	"2MHz"		,0x50
	"4MHz"		,0x60
	"8MHz"		,0x70
	"48MHz"		,0x80
	"To Query"  ,0x0f   <----- No frequency change is done, only a query is perfomed.

	Returns a value that reflects current clocking speed. (one value from above)
*/
#define ESS_SetCPUAndPeripheralsClockSpeed(_cpuspeed) ESS_SystemCall(8,(unsigned short long)(_cpuspeed))

/*
	ESS_USARTInit(__brg)		Macro
	
	Invokes an initialization to @Ware ESS USART Driver.
	
	__brg:unsigned short. A value that establishes baud rate.
	
	USART Baudrate is defined by:
	
	Baudrate=Fosc/(4*(__brg+1))


*/
#define ESS_USARTInit(__brg) ESS_SystemCall(9,(unsigned short long)(__brg))

/*
	ESS_USARTTasks(_ptrTSM)		Macro
	
	Performs @Ware ESS USART Driver Tasks. Executes packets transactions.
	
	_ptrTSM:struct TRASCEIVER_STATE_MACHINE* . An optional pointer to TSM
	If _ptrTSM==NULL, default @Ware ESS State Machine is used.

*/
#define ESS_USARTTasks(_ptrTSM) ESS_SystemCall(10,(unsigned short long)(_ptrTSM))
/*

*/
#define ESS_ReadRAM(__addres)		ESS_SystemCall(11,(__address))
#define ESS_WriteRAM(__value_and_address) ESS_SystemCall(12,(__value_and_address))

/*

*/
#define ESS_USBHostWakeup()		ESS_SystemCall(13,0)

	
// If clock switching or other power savings measures were taken when
// executing the USBCBSuspend() function, now would be a good time to
// switch back to normal full power run mode conditions.  The host allows
// a few milliseconds of wakeup time, after which the device must be 
// fully back to normal, and capable of receiving and processing USB
// packets.  In order to do this, the USB module must receive proper
// clocking (IE: 48MHz clock must be available to SIE for full speed USB
// operation).
#define ESS_USBSetWakeFromSuspendHandler(__pfnHandler) ESS_SystemCall(14,(unsigned short long)(__pfnHandler))

//Example power saving code.  Insert appropriate code here for the desired
//application behavior.  If the microcontroller will be put to sleep, a
//process similar to that shown below may be used:

//ConfigureIOPinsForLowPower();
//SaveStateOfAllInterruptEnableBits();
//DisableAllInterruptEnableBits();
//EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
//Sleep();
//RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
//RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

//IMPORTANT NOTE: Do not clear the ACTVIF bit here.  This bit is 
//cleared inside @Ware ESS.  Clearing ACTVIF here will cause 
//things to not work as intended.	

#define ESS_USBSetSuspendHandler(__pfnHander) ESS_SystemCall(15,(unsigned short long)(__pfnHandler))


/*
	Macro helpers to API Access.
*/
#define ESS_GetUSBAPI ((ESS_GetAPI)(0))
#define ESS_GetUSARTAPI ((ESS_GetAPI)(1))
#endif
