#include "cache.h"
#define MAX_BACK_LOG (5)


pthread_mutex_t mutex;
int CacheSize;


void *webtalk(void * socket_desc);

int main(int argc,char **argv){
    
    uint16_t port;
    int cache_size;
    
    if(argc < 3){
        printf("Command should be: <port>  <size of cache in MBs>\n");
        return 1;
    }
    
    port = atoi(argv[1]);
    if(port <1024 || port >65535){
        printf("Port number should be equal to or larger than 1024 and smaller than 65535\n");
        return 1;
    }
    
    CacheSize =atoi(argv[2]);
    
    //Implementation Details: 2. Make sure that your proxy ignores SGIPIPE signals by installing the ignore function as handler.
    
    signal(SIGPIPE,SIG_IGN);
    struct sockaddr_in client_addr;
    
    /* build address data structure*/
    uint32_t addr_len = sizeof(client_addr);
    bzero((char *)&client_addr, addr_len);
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port= htons(port);
    
    int listenfd;
    /* setup passive open*/
    if((listenfd = socket(AF_INET, SOCK_STREAM/* use tcp*/,0)) < 0){
        perror("simplex-talk: socket\n");
        exit(1);
    }
    if(DEBUG)printf("Socket created\n");
    
    
    if( (bind(listenfd,(struct sockaddr *)&client_addr,addr_len)) < 0) {
        perror("simplex-talk: bind\n");
        exit(1);
    }
    
    if(DEBUG)printf("Bind port succeed\n");
    
    listen(listenfd, MAX_BACK_LOG);
    if(DEBUG)printf("Proxy is Lisenning\n");
    
    
    pthread_t thread_id;
    /* if writing to log files, force each thread to grab a lock before writing
     to the files */
    
    pthread_mutex_init(&mutex, NULL);
    
    /*wait for connecction, then accept*/
    
    while(1){
        if(DEBUG)cout<<"Enter Handler"<<endl;
        int *new_s=(int *)malloc(sizeof(int));
        if((*new_s = accept(listenfd, (struct sockaddr *)&client_addr,&addr_len ))>=0){
            if(DEBUG)printf("Accepted connection\n");
            pthread_create(&thread_id, NULL, webtalk, (void *)new_s);
            pthread_detach(thread_id);
        }
        else{
            free(new_s);
            perror("accept error\n");
        }
    }
    pthread_mutex_destroy(&mutex);
    return 1;
}




/* a possibly handy function that we provide to parse the address */
void parseAddress(char* url, char* host, char** file, int* serverPort){
    
    char *point1;
    //char *point2;
    char *saveptr;
    
    if(strstr(url, "http://"))
        url = &(url[7]);
    *file = strchr(url, '/');
    
    strcpy(host, url);
    
    /* first time strtok_r is called, returns pointer to host */
    /* strtok_r (and strtok) destroy the string that is tokenized */
    
    /* get rid of everything after the first / */
    
    strtok_r(host, "/", &saveptr);
    
    /* now look to see if we have a colon */
    
    point1 = strchr(host, ':');
    if(!point1) {
        *serverPort = 80;  // 443?
        return;
    }
    
    /* we do have a colon, so get the host part out */
    strtok_r(host, ":", &saveptr);
    
    /* now get the part after the : */
    *serverPort = atoi(strtok_r(NULL, "/",&saveptr));
}




int conn_server(const char * http_request,char * hostname, int serverPort){
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *hp;
    
    if ((sock=socket(AF_INET, SOCK_STREAM/* use tcp*/, 0))<0) {
        perror("Create socket error:");
    }
    if (DEBUG) printf("connet Server socket created\n");
    
    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL){
        perror("gethostbyname error"); /* check h_errno for cause of error */
        return -2;
    }
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr,
          (char *)&server_addr.sin_addr.s_addr, hp->h_length);
    server_addr.sin_port = htons(serverPort);
    
    /* Establish a connection with the server */
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        perror("Connect error");
        return -1;
    }
    return sock;
}


/* 4.Upon receiving a request for a new connection, the proxy should spawn a thread to process it.*/

void *webtalk(void * socket_desc){
    
    pthread_mutex_lock(&mutex);
    
    if(DEBUG)printf("Enter handler\n");
    int new_s= *(int *)socket_desc;
    int serverPort;
    
    
    
    char buf[MAX_MSG_LENGTH];
    char host[MAX_MSG_LENGTH];
    char  *url=NULL, *token=NULL,*cmd=NULL, *version=NULL, *file=NULL, *brkt=NULL;
    ssize_t bytes_read;
    token=" \r\n";
    
    if ( (bytes_read = read (new_s,buf,MAX_MSG_LENGTH) ) >0){
        if(DEBUG)printf("read succeed\n");
        buf[bytes_read]=0;
        if(DEBUG)cout<<buf<<endl;
        
        bool keep_alive=false;
        char * p=strstr(buf, "Connection: keep-alive\r\n");
        if(p!=NULL){
            keep_alive=true;
        }
        
        /*Parsing HTTP*/
        char *nbuf=strdup(buf);
        
        
        if (DEBUG) cout<<"Parsing this Request:  "<<nbuf<<endl;
        cmd=strtok_r(nbuf, token, &brkt);
        url=strtok_r(NULL, token, &brkt);
        version = strtok_r(NULL, token, &brkt);
        parseAddress(url,host,&file, &serverPort);
    }
    else{
        perror("Read error:");
    }
    
    
    char reply[MAX_MSG_LENGTH];
    
    if (strcmp(cmd, "GET")==0) {
        if (response_from_cache(new_s,url)) {
            /*return from cache*/
            if(DEBUG)cout<<"Response from Cache succeed"<<endl;
        }
        else{
            if (DEBUG) cout<<"Querying from server"<<endl;
            /*===Cache Response Start===*/
            CacheEntry this_entry;
            this_entry.p_url=strdup(url);
            
            /*Connect to the server*/
            int sock=conn_server(buf, host,serverPort );
            ssize_t recv_len = 0;
            //sending request
            if (send(sock, buf, MAX_MSG_LENGTH, 0) < 0) {
                perror("Send error:");
                
            }
            bzero(reply, sizeof(reply));
            
            /*Read from server*/
            while((recv_len = read(sock, reply,sizeof(reply)))> 0){
                
                size_t nleft = recv_len;
                ssize_t nwritten;
                
                
                char *bufp =  reply;
                
                while (nleft > 0) {
                    if ((nwritten = send(new_s, bufp, nleft,0)) <= 0) {
                        if (errno == EINTR) // interrupted by sig handler return
                            nwritten = 0;    // and call write() again
                        else
                            break;       // errorno set by write()
                    }
                    //if(DEBUG)printf("Server reply bufp:\n%s\n", bufp);
                    nleft -= nwritten;
                    bufp += nwritten;
                    
                }
                
                if(DEBUG) cout<<"reply buffer sent!"<<endl;
                
                /*===Add this reply to response body===*/
               // reply[recv_len]=0;
                
                this_entry.response_body.push_back(reply);
                
                if(DEBUG) cout<<"*************Thie entry is :"<<this_entry.response_body.back()<<endl;
                
                memset(reply, 0, sizeof(reply));
            }
            
            /*Close server socket*/
            close(sock);
            
            /*===Add this entry to Cache*/
            if(DEBUG)cout<<"Whole response received"<<endl;
            cache_response(this_entry);
            
            
        }
    }
    
    /*close browser socket*/
    if (DEBUG) cout<<"Closing socket"<<endl;
    close(new_s);
    free(socket_desc);
    pthread_mutex_unlock(&mutex);
    if (DEBUG) cout<<"EXIT HANDLER"<<endl;
    
    return NULL;
}


void cache_response(CacheEntry entry){
    if (DEBUG) cout<<"Caching Response"<<endl;
    
    while (myCache.size() >= CacheSize) {
        if(DEBUG) cout<<"No enough space in cache, pop the front"<<endl;
        myCache.pop_front();
    }
    myCache.push_back(entry);
    
    if (DEBUG) {
        cout<<"*************Cached response url is :"<< entry.p_url;
    
        for (deque<string>::reverse_iterator it=entry.response_body.rbegin(); it!=entry.response_body.rend(); ++it) {
            cout<<"*****************Cahced response http is :"<<*it<<endl;
        }
    }
    if (DEBUG) cout<<"Response Cached"<<endl;
}

bool response_from_cache(int browserfd, char *url){
    for (deque<CacheEntry>::iterator it=myCache.begin(); it!=myCache.end(); ++it) {
        if (strcmp(it->p_url,url)==0) {
            if(DEBUG)cout<<"HITTTTTTTTT!!!!!!Response from Cache"<<endl;
            CacheEntry move2back;
            move2back.p_url=it->p_url;
            move2back.response_body=it->response_body;
            
            while (it->response_body.size() >0) {
                if(DEBUG){
                    cout<<"Browser fd is :"<<browserfd<<endl;
                    cout<<"Cache Entry is: "<<it->response_body.front()<<endl;
                    
                }
                
                string str = it->response_body.front();
                char *cstr = new char[str.length() + 1];
                strcpy(cstr, str.c_str());
                // do stuff
                
                //char *buf=strdup(it->response_body.front().c_str());
                
                
                
                size_t nleft = strlen(cstr);
                ssize_t nwritten;
                
                
                char *bufp =  cstr;
                
                while (nleft > 0) {
                    if ((nwritten = send(browserfd, bufp, nleft,0)) <= 0) {
                        if (errno == EINTR) // interrupted by sig handler return
                            nwritten = 0;    // and call write() again
                        else
                            break;       // errorno set by write()
                    }
                    if(DEBUG)printf("Server reply bufp:\n%s\n", bufp);
                    nleft -= nwritten;
                    bufp += nwritten;
                    
                }

                delete [] cstr;
                
                it->response_body.pop_front();
               
            }
            
            myCache.erase(it);
            myCache.push_back(move2back);
            return true;
        }
        
    }
    return false;
    
}
