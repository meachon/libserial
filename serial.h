#ifndef _SERIAL_H_
#define _SERIAL_H_

#define LOG(args...)

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#define INVALL_DEV		-1

#define SERIAL_SUCCESS		(0)
#define SERIAL_FAIL			 -1
#define SERIAL_WRONG			-2
#define SERIAL_TIMEOUT			-3

typedef unsigned long TMO;
typedef unsigned int OSAL_ID;
typedef int ER;
typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;


int Uart_Setopt(int sfd, int NBits, unsigned char NEvent,
		int NSpeed, int NStop);
int Uart_DevInit(int *sfd, const char *name, int NBits,
		 char NEvent, int NSpeed, int NStop);
int Uart_DevRead(int sfd, unsigned char *readb, int maxlen, int timeout_s);
int Uart_DevWrite(int sfd, unsigned char *writeb, int maxlen);
int Uart_DevClearReadBuff(int sfd);
int Uart_DevClearWriteBuff(int sfd);
int Uart_DevClose(int sfd);


int tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y);
void time_udelay(U32 us);

#define serial_ReadData        Uart_DevRead
#define serial_WriteData       Uart_DevWrite
#define serial_ClearReadBuff   Uart_DevClearReadBuff
#define serial_ClearWriteBuff  Uart_DevClearWriteBuff
#define serial_ClosePort       Uart_DevClose


#ifdef __cplusplus
}
#endif

#endif
