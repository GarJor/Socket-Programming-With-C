//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define TRUE   1
#define FALSE  0
#define PORT 5500
#define MAXCLI 10

int main(int argc , char *argv[]) {
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[MAXCLI] ,
          activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;

    char buffer[1025];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //a message
    char *message = "ECHO kaliTEST v1.0 \r\n";

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < MAXCLI; i++) {
        client_socket[i] = 0;
    }

    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    // if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
    //       sizeof(opt)) < 0 )
    // {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    //bind the socket to localhost port PORT
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, MAXCLI) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while(TRUE) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for ( i = 0 ; i < MAXCLI ; i++) {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if(sd > 0) FD_SET( sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd) max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR)) {
            printf("select error");
        }

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //add new socket to array of sockets
            for (i = 0; i < MAXCLI; i++) {
                //if position is empty
                if( client_socket[i] == 0 ) {
                    //inform user of socket number - used in send and receive commands
                    printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                    //adding to a vector
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    //send new connection greeting message
                    message = "TEST kali v1.0 \n";
                    if( send(new_socket, message, strlen(message), 0) != strlen(message) ) {
                        perror("send");
                    }

                    puts("Welcome message sent successfully");
                    break;
                }
                if(i==MAXCLI-1) {

                  printf("New socket connection (with fd %d) from %s will not be handled\n" , new_socket , inet_ntoa(address.sin_addr));
                  //send connection fail message
                  message = "ERROR bussy v1.0 \n";
                  if( send(new_socket, message, strlen(message), 0) != strlen(message) ) {
                      perror("send");
                  }

                  puts("Refuse message sent successfully");

                }
            }
        }

        //else its some IO operation on some other socket
        for (i = 0; i < MAXCLI; i++) {
            sd = client_socket[i];

            if (FD_ISSET( sd , &readfds)) {
                //Check if it was for closing , and also read the
                //incoming message
                if (/*(valread = read( sd , buffer, 1024))*/(valread=recv(sd,buffer,sizeof(buffer),0)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" ,
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }

                //Echo back the message that came in
                else {
                    //set the string terminating NULL byte on the end
                    //of the data read

                    buffer[valread] = '\0';
                    printf("CLIENT %d : %s \n", i ,buffer);
                    // send(sd , buffer , strlen(buffer) , 0 );
                    int ndest = buffer[0]-'0';
                    if(client_socket[ndest] == 0 ) send(sd , "ERROR\0" , strlen("ERROR\0") , 0 );
                    else {
                      char aux[1];
                      sprintf(aux,"%d",i);
                      buffer[0] = aux[0];
                      // memcpy( buffer, &buffer[2], strlen(buffer) );
                      send(client_socket[ndest] , buffer , strlen(buffer) , 0 );
                      send(sd , "SENT\0" , strlen("SENT\0") , 0 );

                    }

                    // }

                }

            }

        }

    }

    return 0;
}
