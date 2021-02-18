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

#include "microtcp.h"
#include "../utils/crc32.h"
//================================
//for testing purposes I define a 
//default port we will have to ask 
// how to handle the port-address thing 
//cause I don't understand 
//and there are somethings wrong in the 
//code that will be fixed later (o voithos to eipe)
#define PORT 8080

//=====================================
//=I=I=I=I=I=STATIC FUNCTIONS=I=I=I=I=I=I=

/**
 * Compress the packet 
 * 
 *@param data the data segment of the packet
 *@param header header segment of the packet
 *@return packet as a void pointer
 */
 static void * serialize(char *data,microtcp_header_t header, char* buffer);


 /**
 * Decompress the packet to header and data elements
 * 
 *@param data the data segment of the packet
 *@param header header segment of the packet
 *@return packet as a void pointer 
 */
 static char * deserialize(void *buffer, microtcp_header_t *header, char* data);

#ifdef CHECKSUM_ERROR
  int crc_flag=1;
#endif

#ifdef TIMEOUT
  int timeout_var=1;
#endif

static microtcp_header_t header_init(){
  microtcp_header_t header;
  header.ack_number = 0;
  header.seq_number =0;
  header.control=0;
  header.data_len = 0;
  header.future_use0 = 0;
  header.future_use1=0;
  header.future_use2 = 0;
  header.window = 0;
  header.checksum = 0;
  return header;
} 

#ifdef DEBUG
static void print_header(microtcp_header_t *header){
  printf("header:\n");
  printf("ack_number: %d\n", header->ack_number);
  printf("seq_number: %d\n", header->seq_number);
  printf("control: %d\n", header->control);
  printf("data_len: %d\n",  header->data_len);
  printf("future_use0: %d\n", header->future_use0);
  printf("future_use1: %d\n", header->future_use1);
  printf("future_use2: %d\n", header->future_use2);
  printf("window: %d\n", header->window);
  printf("checksum: %d\n", header->checksum);
}
#endif

static int send__(microtcp_sock_t *socket, void *buffer, size_t buffer_len, int flags, const struct sockaddr *address, socklen_t address_len){
  socket->header.checksum=0;
  socket->bytes_send=socket->bytes_send+sizeof(buffer);
  socket->header.data_len=buffer_len;
  char* serialized_buffer= malloc(MICROTCP_RECVBUF_LEN);
  serialized_buffer=serialize(buffer,socket->header, serialized_buffer);
  //======calculating and passing crc32 to header=======================
  socket->header.checksum=crc32(serialized_buffer, strlen(serialized_buffer));
  serialized_buffer=serialize(buffer,socket->header, serialized_buffer);
  int ret_value=sendto(socket->sd, serialized_buffer, sizeof(microtcp_header_t) + buffer_len, flags, (const struct sockaddr *) address, address_len);
  if(ret_value ==-1){
      perror("Sending packet");
      free(serialized_buffer);
      return -1;
  }
  free(serialized_buffer);
  return ret_value-sizeof(microtcp_header_t);
}

static int receive__(microtcp_sock_t *socket, void *buffer, size_t buffer_len, int flags, const struct sockaddr *address){
  
  memset(buffer,0,MICROTCP_RECVBUF_LEN);
  size_t len=sizeof(socket->peer_addr);
  
  int ret_value=recvfrom(socket->sd, buffer, MICROTCP_RECVBUF_LEN, flags, (struct sockaddr *) address, (socklen_t*)&len);
  
  if(ret_value<0){
       perror("Recieving packet\n");
       return -1;
  } 
  //=======checksum check=============================
  
  char *data =malloc(MICROTCP_RECVBUF_LEN);
  data=deserialize(buffer, &socket->header, data);
  uint32_t crc=socket->header.checksum;
  socket->header.checksum=0;
  char* seriazedbuffer=malloc(MICROTCP_RECVBUF_LEN);
  seriazedbuffer=serialize(buffer, socket->header, seriazedbuffer);
  if(crc32(seriazedbuffer, strlen(seriazedbuffer))!=crc){
    free(seriazedbuffer);
    free(data);
    return -2;
  }
  data = deserialize(buffer, &socket->header, data);
  memcpy(buffer, data, socket->header.data_len);
  socket->bytes_received=socket->header.data_len;
  socket->peer_win_size=socket->header.window;
  free(seriazedbuffer);
  free(data);
  return ret_value;
}


microtcp_sock_t microtcp_socket (int domain, int type, int protocol)
{
  srand(time(NULL));
  //=======socket initialisation===============
  microtcp_sock_t new_socket;
  int sock; 

  if((sock=socket(domain, type, protocol))==-1){
    perror("OPENING UDP SOCKET");
    exit(EXIT_FAILURE);
  }
  /*
  * Initializing socket's fields
  */
  memset(&new_socket, 0, sizeof(microtcp_sock_t));
  new_socket.sd=sock;
  new_socket.state=UNKNOWN;
  new_socket.cwnd=MICROTCP_INIT_CWND;
  new_socket.ssthresh=MICROTCP_INIT_SSTHRESH;
  new_socket.recvbuf= malloc(MICROTCP_RECVBUF_LEN);
  new_socket.header=header_init();
  //==============window==============
  new_socket.curr_win_size=MICROTCP_WIN_SIZE;
  new_socket.init_win_size=MICROTCP_WIN_SIZE;

  new_socket.seq_number=rand(); 

   //=========timer=====
  /*struct timeval timeout ;
  timeout . tv_sec = 0;
  timeout . tv_usec = MICROTCP_ACK_TIMEOUT_US ;
  setsockopt (sock , SOL_SOCKET ,
  SO_RCVTIMEO , &timeout ,
  sizeof ( struct timeval )) < 0;*/

  return new_socket;  
  //============================================
}

int
microtcp_bind (microtcp_sock_t *socket, const struct sockaddr *address,
               socklen_t address_len){
  return bind(socket->sd, (struct sockaddr *)address, sizeof(*address));

}

int 
microtcp_connect (microtcp_sock_t *socket, const struct sockaddr *address,
                  socklen_t address_len){   
  
  //==============copy the peer address to struct==================
  memcpy(&(socket->peer_addr), address, address_len);      
  //=============send first message of handshake============
  socket->header.control=10; 
  //=====setting control to ACK SYN=========
  socket->header.seq_number=socket->seq_number;
  socket->header.window=MICROTCP_WIN_SIZE;
  send__(socket, NULL, 0, MSG_CONFIRM, address, address_len);

  #ifdef DEBUG
  printf("CONNECT: Sent first SYN message: \n");
  print_header(&socket->header);
  printf("\n");
  #endif
  
  //==============receive message of handshake==============

  receive__(socket, socket->recvbuf, MICROTCP_RECVBUF_LEN, MSG_WAITALL, (struct sockaddr *) &(socket->peer_addr));
  socket->init_win_size=socket->header.window;
  socket->curr_win_size=socket->header.window;
  socket->seq_number=socket->header.ack_number;

  #ifdef DEBUG
  printf("CONNECT: Received ACK, SYN message: \n");
  print_header(&socket->header);
  printf("\n");
  #endif
  
  //====================send final ack======================
  socket->seq_number++;
  socket->ack_number=socket->header.seq_number+1;
  socket->header.seq_number=socket->seq_number;
  socket->header.ack_number=socket->ack_number;
  socket->header.control=8;
  send__(socket, NULL, 0, MSG_CONFIRM, address, address_len);

  #ifdef DEBUG
  printf("CONNECT: Sent this final ACK, SYN: \n");
  print_header(&socket->header);
  printf("\n");
  #endif
  //========================================================

  socket->state=ESTABLISHED;
  return 0;

}

int
microtcp_accept (microtcp_sock_t *socket, struct sockaddr *address,socklen_t address_len)
{
  
  //====================receive message=====================
  receive__(socket, (socket->recvbuf), MICROTCP_RECVBUF_LEN, MSG_WAITALL,(struct sockaddr *) &(socket->peer_addr));
  #ifdef DEBUG
  printf("ACCEPT: I received this header: \n\n");
  print_header(&socket->header);
  printf("\n");
  #endif
  socket->init_win_size=socket->header.window;
  socket->curr_win_size=socket->header.window;
  //==============copy the peer address to struct==================
  memcpy(&(socket->peer_addr), address, address_len);
  //========================send ack===============================
  socket->seq_number++;
  socket->ack_number=socket->header.seq_number+1;
  socket->header.seq_number=socket->seq_number;
  socket->header.ack_number=socket->ack_number;
  socket->header.control=8;
  socket->header.window=MICROTCP_WIN_SIZE;

  send__(socket, NULL, 0, MSG_CONFIRM, (const struct sockaddr *) &(socket->peer_addr), address_len);
  #ifdef DEBUG
  printf("ACCEPT: I sent this header: \n");
  print_header(&socket->header);
  printf("\n");
  #endif
  //=============wait to receive the final ack=====================
  receive__(socket, (socket->recvbuf), MICROTCP_RECVBUF_LEN, MSG_WAITALL, (struct sockaddr *) &(socket->peer_addr));
  socket->ack_number=socket->header.seq_number;
  socket->seq_number=socket->header.ack_number;
  #ifdef DEBUG
  printf("ACCEPT: I received this header: \n");
  print_header(&socket->header);
  printf("\n");
  #endif
  
  socket->state=ESTABLISHED;
  return 0;

}

int
microtcp_shutdown (microtcp_sock_t *socket, int how)
{
   size_t address_len=sizeof(socket->peer_addr);
  if(socket->state==ESTABLISHED){
  
    //============SENDING FIRST FIN, ACK packet================
    socket->header.seq_number=socket->seq_number;
    socket->header.ack_number=socket->ack_number;
    socket->header.control=9;
  
    send__(socket, NULL, 0, 0, (const struct sockaddr *) &socket->peer_addr, address_len);
    #ifdef DEBUG
    printf("SHUTDOWN: I sent this FIN, ACK header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
    //============RECEIVING ACK PACKET================
    receive__(socket, (socket->recvbuf), MICROTCP_RECVBUF_LEN, 0, (struct sockaddr *) &socket->peer_addr);
    #ifdef DEBUG
    printf("SHUTDOWN: I received this ACK header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
    //============RECEIVING FIN, ACK packet================
    socket->seq_number=socket->header.ack_number;
    socket->ack_number=socket->header.seq_number+1;
    receive__(socket, (socket->recvbuf), MICROTCP_RECVBUF_LEN, 0,(struct sockaddr *) &socket->peer_addr);
    socket->state=CLOSING_BY_HOST;
    #ifdef DEBUG
    printf("SHUTDOWN_client: I received this FIN ACK header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
    //============SENDING ACK packet================
    socket->ack_number=socket->header.seq_number+1;
    socket->header.seq_number=socket->seq_number;
    socket->header.ack_number=socket->ack_number;
    socket->header.control=8;
    send__(socket, NULL, 0, 0, (const struct sockaddr *) &socket->peer_addr, address_len);
    socket->state=CLOSED;
    #ifdef DEBUG
    printf("SHUTDOWN_client: I sent this ACK header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
  }
  else if(socket->state==CLOSING_BY_PEER){
  
  //==========SENDING FIN, ACK PACKET========== 
    socket->header.control=9;  
    socket->header.seq_number=socket->seq_number;
    socket->header.ack_number=socket->ack_number;
    send__(socket, NULL, 0, 0, (const struct sockaddr *) &socket->peer_addr, address_len);
    #ifdef DEBUG
    printf("SHUTDOWN_server: I sent this FIN, ACK header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
    //===========RECEIVING FINAL ACK PACKET========
    receive__(socket, (socket->recvbuf), MICROTCP_RECVBUF_LEN, 0, (struct sockaddr *) &socket->peer_addr);
    socket->state=CLOSED;
    #ifdef DEBUG
    printf("SHUTDOWN_server: I received ACK header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
  }

}

static int seq_to_index(int *expected_ack_nums, int chunks , int seq){
  int i;
  for(i=0;i<chunks;i++){
    if(expected_ack_nums[i]==seq){
      return i;
    }
  }
  return -1;
}

ssize_t microtcp_send (microtcp_sock_t *socket, const void *buffer, size_t length, int flags){
  size_t remaining=length, sizeof_chunk;
  size_t bytes_to_send, bytes_send=0, bytes_send_tmp, bytes_per_chunk;
  int chunks, i=0, ack_counter=0, y=0, begin=0, end=0, k=0, tout=0, slow_start=1, con_avoidance=0;
  int ret_value=-1;
  //=============================
  int sent=0;
  int acked=-1;
  int acked_count=0;
  char ** chunks_arr=NULL;
  int * expected_ack_nums=NULL;
  int while_count=0;
  //=============================
  uint32_t initial_seq=socket->seq_number;
  int ret_val;
  #ifdef DEBUG
  printf("\n=I=I=I=I=I=I=I=I=I=I=I= NEW SEND =I=I=I=I=I=I=I=I=I=I=I=I=I=\n");
  #endif

  #ifdef SEQ_NUM 
  uint32_t second_seq;
  #endif 
  
  
  
  struct timeval timeout ;
  timeout . tv_sec = 0;
  timeout . tv_usec = MICROTCP_ACK_TIMEOUT_US ;
 // while(while_count<2){
  while(bytes_send<remaining){
    #ifdef DEBUG
      printf("\n=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=\n");
      printf("\n %d: iteration of while loop\n", while_count);
      while_count++;
    #endif
    
     //======creation of chunks=============
    if(acked==-1&&tout==0){
      #ifdef SMALL_WINDOW
        socket->cwnd=600;
      #endif
      //=====computation of chunks to send===
      
      bytes_to_send=MIN((remaining-bytes_send), socket->curr_win_size);
      bytes_to_send=MIN(bytes_to_send, socket->cwnd);
      
      bytes_per_chunk=MIN(MICROTCP_MSS, socket->curr_win_size);
      bytes_per_chunk=MIN(bytes_per_chunk, socket->cwnd);

      chunks=bytes_to_send/bytes_per_chunk;
      if(bytes_to_send%bytes_per_chunk){
        chunks++;
      }
      //=allocation for retransmission arrays=
      if(chunks_arr!=NULL&&sizeof(chunks_arr)<chunks){
        free(chunks_arr);
        free(expected_ack_nums);
        
        chunks_arr=malloc(chunks*sizeof(char*));
        expected_ack_nums=malloc(chunks*sizeof(int));

      }
      else if(chunks_arr==NULL){
        chunks_arr=malloc(chunks*sizeof(char*));
        expected_ack_nums=malloc(chunks*sizeof(int));
      }
      //======================================
      bytes_send_tmp=0;
      #ifdef ANGELA_DEBUG
        printf("\n CREATION OF PACKAGES:\n chunks: %d\n bytes_send: %d\n", chunks, bytes_send);
      #endif
      //end=bytes_send;
      //begin=bytes_send;
      for(i=0; i<chunks; i++){
        
        if ((bytes_to_send - bytes_send_tmp) >= bytes_per_chunk){
          sizeof_chunk=bytes_per_chunk;
        }
        else{
          sizeof_chunk = bytes_to_send-bytes_send_tmp+1;
        }

        socket->tmpbuf=malloc((sizeof_chunk)+1);
        end=end+(sizeof_chunk) - 1;

        // printf(" begin: %d\n end: %d\n sizeof_chunk: %d\n bytes_per_chunk: %d\n", begin, end, sizeof_chunk, bytes_per_chunk);

        for(y=begin; y<end; y++){
          socket->tmpbuf[k]=((uint8_t*)buffer)[y];
          k++;
        }

        socket->tmpbuf[k]='\0';
        begin=end;

        chunks_arr[i]=socket->tmpbuf;
        bytes_send_tmp = bytes_send_tmp + strlen(socket->tmpbuf);
        k=0;
      }
      sent=0;
    }
    //=I=I=I=I=I=I= CHUNKS TRANSMISSION =I=I=I=I=I=I=I=
    for(i=acked+1; i<chunks; i++){
      #ifdef MIKE_DEBUG
        printf("bytes_to_send: %d\n", remaining-bytes_send);
        printf("Chunks: %d\n", chunks);
        printf("chunk_size: %d\n", sizeof_chunk);
      #endif

      socket->seq_number=initial_seq+bytes_send;
      socket->header.seq_number=socket->seq_number;
      socket->header.control=0;
      socket->ack_number++;
      socket->header.ack_number=socket->ack_number;

      #ifdef SEQ_NUM
        if(i==1&&while_count==1){
          second_seq=socket->seq_number;
          socket->seq_number=initial_seq+2;
          socket->header.seq_number=socket->seq_number;
          socket->header.control=0;
        }
        else if(i==2&&while_count==1){
          socket->seq_number=second_seq+3;
          socket->header.seq_number=socket->seq_number;
          socket->header.control=0;
        }
      #endif

      bytes_send+=send__(socket, (uint8_t*)chunks_arr[i], strlen(chunks_arr[i]), 
      flags, (const struct sockaddr *) &(socket->peer_addr), sizeof(socket->peer_addr));

      #ifdef MIKE_DEBUG
      printf("SEND: \n");
      print_header(&socket->header);
      printf("\n");
      #endif
      if(sent<chunks){
        expected_ack_nums[sent]= initial_seq+bytes_send;
        sent++;
      }

      #ifdef MIKE_DEBUG
      printf("SEND: \n");
      print_header(&socket->header);
      printf("\n");
      #endif

      k=0;
    }
    //=I=I=I=I=I=I= END CHUNKS TRANSMISSION =I=I=I=I=I=I=I=
    /*receiving ack for every chunk*/
    
    for(i=acked+1; i<chunks; i++){
      setsockopt(socket->sd, SOL_SOCKET ,SO_RCVTIMEO , &timeout,
      sizeof(struct timeval));
      if(receive__(socket, (socket->recvbuf), MICROTCP_RECVBUF_LEN, 0, (struct sockaddr *) &socket->peer_addr)<0){
        tout=1;//timeout
        if(con_avoidance){
          socket->ssthresh=socket->cwnd/2;
          socket->cwnd=MIN(socket->ssthresh, MICROTCP_MSS);
          //if congestion avoidance then :slow start
          slow_start=1;
          con_avoidance=0;
        }
        
        break;
      }
      int tmp_index = seq_to_index(expected_ack_nums, chunks, socket->header.ack_number);
      // printf("\nret_value %d\n", ret_val);
      #ifdef ANGELA_DEBUG
        printf("\nChunk: %d will be freed header ack number%d \n",tmp_index, socket->header.ack_number);
      #endif
      //=======check for douplikets acks=========
      if(tmp_index>=0){
          if(acked==tmp_index){
            ack_counter++;
          }
          else{
            free(chunks_arr[tmp_index]);
            chunks_arr[tmp_index]=NULL;
            acked=tmp_index;
            ack_counter=1;
            //=========congestion avoidance / slow start=========
              socket->cwnd+= MICROTCP_MSS;
            //===================================================
          }
      }
      if(ack_counter==3){
          #ifdef DEBUG
            printf("\n========3 Duplicate acks=======\n");
          #endif
          if(con_avoidance==1){
            socket->ssthresh=socket->cwnd/2;
            socket->cwnd=socket->ssthresh;
          }
          break;
      }
      
      // printf("\nACK OF RECEIVED HEADER: %d\n", socket->header.ack_number);

      tout=0;//timeout
    }
    //=============check acked messages===========
    for(i=acked+1 ; i<chunks; i++){
      #ifdef DEBUG
        printf("============bytes_send==========: %d\n", bytes_send);
      #endif
      bytes_send=bytes_send-strlen(chunks_arr[i]);
      
    }
    #ifdef ANGELA_DEBUG
      printf("\nacked+1= %d, chunks-1= %d\n", acked+1, chunks);
    #endif
    if(acked+1==chunks)acked=-1;
   

    if(socket->header.window==0){
      #ifdef MIKE_DEBUG
      printf("\nWINDOW IS 0\n");
      #endif
      /*Send empty bodied messages until receive a non-zero window*/
      while(1){
        // printf("\nWINDOW IS 0\n");
        usleep(rand()%(MICROTCP_ACK_TIMEOUT_US));
        socket->seq_number++;
        socket->header.seq_number=socket->seq_number;
        send__(socket, NULL, 0, 0, (const struct sockaddr *) &(socket->peer_addr), sizeof(socket->peer_addr));
        if(receive__(socket, (socket->recvbuf), MICROTCP_RECVBUF_LEN, 0, (struct sockaddr *) &socket->peer_addr)>0){
          if(socket->peer_win_size!=0){
            break;
          }
        }
      }
    }

    socket->seq_number = socket->header.ack_number;
    //=============congestion avoidance=============
    if(socket->cwnd<=socket->ssthresh){
      slow_start=1;
      con_avoidance=0;
    }
    else{
      con_avoidance=1;
      slow_start=0;
    }
    //==============================================
    #ifdef DEBUG
      printf("\n=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=I=\n");
    #endif
  }

  free(expected_ack_nums);
  free(chunks_arr);
  #ifdef DEBUG
    printf("\n=I=I=I=I=I=I=I=I=I=I=I= END SEND =I=I=I=I=I=I=I=I=I=I=I=I=I=\n");
  #endif
  return bytes_send;
}

ssize_t
microtcp_recv (microtcp_sock_t *socket, void *buffer, size_t length, int flags){
  struct sockaddr_in address;
  socklen_t len=sizeof(socket->peer_addr);
 
  int ret= receive__(socket, buffer, sizeof(buffer), flags,(struct sockaddr *) &address);

  #ifdef ANGELA_DEBUG
  //  printf("BUFFER:%s\n",(char*)buffer);
  printf("\nreceiver ack: %d\n",socket->ack_number);
  printf("sender ack: %d\n",socket->header.ack_number);
  
  #endif
  
  /*
  *TERMINATION PROCESS
  *
  * RECEIVED FIN, ACK PACKET (?)
  */


  if(socket->header.control==9){
    #ifdef DEBUG
    printf("MICROTCP_RECV: I received this ACK, FIN header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
      
    //==========SENDING ACK PACKET========== 
    socket->seq_number=socket->header.ack_number;
    socket->ack_number=socket->header.seq_number+1;
    socket->header.ack_number=socket->ack_number;
    socket->header.seq_number=socket->seq_number;
    socket->header.control=8;
    
    send__(socket, NULL, 0, 0, (const struct sockaddr *) &socket->peer_addr, len);
    socket->state=CLOSING_BY_PEER;
    #ifdef DEBUG
    printf("MICROTCP_RECV: I sent this ACK header: \n");
    print_header(&socket->header);
    printf("\n");
    #endif
    
    return -1;
  }
  /*
  *checking if correct seq num
  *if not sending double ACK
  */
  #ifdef MIKE_DEBUG
      printf("RECEIVE: SOCKET_ACK%d  HEADER_SEQ %d\n", socket->ack_number, socket->header.seq_number);
  #endif
  
  #ifdef DEBUG
      printf("\nmicroTCP_receive: I received this header\n");
      print_header(&socket->header);
  #endif
  

    #ifdef TIMEOUT
      if(timeout_var==1){
        timeout_var=0;
        return 0;
      }
    #endif

    //====sending ACK========
    socket->curr_win_size-=socket->header.data_len;
    socket->seq_number++;
    if((socket->ack_number!=socket->header.seq_number)||ret==-2){ //==checking seq_number and checksum===
      #ifdef DEBUG
      printf("\n===========PACKET LOSS====================\n");
      #endif
      socket->header.seq_number=socket->header.ack_number;
      socket->header.ack_number=socket->ack_number;
      socket->header.control=8;

      #ifdef DEBUG
        printf("\nmicroTCP_receive: I sent this header\n");
        print_header(&socket->header);
      #endif
      send__(socket, NULL, 0, 0, (const struct sockaddr *) &socket->peer_addr, len);
      
      socket->curr_win_size+=socket->header.data_len;
      return 0;
    }
    else{
      socket->ack_number+=socket->header.data_len;
      socket->header.seq_number=socket->header.ack_number;
      socket->header.ack_number=socket->ack_number;
      socket->header.control=8;

      #ifdef DEBUG
        printf("\nmicroTCP_receive: I sent this header\n");
        print_header(&socket->header);
      #endif
      send__(socket, NULL, 0, 0, (const struct sockaddr *) &socket->peer_addr, len);
      
      socket->curr_win_size+=socket->header.data_len;
      return ret-sizeof(microtcp_header_t);
    }


    
}
 
static void * serialize(char *data, microtcp_header_t header, char* buffer){
  
  //======reset buffer============
  memset(buffer,0,MICROTCP_RECVBUF_LEN);

  uint32_t temp32 = htonl(header.seq_number);
  uint16_t temp16 = htons(header.control);
  size_t i=0;
  memcpy(buffer, &temp32, sizeof(header.seq_number));
  i=i+sizeof(header.seq_number);
  temp32 = htonl(header.ack_number);
  memcpy(buffer+i, &temp32, sizeof(header.ack_number));
  i=i+sizeof(header.ack_number);
  memcpy(buffer+i, &temp16, sizeof(header.control));
  i=i+sizeof(header.control);
  temp16 = htons(header.window);
  memcpy(buffer+i, &temp16, sizeof(header.window));
  i=i+sizeof(header.window);
  temp32 = htonl(header.data_len);
  memcpy(buffer+i, &temp32, sizeof(header.data_len));
  i=i+sizeof(header.data_len);
  temp32 = htonl(header.future_use0);
  memcpy(buffer+i, &temp32, sizeof(header.future_use0));
  i=i+sizeof(header.future_use0);
  temp32 = htonl(header.future_use1);
  memcpy(buffer+i, &temp32, sizeof(header.future_use1));
  i=i+sizeof(header.future_use1);
  temp32 = htonl(header.future_use2);
  memcpy(buffer+i, &temp32, sizeof(header.future_use2));
  i=i+sizeof(header.future_use2);
  temp32 = htonl(header.checksum);
  memcpy(buffer+i, &temp32, sizeof(header.checksum));
  i=i+sizeof(header.checksum);
  memcpy(buffer+i, data, header.data_len);
  
  #ifdef DEBUG
  //printf("SERIALIZE: data= %s\n",data);
  //printf("SERIALIZE: buffer= %s\n",buffer + i);
  //print_header(&header);
  #endif
  return buffer;
}


static char * deserialize(void *buffer,microtcp_header_t *header, char* data){
  
  uint32_t temp32;
  uint16_t temp16;
  size_t i=0;
  memcpy(&temp32,buffer,sizeof(header->seq_number));
  i=i+sizeof(header->seq_number);
  header->seq_number = ntohl(temp32);
  memcpy(&temp32,buffer+i,sizeof(header->ack_number));
  i=i+sizeof(header->ack_number);
  header->ack_number = ntohl(temp32);
  memcpy(&temp16,buffer+i,sizeof(header->control));
  i=i+sizeof(header->control);
  header->control = ntohs(temp16);
  memcpy(&temp16,buffer+i,sizeof(header->window));
  i=i+sizeof(header->window);
  header->window = ntohs(temp16);
  memcpy(&temp32,buffer+i,sizeof(header->data_len));
  i=i+sizeof(header->data_len);
  header->data_len = ntohl(temp32);
  memcpy(&temp32,buffer+i,sizeof(header->future_use0));
  i=i+sizeof(header->future_use0);
  header->future_use0 = ntohl(temp32);
  memcpy(&temp32,buffer+i,sizeof(header->future_use1));
  i=i+sizeof(header->future_use1);
  header->future_use1 = ntohl(temp32);
  memcpy(&temp32,buffer+i,sizeof(header->future_use2));
  i=i+sizeof(header->future_use2);
  header->future_use2 = ntohl(temp32);
  memcpy(&temp32,buffer+i,sizeof(header->checksum));
  i=i+sizeof(header->checksum);
  header->checksum = ntohl(temp32);
  strncpy((char* )data ,buffer+i, header->data_len);

  #ifdef DEBUG
  //printf("DESERIALIZE: data= %s\n",data);
  //printf("DESERIALIZE: buffer= %s\n",buffer + i);
  //print_header(header);
  #endif
  return data;
}
