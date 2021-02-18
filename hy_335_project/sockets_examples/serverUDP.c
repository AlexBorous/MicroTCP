#include <unistd.h> 
#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <poll.h>
#define PORT 8080 
#define BUFF_SIZE 1024


int main(int argc, char const *argv[]) { 
    int sock, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 
    char *hello = "Hello from server"; 
       
    // Creating socket file descriptor 
    // AF_INET IPv4 internet protocols
    // AF_INET6 IPv6
    // SOCK_STREAM TCP
    // SOCK_DGRAM UDP
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0){ 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    printf("This is the server with UDP\n");

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    address.sin_port = htons( PORT ); 

    size_t len= sizeof(address);
    // Forcefully attaching socket to the port 8080 
    if (bind(sock, (struct sockaddr *)&address, sizeof(address))<0){ 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    valread=recvfrom(sock, (char *)buffer, BUFF_SIZE, MSG_WAITALL,
     (struct sockaddr *) &address, (socklen_t *) &len);
    buffer[valread]='\n';

    printf("%s\n",buffer ); 
    
    sendto(sock, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &address,  
            len);
    printf("Hello message sent\n"); 
    return 0; 
} 