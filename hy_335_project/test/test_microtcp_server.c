/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * You can use this file to write a test microTCP server.
 * This file is already inserted at the build system.
 */


#include "../lib/microtcp.h"

int
main(int argc, char **argv){
    //=============socket creation============
    char *hello = "Hello from the server side"; 
    
    microtcp_sock_t sock = microtcp_socket (AF_INET, SOCK_DGRAM, 0);
    
    printf("this is the server with UDP\n\n");
    //=====the address I'll bind the server=== 
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    address.sin_port = htons( PORT ); 
    //=========bind the server to address======
    if(microtcp_bind(&sock, (const struct sockaddr *)&address,(socklen_t) sizeof(address))<0){ 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    //=======accept connection requested from client========
    microtcp_accept (&sock, (struct sockaddr*)&(sock.peer_addr), sizeof(sock.peer_addr));
    //====================shut down==========================
    uint8_t *retbuf=malloc(MICROTCP_RECVBUF_LEN); 
    //=========receive message from client=======
    while(microtcp_recv (&sock,retbuf, MICROTCP_RECVBUF_LEN, MSG_WAITALL)!=-1){
        printf("Client send:%s with seq:%u\n",(char *)(retbuf),sock.header.seq_number);
    }

    microtcp_shutdown(&sock,1);


    return 0;
}
