#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <poll.h>
#define PORT 8080 
#define BUFF_SIZE 1024
int main(int argc, char const *argv[]) 
{ 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    char *hello = "Hello from client"; 
    char buffer[BUFF_SIZE] = {0}; 
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){ 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 

    memset(&serv_addr, 0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
    serv_addr.sin_addr.s_addr=INADDR_ANY;
      
    printf("this is the client with UDP\n");

    sendto(sock, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &serv_addr,  
            sizeof(serv_addr)); 
    printf("Hello message sent\n"); 

    size_t len=sizeof(serv_addr);
    valread=recvfrom(sock, (char *)buffer, BUFF_SIZE, MSG_WAITALL,
     (struct sockaddr *) &serv_addr, (socklen_t *) &len);
    buffer[valread]='\n';
    printf("%s\n",buffer ); 
    return 0; 
}