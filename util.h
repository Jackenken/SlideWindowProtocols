#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>
#include "common.h"

//Linked list functions
int ll_get_length(LLnode *);
void ll_append_node(LLnode **, void *);
LLnode * ll_pop_node(LLnode **);
void ll_destroy_node(LLnode *);
void  ll_pop_nth_node(LLnode ** head_ptr,LLnode * pos);
LLnode * ll_get_nth_node(LLnode * head_ptr,int pos);
int ll_isempty(LLnode * head);
void ll_print_values(LLnode * head);
//Print functions
void print_cmd(Cmd *);

//Time functions
long timeval_usecdiff(struct timeval *, 
                      struct timeval *);

//TODO: Implement these functions
char * convert_frame_to_char(Frame *);
Frame * convert_char_to_frame(char *);

uint16_t crc16(const char *buf, int len);

#endif
