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
 * You can use this file to write a test microTCP client.
 * This file is already inserted at the build system.
 */

#include "../lib/microtcp.h"

int
main(int argc, char **argv){

    printf("this is the client with UDP\n\n");
    //=============creation of socket=====================
    microtcp_sock_t sock = microtcp_socket (AF_INET, SOCK_DGRAM, 0);   
    //==========addr that the server is bound to============
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    address.sin_port = htons( PORT ); 
    char * hello = "hello from the client side";
    //================3 way handshake=====================
    microtcp_connect(&sock,(const struct sockaddr *)&address,sizeof(address));
    
    microtcp_send(&sock, hello, strlen(hello), 0);

    microtcp_shutdown(&sock, 0);
    
    
    return 0; 
}
