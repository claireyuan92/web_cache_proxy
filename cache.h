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

using std::deque;
using std::string;
using std::cout;
using std::endl;


typedef struct CacheEntry{
    string p_url;
    string response_body;
    //struct CacheEntry * next;
}CacheEntry;

deque<CacheEntry> myCache;

string response_from_cache(string url);

void cache_response(string url,string response);





#endif /* defined(__proxy__cache__) */
