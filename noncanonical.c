/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define ERR 0x36
#define ERR2 0x35


int status = TRUE;
volatile int STOP=FALSE;
volatile int STOP2=FALSE;
//TODO retirar por problemas de memória

int file_size=0;
char* file_name;

//Funções GUIA1
void writeBytes(int fd, char* message)
{
  	
	printf("SendBytes Initialized\n");
    int size=strlen(message);
	int sent = 0;

    while( (sent = write(fd,message,size+1)) < size ){
        size -= sent;
    }
	
}


char * readBytes(int fd)
{
   	char* collectedString=malloc (sizeof (char) * 255); 	
	char buf[2];    
	int counter=0,res=0;
    while (STOP==FALSE) {       /* loop for input */
      
	  res = read(fd,buf,1);   /* returns after 1 chars have been input */
      
	  if(res==-1)
		exit(-1);
	  buf[1]='\0';	
      collectedString[counter]=buf[0]; 
      
      if (buf[0]=='\0'){ 
	  collectedString[counter]=buf[0]; 
	  STOP=TRUE;
      }
      counter++;
    }
    printf("end result:%s\n",collectedString);
	return collectedString;
	
}

//FUNÇÕES GUIA 2
char readSupervision(int fd, int counter){

	char set[5]={0x7E,0x03,0x03,0x00,0x7E};
	char buf[1];    
	int res=0;
    	res = read(fd,buf,1);   /* returns after 1 chars have been input */
	
	if(res==-1){
	printf("read error\n");
	return ERR;
	}
	    
	switch(counter){
	case 0:
		if(buf[0]==set[0]){
			printf("SUCCESS\n");
			return 0x76;		
		}
		else printf("ERROR\n");
		return ERR;
	case 1:
		if(buf[0]==set[1]){
			return 0x03;		
		}
		else printf("ERROR\n");
		return ERR;
	case 2:
		if(buf[0]==set[2]){
			printf("SUCCESS\n");
			return 0x03;		
		}
		else printf("ERROR\n");
		return ERR;
	case 3:
		if(buf[0]==set[3]){
			printf("SUCCESS\n");
			return buf[0];		
		}
		else printf("ERROR\n");
		return ERR2;
	case 4:
		if(buf[0]==set[4]){
			printf("SUCCESS\n");
			return 0X7E;		
		}
		else printf("ERROR\n");
		return ERR;
 	default:
		return ERR;
	}	
}


void llopen(int fd, int type){
 char ua[5]={0x7E,0x03,0x03,0x01,0x7E};
 char readchar[2];
 int counter = 0;
 if(type==0){
	 while (STOP==FALSE) {       /* loop for input */
	 
	  readchar[0]=readSupervision(fd,counter);
	  printf("%c \n",readchar[0]);
	  readchar[1]='\0';	
	  
	  
	  counter++;

	  if(readchar[0]==ERR){
	  counter=0;
	  }
	   
	  if(readchar[0]==ERR2){
	  counter=-1;
	  }
	  
	  if (counter==5){ 
		 STOP=TRUE;
	  }
	  
	 } 
 }
	printf("Sending UA...\n");
    writeBytes(fd,ua);
}

void llread(int fd,char * packet){
	 char readchar[4];
	 int readStart = FALSE;

	 while (STOP2==FALSE) {       /* loop for input */
		 
	  readchar=readInfPackHeader(fd);
	  
	   if(readchar==ERR){
		//send erroR RR
		continue;
	  }
	   
	  if(readchar==ERR2){
		  //aqui chamou o llopen e mandou UA
		readStart=FALSE;
		continue;
	  }
	if(readStart!=TRUE){
	 //readStartPacketInfo(pr);	
	 if(readStartPacketInfo==0)
		 readStart=TRUE;
		//entrar loop de ler infpack, sem ler startpacketinfo
	}
    else //ler resto do ficheiro ficheiro	
	 
	
	 
	  printf("%c \n",readchar[0]);
	
	  
	  
	  counter++;

	 
	  
	  else(counter==5){ 
		 STOP=TRUE;
	  }
	  
	 } 
	
	
}

char destuffPack(int fd,char* buf,size_t length){
	//Buffer whose content will be destuffed
	//TODO make this more accessible to other functions	
	char dbuf[length];

	// counter for finding all bytes to destuff, starts on 4 because from 0 to 3 is the header
	//j is a counter to put bytes on dbuf;
	int i =4;
	int j =0;
	
	//Finding content that needs destuffing
	while(TRUE){
		if(buf[i] == 0x7E){
			
			break;
		}
		//TODO verificar se eu percebi o destuffing corretamente
		//counter measure de flags
		if(buf[i] == 0x7d){
			
			if(buf[i+1] == 0x5e){
				dbuf[j] = 0x7E;
				i = i+2;
				if(i >= strlen(buf)){
					printf("esse stuffing ta mal, oh boi");		
					return ERR;	
				}
				
			}
			else if(buf[i+1] == 0x5f){
				dbuf[j] = 0x7D;
				i = i+2;	
					if(i >= strlen(buf)){
					printf("esse stuffing ta mal, oh boi");		
					return ERR;	
				}				
			}
			else{
				//TODO mudar antes de entrega a mensagem de erro
				printf("esse stuffing ta mal, oh boi");		
				return ERR;	
			}
		}
		//no flags
		else{
		dbuf[j] = buf[i];
		i++;
		if(i >= strlen(buf)){
					printf("esse stuffing ta mal, oh boi");		
					return ERR;	
				}
		} 

	j++:
	 

	}
	

	return ERR;
	

}


char* readInfPackHeader(int fd, char* buf){

	//TODO Verificar se está tudo correto

	//Verifying that the header of the package is correct
	
	char c1alt;

	


	//Verifying starting flag
	if(buf[0] != 0x7E){
		printf("first byte isn't flag error \n");
		return ERR;
	}

	//Verifying A
	if(buf[1] != 0x03){
		printf("read error in (A) \n");
		return ERR;
	}

	//Verifying C1
	if(buf[2] != 0x00 && buf[2] != 0x40){
		printf("read error in (C)");
		return ERR;
	}
	else if(buf[2]== 0x03){
		if(buf[3]==0x00){
			llopen(fd,1);
			return ERR2;	
		}
		
		else{ 
			printf("Invalid information packet\n");
		}
		
	}
	//Verifying BCC1
	if(buf[1]^buf[2] != buf[3]){
		printf("A^C is not equal to BCC1 error");
		return ERR;

	}
	
	//Alternating C1
	if(buf[2] == 0x00){
		c1alt = 0x40;
	}
	else if(buf[2] == 0x40){
		c1alt = 0x00;
	}
	//criating header of start package to send
	char* rr = [0x7E,0x03,c1alt,0x03^c1alt];
	
	return rr;
}

void readStartPacketInfo(char * startPacket){
	//recebe trama start sem header
	//destuff de startPacket
	//ler o tamanho do ficheiro e guardar em variavel global file_size(int)
	//ler o nome do ficheiro e guardar em variavel global file_name
	//se tudo bem sucedido, retornar sucesso e llread começa a ler as tramas de ficheiro

}

char sendRR(int fd,char* rr){
	
	
	//TODO testing required I need clarification on send
	printf("SendBytes Initialized\n");
    int size=strlen(rr);
	int sent = 0;

    while( (sent = write(fd,rr,size+1)) < size ){
        size -= sent;
    }
	
}









int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio,newtio;

 

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(-1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
	llopen(fd,0);
 	

    sleep(2);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
