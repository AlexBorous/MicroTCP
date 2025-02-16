#include <unistd.h> 
#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#define PORT 8080 
int main(int argc, char const *argv[]) { 
    int server_fd, new_socket, valread; 
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
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    address.sin_port = htons( PORT ); 
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (listen(server_fd, 1) < 0){ 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){ 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 

    valread = read( new_socket , buffer, 1024); 
    printf("%s\n",buffer ); 
    send(new_socket , hello , strlen(hello) , 0 ); 
    printf("Hello message sent\n"); 
    return 0; 
} 