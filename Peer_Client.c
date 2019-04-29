#include "header.h"



struct Connected_Nodes{
    char address[64];
    int port;
};

int Get_File(int cl_sock,char * filename){
    char bffr[1024];
    int Recv_Msg_Size,Send_Msg_Size;
    Recv_Msg_Size = read(cl_sock,bffr,sizeof(bffr) - 1)  ; 
    Send_Acknowledgement(cl_sock,"client");
    FILE* filepointer = fopen(filename,"w");
    struct pkt_type* pkt = pkt_parser(bffr);
    int bffr_size = atoi(fldval(pkt,"filesize"));
    int remained_data = bffr_size;


    for(int i=0;i<sizeof(bffr);i++)
            bffr[i]='\0';
    while((Recv_Msg_Size = read(cl_sock,bffr,sizeof(bffr) - 1)) > 0 && remained_data>0){
        fprintf(filepointer, "%s", bffr);
        remained_data -= Recv_Msg_Size;
        memset(bffr,'\0',sizeof(bffr));
        Send_Acknowledgement(cl_sock,"client");
    }
    fclose(filepointer);
    return 0;
}

struct Connected_Nodes* get_nodes_list(int cl_sock){
    int Recv_Msg_Size,Send_Msg_Size,count = 1;
    struct Connected_Nodes* nodes_list = (struct Connected_Nodes*)malloc(sizeof(struct Connected_Nodes)*128);

    int x1 = Get_File(cl_sock,"temp.txt") ; 
    FILE* filepointer = fopen("temp.txt","r");


    char address[64],port[16],check;
    check = getc(filepointer);
    while(check!=EOF){
        ungetc(check,filepointer);
        fscanf(filepointer,"%s",address);
        fscanf(filepointer,"%s",port);
        check = getc(filepointer);
        strcpy(nodes_list[count].address,address);
        nodes_list[count].port = atoi(port);
        count = count + 1 ; 
    }
    remove("temp.txt");
    memset(nodes_list[0].address,'\0',sizeof(nodes_list[0].address));
    nodes_list[0].port = count;
    return nodes_list;
}

int Get_File_(int cl_sock,char * filename){ 
    int Recv_Msg_Size,Send_Msg_Size;
    char bffr[1024];
    Recv_Msg_Size = read(cl_sock,bffr,sizeof(bffr) - 1) ;
    Send_Acknowledgement(cl_sock,"client");
    FILE* filepointer = fopen(filename,"w");
    struct pkt_type* pkt = pkt_parser(bffr);
    int bffr_size = atoi(fldval(pkt,"filesize"));
    int remained_data = bffr_size;
    memset(bffr,'\0',sizeof(bffr));
    while((Recv_Msg_Size = read(cl_sock,bffr,sizeof(bffr) - 1)) > 0 && remained_data>0){
        fprintf(filepointer, "%s", bffr);
        remained_data = remained_data - Recv_Msg_Size;
        memset(bffr,'\0',sizeof(bffr));
        Send_Acknowledgement(cl_sock,"client");
    }
    fclose(filepointer);
    return 0;
}
void utility_fn_cli(struct Connected_Nodes*);
int connect_to_node(char *,int,char*);

int main(int argc,char** argv)
{   
    int cl_sock;
    // input argument <exec> <address> <port>

    if(argc!=3){
        printf("incorrect arguments");
        exit(1);
    }
    int s_port = atoi(argv[2]);
    char* IP_serv = argv[1];
    cl_sock = socket(AF_INET, SOCK_STREAM, 0)  ; 
    struct sockaddr_in srvrAddress;
    memset(&srvrAddress, '\0', sizeof(srvrAddress)); 

    srvrAddress.sin_family = AF_INET;
    srvrAddress.sin_port = htons(s_port);
    srvrAddress.sin_addr.s_addr = inet_addr(IP_serv);

    int x2 = connect(cl_sock, (struct sockaddr *)&srvrAddress, sizeof(srvrAddress))  ; 
    printf("Connected to server with address %s\n",IP_serv) ; 

    char bffr[1024];
    int Recv_Msg_Size,Send_Msg_Size;
    
    strcpy(bffr,"request:client");

    Send_Msg_Size = write(cl_sock,bffr,strlen(bffr)) ; 
    
    // buffer initailized empty
    for(int i=0;i<sizeof(bffr);i++)
            bffr[i]='\0';


    Recv_Msg_Size = read(cl_sock,bffr,sizeof(bffr) - 1) ; 
    
    Send_Acknowledgement(cl_sock,"client");

    struct pkt_type *pkt = pkt_parser(bffr);
    printf("Nodes list collected from server\n");

    if(strcmp(fldval(pkt,"status"),"connected")!=0){
        printf("incorrected reply");
        exit(1);
    }
    
    struct Connected_Nodes * nodes_list = get_nodes_list(cl_sock);
    
    int x3 = shutdown(cl_sock,0)< 0 ;
    printf("server connection closed\n") ; 
    char chr[5] = "y";

    for(;;){
        if(strcmp("N",chr)==0 || strcmp("n",chr)==0){
            break ; 
        }
        else if(strcmp("Y",chr)==0 || strcmp("y",chr)==0){    
            utility_fn_cli(nodes_list);
        }
        printf("Quit:(N\\n)  or Continue:(Y\\y)\n");
        // loop again in case of yes input
        scanf("%s",chr);
    }

    return 0;
}

void utility_fn_cli(struct Connected_Nodes * nodes_list){

    int num_nodes,count = 0 ,i=1; 
    num_nodes=nodes_list[0].port;
    char f_name[64];
    printf("Name of file to download : ");
    scanf("%s",f_name);

    for(i = 1;i<num_nodes-1;i++){
        if(connect_to_node(nodes_list[i].address,nodes_list[i].port,f_name) == 0){
            count = 1;
            printf("file found on %s:%d\n",nodes_list[i].address,nodes_list[i].port);
            break;
        }
    }
    if(count == 0){
        printf("file not found\n");
    }
}

int connect_to_node(char* IP_serv,int s_port,char* filename){
    
    int cl_sock,x5,x6,x7;
    cl_sock = socket(AF_INET, SOCK_STREAM, 0) ;
    struct sockaddr_in srvrAddress;
    memset(&srvrAddress, '\0', sizeof(srvrAddress)); 
    // SOCKET FAMILY
    srvrAddress.sin_family = AF_INET;
    srvrAddress.sin_port = htons(s_port);
    srvrAddress.sin_addr.s_addr = inet_addr(IP_serv);
    // socket FAMILY
    x5 = connect(cl_sock, (struct sockaddr *)&srvrAddress, sizeof(srvrAddress))  ;
    printf("Connected to peer %s:%d\n",IP_serv,s_port);
    char bffr[1024];
    int Recv_Msg_Size,Send_Msg_Size;
    
    strcpy(bffr,"request:client");

    Send_Msg_Size = write(cl_sock,bffr,strlen(bffr)) ; 
    x6 = Acknowledgement_check(cl_sock) ; 

    sprintf(bffr,"request:client\nfilename:%s",filename);
    Send_Msg_Size = write(cl_sock,bffr,strlen(bffr)) ; 
    x7 = Acknowledgement_check(cl_sock) ; 

    for(int i=0;i<sizeof(bffr);i++) 
            bffr[i]='\0';


    Recv_Msg_Size = read(cl_sock,bffr,sizeof(bffr) - 1) ; 
    Send_Acknowledgement(cl_sock,"client");

    if(strcmp(fldval(pkt_parser(bffr),"file"),"yes") ==0){
        if(Get_File_(cl_sock,"temp.txt") < 0){
            close(cl_sock);
            return -1;
        }
        FILE* filepointer1 = fopen("temp.txt","r");
        FILE* filepointer2 = fopen("download.txt","w");
        // printf("File '%s' found on peer %s:%d\n",filename,IP_serv,s_port);
        char check;
        check = getc(filepointer1);
         while(check!=EOF){
            ungetc(check,filepointer1);
            memset(bffr,'\0',sizeof(bffr));
            fgets(bffr,sizeof(bffr)-1,filepointer1);
            printf("%s", bffr);
            fprintf(filepointer2,"%s\n",bffr);
            check = getc(filepointer1);
        }
      
        printf("\n");
        fclose(filepointer1);
        fclose(filepointer2);
        printf("File downloaded in %s\n", "download.txt");
        remove("temp.txt");
        printf("Connection with %s:%d closing\n", IP_serv,s_port);
        shutdown(cl_sock,0);
        return 0;
    }
    else{
        // printf("File '%s' not found on peer %s:%d\n",filename,IP_serv,s_port);
        printf("Connection with  %s:%d closed\n", IP_serv,s_port);
        shutdown(cl_sock,0);
        return -1;
    }   
}