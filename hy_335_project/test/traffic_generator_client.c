/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2017  Manolis Surligas <surligas@gmail.com>
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "../lib/microtcp.h"
#include "../utils/log.h"

static char running = 1;

static void
sig_handler(int signal)
{
  if(signal == SIGINT) {
    LOG_INFO("Stopping traffic generator client...");
    running = 0;
  }
}

int
main(int argc, char **argv) {
  struct timespec start_time;
  struct timespec end_time;
  ssize_t bytes_received=0, ret;
  uint16_t port=8080;
  microtcp_sock_t sock = microtcp_socket (AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = htonl(INADDR_ANY); 
  address.sin_port = htons( PORT ); 
  /*
   * Register a signal handler so we can terminate the client with
   * Ctrl+C
   */
  signal(SIGINT, sig_handler);
  uint8_t* buffer =malloc(MICROTCP_RECVBUF_LEN);
  LOG_INFO("Start receiving traffic from port %u", port);
  microtcp_connect(&sock,(const struct sockaddr *)&address,sizeof(address));
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
  while(running&&ret!=-1) {
    ret=microtcp_recv(&sock, buffer, MICROTCP_RECVBUF_LEN, MSG_WAITALL);
    bytes_received+=ret;
    
  }
   clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    microtcp_shutdown(&sock, 0);
   printf("Received %d bytes in %d ", bytes_received, end_time);
   free(buffer);
  /* Ctrl+C pressed! Store properly time measurements for plotting */
}



