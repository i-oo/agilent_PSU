/*OnyeBUCHI Onumonu
3rd attempt to communicate with E3646A through
Prologix GPIB-USB Controller
25/05/2019

USE \n AS EOI-EOS FOR PROLOGIX ONLY, E3646A DOESNT NEED THIS

DWORD = ULONG = unsigned long
typedef DWORD	*FT_HANDLE;
typedef ULONG	FT_STATUS;
LPVOID = void*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ftd2xx.h"

#define E3646A 10		// default GPIB address for E3646A

int baudRate = 9600;
char recv[256];				//global GPIB receive string
char buffer[32];


BOOLEAN TxGpib(FT_HANDLE handle, const char* buffer);	//WRITE TO GPIB
void Query(FT_HANDLE handle, char cmd[]);				//WRITE TO GPIB
FT_HANDLE Init(void);									//INITIALIZE GPIB



/*---------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
	if(argc != 2)	//Chech run syntax
    {
        printf("\nUsage: sudo ./devMain <Bias>\n");// <Current>\n");
        return 0;
    }

/********************************************************************************************/
/****************************************INITIALIZE CONNECTION*******************************/
/********************************************************************************************/

	system("echo 3c19.1 | sudo -S rmmod ftdi_sio");	//unmount ftdi module. Replace 3c19.1 with new password. This is important for first run after boot up.
/*******************************************************************************************/


	float bias = atof(argv[1]);		//Voltage bias applied
	//float Amps = atof(argv[2]);		//Voltage bias applied
	printf("\nVolt: %'.2f\n\n");// Current: %'.3f\n\n", bias, Amps);

	FT_HANDLE handle = Init();

	if (handle == 0){return 0;}

    // Controller mode
    char *cmd = "++mode 1\n";
    TxGpib(handle, cmd);

    // GPIB address of E3646A
    sprintf(buffer, "++addr %d\n", 5);
    TxGpib(handle, buffer);

    // No automatic read-after-write
    cmd = "++auto 0\n";
    TxGpib(handle, cmd);

    // No GPIB termination characters (LF,CF,ESC,+) appended
    cmd = "++eos 3\n";
    TxGpib(handle, cmd);

    // Assert EOI with last byte
    cmd = "++eoi 1\n";
    TxGpib(handle, cmd);

/*TALK TO DEVICE*/
    cmd = "Output on\n";
    TxGpib(handle, cmd);

    //cmd = "SOURce:VOLTage:LEVel:IMMediate:AMPLitude 3.00\n";
	sprintf(buffer, "SOURce:VOLTage:LEVel:IMMediate:AMPLitude %'.2f\n", bias);
    TxGpib(handle, buffer);


	//cmd = "SOURce:CURRent:LEVel:IMMediate:AMPLitude 1.000\n";
	//sprintf(buffer, "SOURce:CURRent:LEVel:IMMediate:AMPLitude %'.3f\n", Amps);
    //TxGpib(handle, buffer);



    FT_Close(handle);

    return 0;
}


/*------------------------WRITE TO GPIB--------------------------------------------------*/
BOOLEAN TxGpib(FT_HANDLE handle, const char* buffer)
{
    size_t len = strlen(buffer);
	unsigned long bytesWritten;

    FT_STATUS status = FT_Write(handle, (LPVOID)buffer, (unsigned long)len, &bytesWritten);

    if ((status != FT_OK)  || (bytesWritten != len))
        {
        printf("\nUnable to write '%s' to controller. Status: %d\r\n", buffer, status);
        return FALSE;
        }

	printf("\nWrote '%s' to controller. Status: %d\r\n", buffer, status);

	return TRUE;
}


/*------------------------READ FROM GPIB--------------------------------------------------*/
void Query(FT_HANDLE handle, char cmd[])
{ 
	DWORD length;
	char rxBuff[256];			

	TxGpib(handle, cmd);					
	TxGpib(handle, "++auto 1\n");					//make the PC listen, Agilent talk
	FT_Read(handle, &rxBuff, (DWORD)256, &length);	

	printf("%d bytes read\n\n", length);				//actual number of bytes read
	printf("%s\n", rxBuff);

	TxGpib(handle, "++auto 0\n");					//make the PC listen, Agilent talk

}


/*--------------------------Initialize GPIB------------------------------------------------*/
FT_HANDLE Init(void)
{
    FT_HANDLE handle = 0;
	FT_DEVICE_LIST_INFO_NODE *devInfo;
	DWORD numDevs;

    printf("Connecting to Prologix GPIB-USB Controller...\r\n");
    FT_STATUS status = FT_Open(0, &handle);
    if (status != FT_OK)
        {
		printf("FT_Open(%d) failed, with error %d.\n", 0, (int)status);
		printf("Use 'lsmod | grep ModuleName' to check if ftdi_sio (and usbserial) modules are present.\n");
		printf("If so, unload them using 'sudo rmmod ModuleName', as they conflict with ftd2xx.\n\n");
        return 0;
        }

    printf("Successfully connected.\r\n");

	status = FT_ResetDevice(handle);
	if (status != FT_OK) 
	{
		printf("Failure.  FT_ResetDevice returned %d.\n", (int)status);
	}
	
	status = FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX); // Purge both Rx and Tx buffers
	if (status != FT_OK) 
	{
		printf("Failure.  FT_Purge returned %d.\n", (int)status);
	}

	status = FT_SetBaudRate(handle, (ULONG)baudRate);
	if (status != FT_OK) 
	{
		printf("Failure.  FT_SetBaudRate(%d) returned %d.\n", 
		       baudRate,
		       (int)status);
	}
	
	status = FT_SetDataCharacteristics(handle, 
	                                     FT_BITS_8,
	                                     FT_STOP_BITS_1,
	                                     FT_PARITY_NONE);
	if (status != FT_OK) 
	{
		printf("Failure.  FT_SetDataCharacteristics returned %d.\n", (int)status);
	}
	                          
	// Indicate our presence to remote computer
	status = FT_SetDtr(handle);
	if (status != FT_OK) 
	{
		printf("Failure.  FT_SetDtr returned %d.\n", (int)status);
	}

	// Flow control is needed for higher baud rates
	status = FT_SetFlowControl(handle, FT_FLOW_RTS_CTS, 0, 0);
	if (status != FT_OK) 
	{
		printf("Failure.  FT_SetFlowControl returned %d.\n", (int)status);
	}

	// Assert Request-To-Send to prepare remote computer
	status = FT_SetRts(handle);
	if (status != FT_OK) 
	{
		printf("Failure.  FT_SetRts returned %d.\n", (int)status);
	}

	status = FT_SetTimeouts(handle, 7000, 7000);	// 7 seconds
	if (status != FT_OK) 
	{
		printf("Failure.  FT_SetTimeouts returned %d\n", (int)status);
	}

	return handle;
}

