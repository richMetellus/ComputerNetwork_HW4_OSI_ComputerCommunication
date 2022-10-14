/*
 * The data link layer accepts packets from the network layer, wraps them 
 * into frames, and sends them to the physical wire. It also receives frames 
 * from the physical wire, removes the frame headers, and passes the included
 *  packets to the network layer
 * 
 * The data link layer connects to the physical wire as a client. At the same 
 * time, it acts as a server to accept connection request from one network layer.
 * Usage intended: ./data_link_layer.exe [wire_addr] [Port no of physical wire] [port no set for data link]
 * ./data_link_layer.exe  lab02.cs.ndsu.nodak.edu wire_port data_port
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include "structs.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/*global variables to be used in the threads*/
int network_layersockfd;//--the socket through which the network layer is connected.
int wiresockfd; //--the socket through which the wire is connected.


/*--the thread function that receives frames from the wiresocket and sends packets to the network_layer */

// this function handle the upward move of my pdf diagram.
void * rcvfromwiresend2network_layer ( char *argv[] ) //--
{
	/*add codes to declare locals*/
    int charRead_written;                
    frame frame_from_phyLayer;      // initally empty to hold the frame after reading from wire 

	 while (1)
	 {
		 /*add codes receive a frame from wire*/
             charRead_written = read(wiresockfd, &frame_from_phyLayer, sizeof(frame));
             if(charRead_written < 0)
                 error("Error reading from physical layer socket \n");
            		 
		/*add codes to send the included packet to the network layer*/	
             printf("\n now will remove the frame header and transmit the included packet to network layer\n");
             charRead_written = write(network_layersockfd, &frame_from_phyLayer.my_packet, sizeof(packet));
             if(charRead_written < 0)
                 error("Error sending packet to network layer\n");
             else
                 printf("The packet is in route to network layer\n");
             printf("\n---------------------------------------------------------------------------------------------\n");
             printf("Recieved the frame from the physical layer\n");
             printf("Frame identifier\n");
             printf("\t Sequence Number: %d \n", frame_from_phyLayer.seq_num);
             printf("\t frame type: %d\n",frame_from_phyLayer.type);
             
	 }
    return NULL;
}

// the main handle the downward movement of the transmission. packet--> frame-->physical layer
int main(int argc, char *argv[])
{
/*add codes to declare local variables*/
    printf("\nEntering data link layer main\n");
    //variable for datalink as server
    int mainSockfd;         //  datalink as server socket to listen for network as client   
    int frameSeq = 0; 
    packet packet_from_NetwLayer; 
    socklen_t clilen;                  // to hold size of network layer( client)
    struct sockaddr_in cli_addr;        // struct to hold the client address (network layer) IP
    //struct sockaddr_in serv_addr;        // to hold the info of the type of server the data link layer is
    
    
    // information needed to facilitate datalink to act as client
    frame frame_to_phyLayer;       // frame to be sent to the physical layer
    int physical_layer_portno;              // physical layer port number as a server.
    struct sockaddr_in serv_addr;        // to hold the info of the server to connect to; use twice
    struct hostent *server;  // physical wire as server...remember to past same computer where physical wire is currently running
   
    

	/*--check number of arguments*/
     if (argc < 4) {
		 fprintf(stderr,"Usage: %s  wire__IP  wire_port data_port\n",argv[0] );
         exit(1);
     }

/* add codes to connect to the wire. Assign value to gobal varialbe wiresockfd */
   
   // ********initialization of physical wire server information ***************
    
    server = gethostbyname(argv[1]);            // lab02.cs.ndsu.nodak.edu
    if(server == NULL)
    {
        fprintf(stderr, "No such host can be found\n");
        exit(0);
    
    }
    physical_layer_portno = atoi(argv[2]);      // server port no [wire_port] from usage at running the program
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // hostent Addresses are returned in network byte order. The macro h_addr is defined to be h_addr_list[0] for compatibility with older software
    bcopy((char *) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(physical_layer_portno);
    
    //************************End of wire server info gathering*************************
    
    // serv_addr will be the physical wire as the server
    wiresockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(wiresockfd < 0)
        error("Error opening socket\n");
    // connect wiresockfd on the datalink side to the wire side.
    if(connect(wiresockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("Error connecting data link socket to the specified server (pysical-wire)\n");
    printf("connection to the wire successful\n");
        

/*generate a new thread to receive frames from the wire and pass packets to the network layer */
    
    pthread_t wirepth;	//--this is our thread identifier
    pthread_create(&wirepth,NULL,rcvfromwiresend2network_layer, NULL); //--
        

/*--add codes to create and listen to a socket that the network_layer will connect to. Assign value to global variable network_layersockfd*/
    
  //************************Beginning of datalink  acting as a server ******************************************************
    bzero((char *)&serv_addr, sizeof(serv_addr));
    int data_port = atoi(argv[3]);
    serv_addr.sin_port = htons(data_port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    
    mainSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(mainSockfd < 0)
        error("Datalink layer error opening the socket for listen\n");
    // bind assigned the address specified by serv_addr to the mainSockfd
     if(bind(mainSockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
             error("error on binding");
    listen(mainSockfd, 5);
    clilen = sizeof(cli_addr);
    
    // this will also fill the info from mainsocketfd and fill the cli_addr 
    // I think of socket as file containing info. so the netwlayer will use connect that will make a connection with the socket(mainSocfd) that this 
    // bound to the server Serv_addr here the datalink.
    // R extract the first connection request on the queue, and create new connected socket that will be used for handling the communication.
    network_layersockfd = accept(mainSockfd, (struct sockaddr *) &cli_addr, &clilen); 
    if(network_layersockfd < 0)
        error("Error on accept");
    printf("create new socket: %d \n", network_layersockfd);
    close(mainSockfd); //so that only 1 network layer can connect to it/ no more listening
    

	/*the main function will receive packets from the network layer and pass frames to wire*/
	 while (1)
	 {
	/*add codes to receive a packet from the network layer*/
             int n = read(network_layersockfd, &packet_from_NetwLayer, sizeof(packet));
             if(n < 0)
                 error("ERROR receiving packet from network layer");
             printf("-----------------------------------------------------\n");
             printf("Recieved a packet from network layer \n");
        /* add codes to wrap the packet into a frame*/	
             bzero((char *) &frame_to_phyLayer, sizeof(frame));
             frame_to_phyLayer.seq_num = frameSeq;
             frameSeq++;
             frame_to_phyLayer.type = 0;      // 0 for data, 1 for acknowlgement
             strcpy(frame_to_phyLayer.my_packet.nickname, packet_from_NetwLayer.nickname);
             strcpy(frame_to_phyLayer.my_packet.message, packet_from_NetwLayer.message);
		

        /*add codes to send the frame to the wire*/
             printf("Now new wrapped framed will be sent to the wire\n");
             // this write the frame to its own socket used to connect to the physical layer
             n = write(wiresockfd, &frame_to_phyLayer,sizeof(frame));
             if(n < 0)
                 error("Error sending a frame to the physical wire\n");
             else
                 printf("frame is written to own layer. Now physical layer will handle the read and send to other host\n");
             

/*if the message is "EXIT"  */
             if (strcmp(packet_from_NetwLayer.message, "EXIT\n") == 0) //--
            {
                pthread_cancel(wirepth); //--kill the child thread
                close(wiresockfd); //--
                close(network_layersockfd); //--close sockets
                printf("\nExiting data link main function\n");
                return 0; // --terminate the main function
            }

    }

}
