
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "structs.h"
#include <stdbool.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/*Global variables that will be accessed in the thread function  */
int clientlist[2]; /* --the socket numbers through which the 2 clients (e.g. data link layer) are connected to this wire*/
pthread_t threadlist[2];




/*--the thread function that will receive frames from socket (i.e. data_link layer) and send the received frames to another socket*/
/* add codes to receive a frame from threadsocketfd */
void * onesocket ( int threadsockfd) // --
{
    /*add codes to declare local variables*/
    int charRead_written;
    frame frame_from_dataLayer;

    static bool otherHostExit = false;
    static bool secondHostExit = false;
    static frame ack_frame; // frame to include packet that other host has exited.
    static packet ack_packet; // packet that hold the message that the  other host has exited.
    
    
   
    
    printf("otherHostExit value %d, secondHostExit %d at start of new function call\n",
            otherHostExit, secondHostExit); 
    
    
    // thread to serve as a server to listen to either side of the 2 host.

    while (1) {


        /*if the message in the frame in EXIT close the socket and terminate this thread using
                           close(threadsockfd);
                           return NULL;
            other wise send the frame to the other socket (which is stored in clientlist) 
         */
        charRead_written = read(threadsockfd, &frame_from_dataLayer, sizeof (frame));
        if (charRead_written < 0)
            error("Error reading from socket");
        
        printf("\n----------------------------------------------------------------------------------------------\n");
        printf("Physical layer just get a packet from machine: %s \n", frame_from_dataLayer.my_packet.nickname);

        if (strcmp(frame_from_dataLayer.my_packet.message, "EXIT\n") == 0) 
        {
            close(threadsockfd); // will close the socket of the whichever host send the Exit message.
            //1st call  otherHostExit = false; so do not execute, the 2nd call, static value of otherHostExit = true
            // and the new frame is coming from available host message is true.
            if (otherHostExit && (strcmp(frame_from_dataLayer.my_packet.message, "EXIT\n") == 0)) {
                secondHostExit = true;
                return NULL;
            } 
            else {// 1st time only this get to be equal to true
                otherHostExit = true; // should use this to check if when the other host is writing a message to unavailable host/client.
                strcpy(ack_packet.nickname, frame_from_dataLayer.my_packet.nickname);
                strcpy(ack_packet.message, " The other Host is unavailable. You should send EXIT to terminate");
                ack_frame.seq_num = -1;
                ack_frame.type = 1 ;// acknowledgement
                ack_frame.my_packet = ack_packet;
                return NULL;
            }
        }
        
        // will not attempt to write to the unavailable host socket unless both are available.
        if(!otherHostExit && !secondHostExit )
        //else
        {
            
            printf("Sending the frame to the other host\n");
            printf("theadsockfd :%d, clientlist[0]: %d, clientlist[1]: %d",
                    threadsockfd, clientlist[0], clientlist[1]);
            //writing from clientlist[0] socket to clientlist[1] socket
            if (threadsockfd == clientlist[0]) {
                charRead_written = write(clientlist[1], &frame_from_dataLayer, sizeof (frame));
                if (charRead_written < 0) 
                    error("error witting to socket from clientlist[0]");
            }
            else if(threadsockfd == clientlist[1])
            {

                charRead_written = write(clientlist[0], &frame_from_dataLayer, sizeof (frame));
                if (charRead_written < 0)
                    error("error witting to socket of clientlist[0]");
                printf(" writing to socket file descriptor id : %d", clientlist[0]);
            }
        
        }
       // else send a default frame to the available host telling it the communication has ended.
        else
        {
            printf("Sending the frame to the same available host\n");
            printf("theadsockfd :%d, clientlist[0]: %d, clientlist[1]: %d",
                    threadsockfd, clientlist[0], clientlist[1]);
            //writing from clientlist[0] socket to clientlist[1] socket
            if (threadsockfd == clientlist[0]) {
                charRead_written = write(clientlist[0], &ack_frame, sizeof (frame));
                if (charRead_written < 0) 
                    error("error witting to socket");
            }
            else if(threadsockfd == clientlist[1])
            {

                charRead_written = write(clientlist[1], &ack_frame, sizeof (frame));
                if (charRead_written < 0)
                    error("error witting to socket");
                printf(" writing to socket file descriptor id : %d", clientlist[1]);
            }
        }

    }
}

int main(int argc, char *argv[])
{
    printf("Entering physical_layer main function\n");
    // adapted from assignment 3
    int sockfd, connectedSockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
   
    /*check the number of arguments*/
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

/*add codes to create a socket (sockfd), bind its address to it and listen to it*/
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno = atoi(argv[1]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno);
	 sockfd = socket(AF_INET, SOCK_STREAM, 0);
         if(sockfd < 0)
             error("ERROR opening socket");
         if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
             error("error on binding");

    for (int i = 0; i < 2; i = i + 1) /*--only accept two requests*/ 
    {
        listen(sockfd, 10);
        clilen = sizeof (cli_addr);
        // accept() system caall wake s u p the process when a connection with client has been successful establish.
        // return a file descriptor all communication on this connectin will be done susing the new file descriptor.
        // the second arg is a ref ptr to the address of the address of the cleint on the other end of the connection. 3rd arg size of this struc
        connectedSockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (connectedSockfd < 0)
            error("ERROR on accept");
        else
            printf("a connection to the wire has been accepted\n");



        /* --store the new socket into clientlist*/
        clientlist[i] = connectedSockfd;

        /*create a thread to take care of the new connection*/
        pthread_t pth; /* --this is the thread identifier*/
        pthread_create(&pth, NULL, onesocket, clientlist[i]); // --
        threadlist[i] = pth; /*save the thread identifier into an array*/
    }
    close(sockfd); /*--so that wire will not accept further connection request*/
    pthread_join(threadlist[0], NULL); //--
    pthread_join(threadlist[1], NULL); /*-- the main function will not terminated until both threads finished*/
    printf("\nExiting physical layer main function\n");
    return 0;

}
