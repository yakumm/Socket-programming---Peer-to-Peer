#include "header.h"

int main(int argc,char **argv)
{
    if(argc!=3){
        printf("<executable code> <Server IP Address> <Server Port number>");
        exit(1);
    }

    int Node_socket;
    int port_socket = atoi(argv[2]);
    char* server_ip = argv[1];
    int Size_rcv_message,Size_send_message;
    char* request = "request:node";
    int server_port_node; 

    //initializing buffer
    char bffr[1024];
    for(int i=0;i<sizeof(bffr);i++)
        bffr[i]='\0';

    if((Node_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error in socket()");
        exit(1);
    } 

    //intialising server socket
    struct sockaddr_in server_address;
    memset(&server_address, '\0', sizeof(server_address)); 
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_socket);

    //establishing of the connection
    if( connect(Node_socket, (struct sockaddr *)&server_address, sizeof(server_address)) >= 0){
       printf("Connection established to server %s:%d\n",server_ip,port_socket);
    }
    else{
        printf("Connection Failed");
        exit(1);
    }
    
    //message is sent
    if((Size_send_message=write(Node_socket,request,strlen(request))) >= 0){
        printf("saving details in server %s:%d\n",server_ip,port_socket);
    }
    else{
        printf("error in write");
        exit(1);
    }
    
    //message is recieved
    if((Size_rcv_message = read(Node_socket,bffr,sizeof(bffr) - 1)) < 0){
        printf("error in read");
        exit(1);
    }

    char* fld_reply = strtok(bffr,"\n");
    char* fld_stts = strtok(NULL,"\n");
    char* fld_port = strtok(NULL,"\n");
    if(strcmp(fld_reply,"reply:server") != 0 || strcmp(fld_stts,"status:connected")!=0){
        printf("Server reply: unexpected");
        exit(1);
    }
    else{
        fld_port = strtok(fld_port,":");
        fld_port = strtok(NULL,":");
        if(shutdown(Node_socket,0)>= 0){
            printf("Saved details in server %s:%d\n",server_ip,port_socket);
            printf("closed connection with server %s:%d\n",server_ip,port_socket ); 
        }
        else{
            printf("shutdown() error");
            exit(1);
        }
        int server_port_node = atoi(fld_port);
        startNodeServer(server_port_node);
    }
    return 0;
}
