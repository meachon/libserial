#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include "serial.h"

int Uart_Setopt(int sfd, int NBits, unsigned char NEvent, int NSpeed, int NStop)
{
	printf("Uart_Setopt() ------------------- \n");
	struct termios newtio;
	struct termios oldtio;

	if (tcgetattr(sfd, &oldtio) != 0)
	{
		LOG(ERROR_LEVEL, "SetupSerial failed!");
		return SERIAL_FAIL;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	switch (NBits)
	{
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch (NEvent)
	{
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E':
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'N':
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch (NSpeed)
	{
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 19200:
		cfsetispeed(&newtio, B19200);
		cfsetospeed(&newtio, B19200);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	case 460800:
		cfsetispeed(&newtio, B460800);
		cfsetospeed(&newtio, B460800);
		break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}

	if (NStop == 1)
	{
		newtio.c_cflag &= ~CSTOPB;
	}
	else if (NStop == 2)
	{
		newtio.c_cflag |= CSTOPB;
	}
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;

	tcflush(sfd, TCIOFLUSH);
	if ((tcsetattr(sfd, TCSANOW, &newtio)) != 0)
	{
		LOG(ERROR_LEVEL, "serial set error");
		return SERIAL_FAIL;
	}

	return SERIAL_SUCCESS;
}

int Uart_DevInit(int *sfd, const char *name, int NBits, char NEvent,
				 int NSpeed, int NStop)
{
	*sfd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
	if (*sfd == INVALL_DEV)
	{
		printf("open (%s) failed ------------------- \n", name);
		LOG(ERROR_LEVEL, "open tty fail");
		return SERIAL_FAIL;
	}
	//fcntl(*sfd, F_SETFL, 0); 
	int flags = fcntl(*sfd,F_GETFL,0);
    flags |= O_NONBLOCK;
	//flags &= ~O_NONBLOCK;
    fcntl(*sfd,F_SETFL,flags);

	if (Uart_Setopt(*sfd, NBits, NEvent, NSpeed, NStop) < 0)
	{
		LOG(ERROR_LEVEL, "set_opt error");
		return SERIAL_FAIL;
	}

	return SERIAL_SUCCESS;
}

int Uart_Read(int sfd, unsigned char *readb, int maxlen, int timeout_s)
{
	//LOG(DEBUG_LEVEL, "Uart_Read(sfd = %d, maxlen = %d, timeout_s = %d)", sfd, maxlen, timeout_s);
	int nread = 0;
	int len = 0;

	struct timeval start, stop, diff;

	gettimeofday(&start, 0);
	if (sfd == INVALL_DEV)
	{
		LOG(ERROR_LEVEL, "tty fail");
		return SERIAL_FAIL;
	}

	while (1)
	{
		gettimeofday(&stop, 0);
		tim_subtract(&diff, &start, &stop);
		if (diff.tv_sec * 1000000 + diff.tv_usec > 1000000 * timeout_s)
		{
			//LOG(DEBUG_LEVEL, "read timeout");
			return SERIAL_TIMEOUT;
		}

		nread = read(sfd, readb + len, maxlen - len);
		if (nread < 0)
		{
			//LOG(DEBUG_LEVEL, "device is error");
			return SERIAL_FAIL;
		}
		else
		{
			if (nread == (maxlen - len))
			{
				//LOG(INFO_LEVEL, "get all number:%d:%d:%d", maxlen, nread, len);
				return SERIAL_SUCCESS;
			}
			else if (nread == 0)
			{
				usleep(20000);
				continue;
			}
			else
			{
				len += nread;
				//LOG(INFO_LEVEL, "get %d left:%d:%d", len, maxlen - len, nread);
				continue;
			}
		}
	}

	return SERIAL_FAIL;
}

int Uart_DevRead(int sfd, unsigned char *readb, int maxlen, int timeout_s)
{
	LOG(DEBUG_LEVEL, "Uart_DevRead(sfd = %d, maxlen = %d, timeout_s = %d)", sfd, maxlen, timeout_s);
	int frame_len = 512;
	int frame_count = 0;
	int i = 0;
	int ret = 0;

	frame_count = maxlen / frame_len + 1;
	//LOG(DEBUG_LEVEL, "frame count: %d", frame_count);

	if (frame_count == 1)
	{
		ret = Uart_Read(sfd, readb, maxlen, timeout_s);
		if (SERIAL_SUCCESS != ret)
		{
			//LOG(DEBUG_LEVEL, "Uart_Read() return: %d", ret);
			return ret;
		}
		return SERIAL_SUCCESS;
	}

	int timeout = 0;

	for (i = 0; i < frame_count; i++)
	{
		if (i == (frame_count - 1))
		{
			ret = Uart_Read(sfd, readb + (frame_len * i), maxlen - frame_len * i, 2);
			if (SERIAL_SUCCESS != ret)
			{
				//LOG(DEBUG_LEVEL, "Uart_Read() return: %d", ret);
				return ret;
			}

			return SERIAL_SUCCESS;
		}

		if(i == 0)
		{
			timeout = timeout_s;
		}
		else 
		{
			timeout = 2;
		}

		ret = Uart_Read(sfd, readb + (frame_len * i), frame_len, timeout);
		if (SERIAL_SUCCESS != ret)
		{
			//LOG(DEBUG_LEVEL, "Uart_Read() return: %d", ret);
			return ret;
		}
	}

	return SERIAL_SUCCESS;
}

int Uart_DevClearReadBuff(int sfd)
{
	//LOG(DEBUG_LEVEL, "Uart_DevClearReadBuff(%d)", sfd);
	struct termios oldtio;

	if (tcgetattr(sfd, &oldtio) != 0)
	{
		//LOG(ERROR_LEVEL, "tcgetattr faield!");
		return SERIAL_FAIL;
	}

	tcflush(sfd, TCIFLUSH);
	if ((tcsetattr(sfd, TCSANOW, &oldtio)) != 0)
	{
		//LOG(ERROR_LEVEL, "tcsetattr faield!");
		return SERIAL_FAIL;
	}

	return SERIAL_SUCCESS;
}
int Uart_DevClearWriteBuff(int sfd)
{
	//LOG(DEBUG_LEVEL, "Uart_DevClearWriteBuff(%d)", sfd);
	struct termios oldtio;

	if (tcgetattr(sfd, &oldtio) != 0)
	{
		//LOG(ERROR_LEVEL, "tcgetattr faield!");
		return SERIAL_FAIL;
	}

	tcflush(sfd, TCOFLUSH);
	if ((tcsetattr(sfd, TCSANOW, &oldtio)) != 0)
	{
		//LOG(ERROR_LEVEL, "tcsetattr faield!");
		return SERIAL_FAIL;
	}

	return SERIAL_SUCCESS;
}

int Uart_DevClose(int sfd)
{
	LOG(DEBUG_LEVEL, "Uart_DevClose(%d)", sfd);
	if (sfd == INVALL_DEV)
	{
		LOG(ERROR_LEVEL, "DevClose a invall fd");
		return SERIAL_FAIL;
	}
	close(sfd);
	sfd = INVALL_DEV;
	return SERIAL_SUCCESS;
}

int Uart_DevWrite(int sfd, unsigned char *writeb, int maxlen)
{
	LOG(DEBUG_LEVEL, "Uart_DevWrite(sfd = %d, maxlen = %d)", sfd, maxlen);
	int sendlen = 0;
	int len = 0;

	if (sfd == INVALL_DEV)
	{
		LOG(ERROR_LEVEL, "DevWrite a invall fd");
		return SERIAL_FAIL;
	}
	tcflush(sfd,TCOFLUSH);
	while (1)
	{
		sendlen = write(sfd, writeb + len, maxlen - len);
		if (sendlen == -1)
		{
			return SERIAL_FAIL;
		}
		len += sendlen;
		if (len == maxlen)
		{
			break;
		}
	}
	return SERIAL_SUCCESS;
}

int tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	if (x->tv_sec > y->tv_sec)
	{
		return -1;
	}
	if ((x->tv_sec == y->tv_sec) && (x->tv_usec > y->tv_usec))
	{
		return -1;
	}
	result->tv_sec = (y->tv_sec - x->tv_sec);
	result->tv_usec = (y->tv_usec - x->tv_usec);
	if (result->tv_usec < 0)
	{
		result->tv_sec--;
		result->tv_usec += 1000000;
	}
	return 0;
}

void time_udelay(U32 us)
{
	struct timeval start, stop, diff;

	gettimeofday(&start, 0);
	while (1)
	{
		gettimeofday(&stop, 0);
		tim_subtract(&diff, &start, &stop);
		if (diff.tv_usec >= us)
		{
			return;
		}
	}
	return;
}
