#include "header.h"

#define NODES_ARRAY "nodes_info.txt"

void service_sckt(FILE*,int,struct sockaddr_in *);

int main(int argc,char **argv){
	//taking input as <executable> <port>
	if(argc!=2){
		printf("Incorrect arguments");
		exit(1);
	}
	//1st argument taken as port
	int s_port = atoi(argv[1]);
	int s_sock;

	//tcp socket is created with ipv4 
	s_sock=socket(AF_INET,SOCK_STREAM, 0);

	// initialize socket struct
	struct sockaddr_in srvrAddress;
	srvrAddress.sin_family = AF_INET;
	srvrAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	memset((void *)&srvrAddress,'\0',sizeof(srvrAddress));
	srvrAddress.sin_port = htons(s_port);

	/*  bind() call used to bind socket struct with socket */
	bind(s_sock,(struct sockaddr*) &srvrAddress, sizeof(srvrAddress));
	
	/* after listening to client process will go into sleep mode 
	   and will wait for incoming requests */
	listen(s_sock,MAX_WAIT);
	printf("server running on port: %d\n", s_port);
	
	//file pointer for file where we will store the information of peer nodes
	FILE* file_pointer = fopen(NODES_ARRAY,"w");
	for(;;){

		int client_len,cl_sock;
		struct sockaddr_in client_address;

		client_len = sizeof(client_address);
		 // client request being accepted by relay server
		cl_sock=accept(s_sock,(struct sockaddr *)&client_address,&client_len);

		//multi threading is used to handle multiple connections
		int p_id = fork();
		
		if(p_id == 0){
			close(s_sock);
			service_sckt(file_pointer,cl_sock,&client_address);
			exit(0);
		}
		else{
			close(cl_sock);
		}
	}
	
	return 0;
}

void service_sckt(FILE* file_pointer,int cl_sock, struct sockaddr_in * client_address){
	int Size_rcv_msg, Size_snd_msg;
	char bffr[1000];
	for(int i=0;i<sizeof(bffr);i++)
			bffr[i]='\0';

	Size_rcv_msg = read(cl_sock,bffr,sizeof(bffr) - 1);

	//if the request is from a peer node
	if(strcmp(bffr,"request:node") == 0)
	{
			
		int nd_port = rand()%123 + ntohs(client_address->sin_port);

		//port number cannot exceed 65535
		if(nd_port > 65535)
		{
			nd_port = nd_port - 123;
		}
		char nodeIP[INET_ADDRSTRLEN];
		for(int i=0;i<sizeof(nodeIP);i++)
			nodeIP[i]='\0';

		if(inet_ntop(AF_INET, &(client_address->sin_addr),nodeIP,sizeof(nodeIP)) == 0){
			printf("inet_ntop() error");
			exit(1);
		}
		printf("Peer node %s:%d accepted\n",nodeIP,nd_port);

		//writing peer node info in file

		fprintf(file_pointer, "%s %d\n",nodeIP,nd_port);
		char* reply = "reply:server";
		char bffr[1024];
		for(int i=0;i<sizeof(bffr);i++)
			bffr[i]='\0';
		sprintf(bffr,"%s\nstatus:connected\nport:%d",reply,nd_port);

		int Size_snd_msg;
		Size_snd_msg = write(cl_sock,bffr,strlen(bffr));
	
	printf("Peer node %s:%d info stored on the server\n",nodeIP,nd_port);
	}
	//if the request is from the client
	else if(strcmp(bffr,"request:client") == 0)
	{
		int clientPort = ntohs(client_address->sin_port);
		char clientIP[INET_ADDRSTRLEN];
		for(int i=0;i<sizeof(clientIP);i++)
			clientIP[i]='\0';

		if(inet_ntop(AF_INET, &(client_address->sin_addr),clientIP,sizeof(clientIP)) == 0){
		exit(1);
	}
	printf("Client %s:%d accepted\n",clientIP,clientPort);

	char* reply = "reply:server";
	char bffr[1024];
	for(int i=0;i<sizeof(bffr);i++)
			bffr[i]='\0';

	sprintf(bffr,"%s\nstatus:connected\nport:%d",reply,clientPort);

	int Size_snd_msg,Size_rcv_msg;
	Size_snd_msg = write(cl_sock,bffr,strlen(bffr));
	
	for(int i=0;i<sizeof(bffr);i++)
			bffr[i]='\0';

	Size_rcv_msg = read(cl_sock,bffr,sizeof(bffr) - 1);

	struct pkt_type* pkt = pkt_parser(bffr);
	char * ack = fldval(pkt,"ack");

	if(strcmp(ack,"1")!=0)
{		printf("no ack recieved");
		exit(1);
	}
	else{
        if(SaveFile1(cl_sock,NODES_ARRAY) < 0){
            for(int i=0;i<sizeof(bffr);i++)
				bffr[i]='\0';
            sprintf(bffr,"sending peer nodes list to client failed %s:%d\n", clientIP,clientPort);
            printf("%s",bffr);
            exit(1);
        }
        else{
        	printf("sending peer nodes list to client %s:%d\n",clientIP,clientPort);
        }
    }
	}
	else{
		printf("request unknown %s\n", bffr);
	}
}

