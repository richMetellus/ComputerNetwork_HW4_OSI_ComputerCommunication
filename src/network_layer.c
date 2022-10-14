#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include "structs.h"

/*
 * The network layer accepts messages from the keyboard, wraps them into packets, 
 * and sends them to the data link layer. 
 * I use a top down approach starting from network layer to implement.
 */

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

/*the thread function to receive packets from the data link layer and display the messages*/
/**
 * 
 * @param threadsockfd : thread file descriptor of the socket to read from
 * @return nada
 */
void * rcvmsg (int threadsockfd)
{
/*add codes for local variables*/
    packet packet_from_dataLink;         // R the message is contained within that packet within that structure
    
    int charRead_written;                 // variable to serve as a checker for read or write on socket
    

	while (1)
	{
/*add codes to read a packet from threadsockfd and display it to the screen*/
            charRead_written = read(threadsockfd, &packet_from_dataLink, sizeof(packet));
            if(charRead_written < 0)
                error("network Layer: error reading from the socket\n");
            printf("\n---------------------------------------------------------------------\n");
            printf("\t \t \n Here is the message from host %s : %s \n\n\n",
                    packet_from_dataLink.nickname,packet_from_dataLink.message);

	}
	return NULL;    //--
}


int main(int argc, char *argv[])
{
    printf("\nEntering network layer main function.\n");
/*add codes for local variables*/
    // imported from client.c side  previous prog ;send request to datalink when datalink acting as a server.
   
    int netw_sockfd;            // the network socket
    int portno;                 // the port no where datalink is accepting incoming request. User provided.
    
    struct sockaddr_in serv_addr;  // this should hold info abt the data link server side; server to connect to
    
    /* hosten is defined in netdb.h. it defines a host computer on the interrnet
	store information about a given host, like name, ipv4 address. 
	only one copy of the hosten structure is allocated per thread. 
	application should copy the info it before using the sockets api
	i.e i thnk the sever labo2.cs.ndsu.edu and a port number.
	 have char *h_name (off name of the host) char **h_alliases alias list a NULL-terminated array of alternate names
	int h_addrtype host address type; type of address being returned.
	 h_length: lenth in byte of each address
     */
    struct hostent * server;
    
    char buffer[256];
    packet packet_to_dataLink;          // R packet to the dataLink
    


/*---check number of aruguments*/
    if (argc < 4) {
       fprintf(stderr,"usage %s data_addr data_port nickname\n", argv[0]);
       exit(0);
    }
    
/*add codes to connect to the data link layer*/
    
    // initialization of the server data to connect to. ip addr and port and protocol family used.
   
    server = gethostbyname(argv[1]);
    if(server == NULL)
    {
        fprintf(stderr, "Error, no such host \n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;             //ipv4 the type of family server can communicate with
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr,
            server->h_length);
    portno = atoi(argv[2]);
    serv_addr.sin_port = htons(portno);
    
    printf("Creating endpoint of network layer to connect to server data-link \n");
    
    netw_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //* since network layer is server as a client to connect to datalink here , no need for bind method.
    if(netw_sockfd <0)
        error("ERROR opening the network layer socket\n");
    
    
    printf("attempting to connect to the socket that is bound to the address server specified\n");
    int connectStatus;
    connectStatus = connect(netw_sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if(connectStatus < 0)
        error("Error connecting to data link layer \n");
    else
        printf(" bi-directional Connection established\n");
    
    
    
    
    

	/*create a thread to receive packets from the data link layer*/
	pthread_t pth;	//-- this is our thread identifier
	pthread_create(&pth,NULL,rcvmsg,netw_sockfd); //--
        
        printf("Please Enter a Message: "); //R

/* the main function will receive messages from keyboard and send packets to the data link layer*/
	while(1)
	{

         /*add codes to receive a message from keyboard, wrap it into a packet and send it to the data link layer*/
            
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);          // take keyboard input
            bzero((char*) &packet_to_dataLink, sizeof(packet));
            strcpy(packet_to_dataLink.message, buffer);
            strcpy(packet_to_dataLink.nickname, argv[3]);
            
            // write from the newly create packet to the file referred by the file descriptor new_sockfd
            // the network layer is writing to its own socket according to the picture the teacher drew in class.
            // the data link should read that same network socket to take that packet.
            int n = write(netw_sockfd, &packet_to_dataLink, sizeof(packet));
            if(n< 0)
                error("Error writing to socket newt_sockfd");

		/*--if the message is "EXIT"*/
		if (strcmp (buffer, "EXIT\n")==0)
		{
			pthread_cancel(pth); //kill the child thread
			close(netw_sockfd); // close socket
                        printf("\nExiting main fucntion of network layer\n");
			return 0;	//terminate
		}
	}

}
