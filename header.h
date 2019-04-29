#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <signal.h>

#define MAX_WAIT 5


struct pkt_type
{   
    char* value;
    char* name;
};

int compare_string(char *first, char *second) {
   while (*first == *second) {
      if (*first == '\0' || *second == '\0')
         break;
         
      first++;
      second++;
   }
 
   if (*first == '\0' && *second == '\0')
      return 0;
   else
      return -1;
}

void swap(int *a, int *b)
{
   int t;
 
   t  = *b;
   *b = *a;
   *a = t;
}

char * fldval(struct pkt_type* pkt,char *name_fld){
    int n = atoi(pkt[0].value);
    while(n>0 && strcmp(pkt[--n].name,name_fld)!=0);
    if(n<=0){
        return NULL;
    }
    return pkt[n].value;
}

struct pkt_type * pkt_parser(char *bffr){
    struct pkt_type* pkt_flds = (struct pkt_type*)malloc(sizeof(struct pkt_type)*32);
    int flds_num = 1;
    char* temp = strtok(bffr,"\n");
    while(temp){
        pkt_flds[flds_num++].name = temp;
        temp = strtok(NULL,"\n");
    }

    char* flds_num_string = (char*)malloc(sizeof(char)*32);
    sprintf(flds_num_string,"%d",flds_num);

    pkt_flds[0].name = NULL;
    pkt_flds[0].value = flds_num_string;

    
    for(int i = 1;i<flds_num;i++){
        temp = strtok(pkt_flds[i].name,":");
        pkt_flds[i].name = temp;
        temp = strtok(NULL,":");
        pkt_flds[i].value = temp;
    }

    return pkt_flds;
}

void empty(char * x)
{
    int i;
    for(i=0;i<1024;i++)x[i]='\0';
}

int Send_Acknowledgement(int cl_sock,char *prefix){
    char bffr[1024];
    int recvMessageSize,sendMessageSize;
    if(strcmp(prefix,"client") == 0)    strcpy(bffr,"reply:client\nack:1");
    else    strcpy(bffr,"reply:node\nack:1");
    if((sendMessageSize = write(cl_sock,bffr,strlen(bffr))) < 0){
        if(strcmp(prefix,"node") == 0)
            printf("request write() ack error");
        else
            printf("write() ack send error");
        exit(1);
    }
}

int Acknowledgement_check(int cl_sock){
    char bffr[1024];
    int recvMessageSize;
   
    for(int i=0;i<sizeof(bffr);i++)
        bffr[i]='\0';
    recvMessageSize = read(cl_sock,bffr,sizeof(bffr) - 1);
    
    if(strcmp(fldval(pkt_parser(bffr),"ack"),"1") != 0){
        return -1;
    }
    return 0;
}

int match(char text[], char pattern[]) {
  int c, d, e, text_length, pattern_length, position = -1;
 
  text_length    = strlen(text);
  pattern_length = strlen(pattern);
 
  if (pattern_length > text_length) {
    return -1;
  }
 
  for (c = 0; c <= text_length - pattern_length; c++) {
    position = e = c;
 
    for (d = 0; d < pattern_length; d++) {
      if (pattern[d] == text[e]) {
        e++;
      }
      else {
        break;
      }
    }
    if (d == pattern_length) {
      return position;
    }
  }
 
  return -1;
}

int SaveFile(int cl_sock,const char * filename){
    int recvMessageSize, sendMessageSize;
    
    char reply[128];
    int fd;
    struct stat file_stat;
    if((fd = open(filename,O_RDONLY)) < 0){
        // printf("Error opening file");
        strcpy(reply,"reply:node\nfile:#####");
        sendMessageSize = write(cl_sock,reply,strlen(reply));
        
        if(Acknowledgement_check(cl_sock) < 0){
            printf("No ack recieved");
            exit(1);
        }
        return -1;
    }

    strcpy(reply,"reply:node\nfile:yes");
    sendMessageSize = write(cl_sock,reply,strlen(reply));
    
    if(Acknowledgement_check(cl_sock) < 0){
        printf("No ack recieved");
        exit(1);
    }


    if(fstat(fd,&file_stat)<0){
        printf("fstsat() error");
        exit(1);
    }
    char bffr[1024];
    sprintf(bffr,"reply:node\nfile:%s\nfilesize:%d",filename,(int)file_stat.st_size);

    sendMessageSize = write(cl_sock,bffr,strlen(bffr));
   
    if(Acknowledgement_check(cl_sock) < 0){
        printf("No ack recieved");
        exit(1);
    }
    
    off_t offset = 0,remain_data = file_stat.st_size;
    float z = remain_data/1024.0;
    printf("File info - Filename : %s, Size : %.3f KB\n",filename,z);

    while((sendMessageSize = sendfile(cl_sock,fd,&offset,BUFSIZ)) > 0 && remain_data){
        
        remain_data -= sendMessageSize;
        
        
        if(Acknowledgement_check(cl_sock) < 0){
            printf("No ack recieved");
            exit(1);
        }
    }
    return 0;
}


int SaveFile1(int cl_sock,char * filename){
    int recvMessageSize, sendMessageSize;
    
    int fd;
    struct stat file_stat;
    if((fd = open(filename,O_RDONLY)) < 0){
        return -1;
    }

    if(fstat(fd,&file_stat)<0){
        printf("fstsat() error");
        exit(1);
    }
    char bffr[1024];
    sprintf(bffr,"reply:server\nfile:%s\nfilesize:%d",filename,(int)file_stat.st_size);
    sendMessageSize = write(cl_sock,bffr,strlen(bffr));
   
     for(int i=0;i<sizeof(bffr);i++)
            bffr[i]='\0';
    recvMessageSize = read(cl_sock,bffr,sizeof(bffr) - 1);

   
    struct pkt_type* pkt = pkt_parser(bffr);
    int n = atoi(pkt[0].value);
    while(n>0 && strcmp(pkt[--n].name,"ack")!=0);

    if(n<=0){
        printf("No ack recieved");
        exit(1);
    }
    off_t offset = 0,remain_data = file_stat.st_size;

    while((sendMessageSize = sendfile(cl_sock,fd,&offset,BUFSIZ)) > 0 && remain_data){
        // printf("1. Server sent %d bytes from file's data and remaining data = %d\n", sendMessageSize, (int)remain_data);
        remain_data -= sendMessageSize;
        // printf("2. Server sent %d bytes from file's data and remaining data = %d\n", sendMessageSize, (int)remain_data);
        
        for(int i=0;i<sizeof(bffr);i++)
                bffr[i]='\0';
        recvMessageSize = read(cl_sock,bffr,sizeof(bffr) - 1);
       
        if(strcmp(fldval(pkt_parser(bffr),"ack"),"1") != 0){
            printf("No ack recieved");
            exit(1);
        }
    }
    return 0;
}

void printRandoms(int lower, int upper, int count) 
{ 
    int i; 
    for (i = 0; i < count; i++) { 
        int num = (rand() % 
           (upper - lower + 1)) + lower; 
        printf("%d ", num); 
    } 
} 

void servUtil(int cl_sock,char *client_IP,int client_Port){
    int recvMessageSize, sendMessageSize;
    char bffr[1024];
    for(int i=0;i<sizeof(bffr);i++)
                bffr[i]='\0';

    recvMessageSize = read(cl_sock,bffr,sizeof(bffr) - 1);
    
    if(strcmp(bffr,"request:client") != 0){
        printf("Unexpected request");
        exit(1);
    }
    Send_Acknowledgement(cl_sock,"node");

    for(int i=0;i<sizeof(bffr);i++)
                bffr[i]='\0';
    if(recvMessageSize = read(cl_sock,bffr,sizeof(bffr) - 1) < 0){
        printf("read() error");
        exit(1);
    }
    Send_Acknowledgement(cl_sock,"node");

    char* filefld;
    filefld = fldval(pkt_parser(bffr),"filename");
    printf("Client %s:%d fetch request for file '%s'\n",client_IP,client_Port, filefld);
    if(SaveFile(cl_sock,filefld) == 0){
        fprintf(stdout,"File '%s' sent\n", filefld);
    }
    else{
        fprintf(stdout,"File '%s' not found\n", filefld);
    }
}

void startNodeServer(int port){

    int s_port = port;
    int s_sock;

    if((s_sock=socket(AF_INET,SOCK_STREAM, 0)) < 0){
        printf("socket() failed");
        exit(1);
    }

    struct sockaddr_in srvrAddress;
    memset((void *)&srvrAddress,'\0',sizeof(srvrAddress));


    srvrAddress.sin_family = AF_INET;
    srvrAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    srvrAddress.sin_port = htons(s_port);

    bind(s_sock,(struct sockaddr*) &srvrAddress, sizeof(srvrAddress));
    
    listen(s_sock,MAX_WAIT);
    printf("Node server listening on port %d\n",s_port);

    while(1){

        int clntLen,cl_sock;
        struct sockaddr_in clntAddr;

        clntLen = sizeof(clntAddr);

        int client_Port;
        char client_IP[INET_ADDRSTRLEN];
        if((cl_sock=accept(s_sock,(struct sockaddr *)&clntAddr,&clntLen)) < 0){
            printf("accept() failed");
            exit(1);
        }
        else{

            for(int i=0;i<sizeof(client_IP);i++)
                client_IP[i]='\0';
            if(inet_ntop(AF_INET, &(clntAddr.sin_addr),client_IP,sizeof(client_IP)) == 0){
                printf("inet_ntop() error");
                exit(1);
            }
            client_Port = ntohs(clntAddr.sin_port);
            printf("Client %s:%d accepted\n", client_IP,client_Port);
        }

        int p_id = fork();

        if(p_id < 0){
            printf("fork() error");
            exit(1);
        }

        if(p_id == 0){
            close(s_sock);
            servUtil(cl_sock,client_IP,client_Port);
            exit(0);
        }
        else{
            close(cl_sock);
        }
    }
}