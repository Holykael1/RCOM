#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1
#define PACKET_SIZE 2048
#define FLAG 0x7E

int RR_RECEIVED = FALSE;
int REJ_RECEIVED = FALSE;
int TIMEOUT = FALSE;

unsigned char C1 = 0x40;
volatile int STOP=FALSE;

FILE *fp;
int flag=0, conta=1;

/*int readREJ(int fd)
{
	unsigned char C = 0x01;
	int k = readSupervisionPacket(C,fd);
	if (k == 0)
	{
		REJ_RECEIVED = TRUE;
		printf("REJ received alright\n");
	}
	return k;
}

int readRR(int fd)
{
	unsigned char C = 0x05;
	int k = readSupervisionPacket(C,fd);
	if (k == 0)
	{
		RR_RECEIVED = TRUE;
		printf("RR received alright\n");
	}
	return k;
}*/


void switchC1()
{
	if (C1 == 0x00)
		C1 = 0x40;
	else
		C1 = 0x00;
}

void resetRRRejFlags()
{
	RR_RECEIVED = FALSE;
	REJ_RECEIVED = FALSE;
}

void endOfLL()
{
	conta = 0;
	STOP = FALSE;
	flag = 0;
}

void atende()
{
	printf("alarme #%d\n", conta);
	flag=1;
	conta++;
}

int readSupervisionPacket(unsigned char C, int fd)
{
	char buf[1];
	char sup[5] = {FLAG, 0x03, C, 0x03^C, 0x7E};
	int res;
	int counter = 0;
	int errorflag =0;

	while (STOP==FALSE && counter < 5)
	{
		read(fd,buf,1);

		switch(counter)
		{
			case 0:
			if(buf[0]!=sup[0])
				errorflag=-1;
			break;
			case 1:
			if(buf[0]!=sup[1])
				errorflag=-1;
			break;
			case 2:
			if(buf[0]!=sup[2])
				errorflag=-1;
			break;
			case 3:
			if(buf[0] != sup[3])
				errorflag=-1;
			break;
			case 4:
			if(buf[0]!=sup[4])
				errorflag=-1;
			break;
		};
		counter++;

		if (counter==5 && errorflag ==0)
		{
			STOP=TRUE;
			return 0;
		}
	}
	return -1;
}

int writeBytes(int fd)
{
	unsigned char ua[5] = {0x7E,0x03,0x07,0x01,0x7E};
	int res;

	res = write(fd,ua,5);
	printf("writeBytes: %d bytes written\n", res);
	return res;
}

void writeSet(int fd)
{
	srand(time(NULL));
	int random = rand()%3+1;
	int res;
	char set[5] = {0x7E,0x03,0x03,0x00,0x7E};
	char set3[5] = {0x7E,0x03,0x03,0x01,0x7E};

	switch(random)
	{
		case 1:
		printf("PRINTING RIGHT SET\n");
		res = write(fd,set,5);
		break;
		case 2:
		printf("PRINTING RIGHT SET\n");
		res = write(fd,set,5);
		break;
		case 3:
		printf("PRINTING WRONG SET\n");
		res = write(fd,set3,5);
		break;
		default:
		printf("?????\n");
		break;
	}

	printf("writeSet: %d bytes written\n", res);
}

int sendReadDISC(int fd,int toRead)
{
	if (toRead == TRUE)
	{
		unsigned char C = 0x0B;
		int k = readSupervisionPacket(C,fd);
		return k;
	}

	unsigned char disc[5];
	unsigned char A=0x03;
	unsigned char C = 0x0B;
	unsigned char BCC1 = A^C;
	disc[0] = FLAG;
	disc[1] = A;
	disc[2] = C;
	disc[3] = BCC1;
	disc[4] = FLAG;

	write(fd,disc,5);
	return 0;
}

int readUa(int fd)
{
	unsigned char C = 0x07;
	int k = readSupervisionPacket(C,fd);
	return k;
}
void printArray(char* arr,size_t length){

	int index;
	for( index = 0; index < length; index++){
			printf( "0x%X\n", (unsigned char)arr[index] );
	}
}

int detectRRorREJ(int fd)
{
	char buf[5];
	read(fd,buf,5);
	//printArray(buf,5);
	//Verifying starting flag
	if(buf[0] != 0x7E){
		printf("first byte isn't flag error \n");
		return -1;
	}

	//Verifying A
	if(buf[1] != 0x03){
		printf("read error in (A) \n");
		return -1;
	}

	//Verifying C1
	if(buf[2] == 0x01 ){
		if((buf[1]^buf[2]) != buf[3]){
				printf("A^C is not equal to BCC1 error");

		}
		printf("Received REJ with no errors\n",buf[2]);
		REJ_RECEIVED=TRUE;
		return 1;
	}

	if(buf[2] == 0x00 || buf[2]==0x40){
		if((buf[1]^buf[2]) != buf[3]){
				printf("A^C is not equal to BCC1 error");
		}
		printf("Received RR(%02x) with no errors\n",buf[2]);
		RR_RECEIVED=TRUE;
		switchC1();
		return 0;
	}
	return -1;
}

int sendInfoFile(int fd, unsigned char *buf, int size) //Handles the process of sending portions of the file to receiver
{
	int newSize = (size+6),i,j,res,k;
	unsigned char BCC2,BCC1;

	for (i = 0; i < size; i++)
	{
		if (buf[i] == 0x7E || buf[i] == 0x7D)
		{
			newSize++;
		}
	}

	//COMPUTING BCC2
	BCC2 = buf[0] ^ buf[1];
	for (i = 2; i < size;i++)
	{
		BCC2 = BCC2^buf[i];
	}

	unsigned char *dataPacket = (unsigned char*)malloc(newSize);

	dataPacket[0] = FLAG;
	dataPacket[1] = 0x03;
	dataPacket[2] = C1;
	BCC1 = dataPacket[1]^C1;
	dataPacket[3] = BCC1;

	j = 5;
	k = 4;
	for (i = 0; i < size;i++)
	{
		if (buf[i] == 0x7E)
		{
			dataPacket[k] = 0x7D;
			dataPacket[j] = 0x5E;
			k++;
			j++;
		}

		if (buf[i] == 0x7D)
		{
			dataPacket[k] = 0x7D;
			dataPacket[j] = 0x5D;
			k++;
			j++;
		}

		if(buf[i] != 0x7D && buf[i] != 0x7E)
			dataPacket[k] = buf[i];

		j++;
		k++;
	}

	dataPacket[newSize-2] = BCC2;
	dataPacket[newSize-1] = FLAG;

	res = write(fd,dataPacket,newSize);
	if (res == 0 || res == -1)
	{
		printf("sendInfoFile() - %d bytes written.\n",res);
		return res;
	}
	return res;
}

int getDataPacket(int fd) //Handles the process of dividing file into PACKET_SIZE bytes portions
{
	int i = 0, sizeDataPacket = 0;
	fp = fopen("pinguim.gif","rb"); //TODO: se quisermos receber nome de ficheiro ver isto
	int bytesRead = 0,res,read = 0;

	unsigned char *dataPacket = (unsigned char *)malloc(PACKET_SIZE);

	//READ PACKET_SIZE BYTES "AT A TIME"
	while ((bytesRead = fread(dataPacket, sizeof(unsigned char), PACKET_SIZE, fp)) > 0)
	{
		while (conta < 4)
		{
			res = sendInfoFile(fd,dataPacket,bytesRead);
			printf("res: %d\n",res);
			TIMEOUT = FALSE;
			alarm(3);

			if (res == -1)
			{
				while (!flag){}
				TIMEOUT = TRUE;
				continue;
			}
			else
			{
				while(!flag && (RR_RECEIVED == FALSE && REJ_RECEIVED == FALSE))
				{
					detectRRorREJ(fd);
					printf("inside wait while RR_RECEIVED:%d REJ_RECEIVED:%d\n",RR_RECEIVED,REJ_RECEIVED	);
				}
			}

			if (RR_RECEIVED)
			{
				conta = 0;
				alarm(0);
				printf("Inside RR if\n");
				resetRRRejFlags();
				break;
			}
			if (REJ_RECEIVED)
			{
				conta++;
				alarm(0);
				printf("Inside REJ if\n");
				resetRRRejFlags();
				continue;
			}
			printf("No RR or REJ\n");
			TIMEOUT = TRUE;
			resetRRRejFlags();
		}

		dataPacket = memset(dataPacket, 0, PACKET_SIZE);
		read += bytesRead;
	}
	fclose(fp);
	STOP = TRUE;
	return read;
}

unsigned char *buildStartPacket(int fd)
{
	int fsize, i=0, j=0;
	char *fileName = "pinguim.gif"; //TODO: se quisermos receber nome de ficheiro ver isto
	fseek(fp,0,SEEK_END);
	fsize = ftell(fp);
	fseek(fp,0,SEEK_SET);

	unsigned char A = 0x03;
	unsigned char C = 0x00;
	unsigned char BCC1 = A^C;
	unsigned char BCC2;

	fclose(fp);
	int startBufSize = 9+strlen(fileName);

	unsigned char *startBuf = (char *)malloc(startBufSize);

	startBuf[0] = 0x02;
	startBuf[1] = 0x00;
	startBuf[2] = 0x04;
	int* intloc= (int*)(&startBuf[3]);
	*intloc=fsize;
	startBuf[7] = 0x01;
	startBuf[8] = strlen(fileName);

	//INSERTS FILE NAME IN ARRAY
	for(i = 0; i < strlen(fileName);i++)
	{
		startBuf[i+9] = fileName[i];
	}

	//COMPUTE FINAL SIZE OF DATA ARRAY
	int sizeFinal = startBufSize+6;
	for (i = 0; i < startBufSize;i++)
	{
		if (startBuf[i] == 0x7E || startBuf[i] == 0x7D)
			sizeFinal++;
	}

	//COMPUTING BCC2
	BCC2 = startBuf[0]^startBuf[1];
	for (i = 2; i < startBufSize;i++)
	{
		BCC2 = BCC2^startBuf[i];
	}
	//printf("BCC2: %02X\n\n",BCC2);

	unsigned char *dataPackage;
	if (BCC2 == 0x7E)
	{
		dataPackage = (unsigned char *)malloc(sizeFinal+1);
		dataPackage[sizeFinal-3] = 0x7D;
		dataPackage[sizeFinal-2] = 0x5E;
	}

	if (BCC2 == 0x7D)
	{
		dataPackage = (unsigned char *)malloc(sizeFinal+1);
		dataPackage[sizeFinal-3] = 0x7D;
		dataPackage[sizeFinal-2] = 0x5D;
	}

	if (BCC2 != 0x7D && BCC2 != 0x7E)
	{
		 dataPackage = (unsigned char *)malloc(sizeFinal);
		 dataPackage[sizeFinal-2] = BCC2;
	}

	dataPackage[0] = FLAG;
	dataPackage[1] = A;
	dataPackage[2] = 0x00;
	BCC1 = A^dataPackage[2];
	dataPackage[3] = BCC1;

	j = 5;
	int k=4;
	for (i = 0; i < startBufSize;i++)
	{
		if (startBuf[i] == 0x7E)
		{
			dataPackage[k] = 0x7D;
			dataPackage[j] = 0x5E;
			k++;
			j++;
		}

		if (startBuf[i] == 0x7D)
		{
			dataPackage[k] = 0x7D;
			dataPackage[j] = 0x5D;
			k++;
			j++;
		}

		if(startBuf[i] != 0x7D && startBuf[i] != 0x7E)
			dataPackage[k] = startBuf[i];

		j++;
		k++;
	}
	dataPackage[sizeFinal-1] = FLAG;

	/*i = 0;
	for (; i < sizeFinal;i++)
	{
		printf("dataPackage[%d] = 0x%02X\n",i,dataPackage[i]);
	}*/

	int res;
	res = write(fd, dataPackage, sizeFinal);

	printf("%d bytes written\n",res);
	return 0;
}

int llwrite(int fd)
{
	int res;
	buildStartPacket(fd);
	res = getDataPacket(fd);
	endOfLL();
	return res;
}

int llopen(int fd)
{

	while(conta < 4)
	{
		writeSet(fd);
		alarm(3);

		while(!flag && STOP == FALSE)
		{
			readUa(fd);
		}
		if(STOP==TRUE)
		{
			alarm(0);
			endOfLL();
			return 0;
		}
		else
			flag=0;
	}
	return -1;
}

int llclose(int fd)
{
	while(conta < 4)
	{
		sendReadDISC(fd,FALSE);
		alarm(3);

		while(!flag && STOP == FALSE)
		{
			sendReadDISC(fd,TRUE);
		}

		if(STOP==TRUE)
		{
			alarm(0);
			endOfLL();
			return 0;
		}
		else
			flag=0;
	}
	writeBytes(fd);
	endOfLL();
	return -1;
}

int cycle(int fd)
{
	int fsize, finish = 0,res;
	fseek(fp,0,SEEK_END);
	fsize = ftell(fp);
	fseek(fp,0,SEEK_SET);

	while (finish == 0)
	{
		printf("Starting llopen()\n");
		res = llopen(fd);
		if (res == -1)
		{
			printf("Leaving application. llopen() failed\n");
			return -1;
		}
		res = llwrite(fd);
		if (res < fsize)
		{
			printf("DFGSDFGDFGSDFG\n");
			continue;
		}
		else
			finish = 1;
	}

	//uncomment this later
	/*res = llclose(fd);
	if (res == -1)
	{
		printf("Leaving application. llclose() failed\n");
		return -1;
	}*/

	return 0;
}

int main(int argc, char** argv)
{
	(void) signal(SIGALRM, atende);
	int fd;
	struct termios oldtio,newtio;

	if ( (argc < 2) ||
		((strcmp("/dev/ttyS0", argv[1])!=0) &&
			(strcmp("/dev/ttyS1", argv[1])!=0) ))
	{
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
	}

	fp = fopen("pinguim.gif","r"); //TODO: se quisermos receber nome de ficheiro ver isto

	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0)
	{
		perror(argv[1]); exit(-1);
	}

	if ( tcgetattr(fd,&oldtio) == -1)
	{
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 3;
	newtio.c_cc[VMIN]     = 0;

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1)
	{
		perror("tcsetattr");
		exit(-1);
	}
	cycle(fd);
	//llopen(fd);
	//buildStartPacket(fd);
	//getDataPacket(fd);
	//llwrite(fd);

	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}
	close(fd);
	return 0;
}
