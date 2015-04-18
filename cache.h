//
//  cache.h
//  proxy
//
//  Created by Claire on 4/15/15.
//  Copyright (c) 2015 Claire. All rights reserved.
//

#ifndef __proxy__cache__
#define __proxy__cache__

#include <stdio.h>
#include <cerrno>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <iostream>
#include <stdbool.h>
#include <deque>
#include <string>
#include <queue>
#include "csapp.h"

#define DEBUG 1
#define MAX_MSG_LENGTH 8192

using std::deque;
using std::string;
using std::cout;
using std::endl;
using std::queue;


typedef struct CacheEntry{
    char *p_url;
    deque<char *> response_body;
    //struct CacheEntry * next;
}CacheEntry;

deque<CacheEntry> myCache;

string response_from_cache(string url);

void cache_response(string url,string response);

string socket_read(int fd){
    string result;
    
    char buf[MAX_MSG_LENGTH];
    int byteCount=0;
    rio_t my_rio;
    bzero(buf,sizeof(buf));
    Rio_readinitb(&my_rio, fd);
    while ( (byteCount = Rio_readlineb(&my_rio,buf,MAX_MSG_LENGTH))>0) {
        //if (DEBUG) cout<<"Bytes read from server in this loop:"<<byteCount<<endl;
        result+=buf;
        memset(buf,0,sizeof(buf));
    }
    return result;
}




#endif /* defined(__proxy__cache__) */
