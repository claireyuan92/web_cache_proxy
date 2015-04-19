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
#include <string.h>
#include <queue>
//#include "csapp.h"

#define DEBUG 1
#define MAX_MSG_LENGTH 8192

using std::deque;
using std::string;
using std::cout;
using std::endl;
using std::queue;


typedef struct CacheEntry{
    char *p_url;
    string response_body;
    //struct CacheEntry * next;
}CacheEntry;

/*This is a LRU cache maintained by a FIFO deque, pop the front when there is no enough room in the cache, push back when a new entry comes in */

deque<CacheEntry> myCache;


void cache_response(CacheEntry entry);
bool response_from_cache(int browserfd, char * url);



#endif /* defined(__proxy__cache__) */
