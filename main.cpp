#include "cache.h"
#include "csapp.h"
#define MAX_MSG_LENGTH 8192
#define MAX_BACK_LOG (5)

#define DEBUG 1

int CacheSize;

void *webtalk(void * socket_desc);



pthread_mutex_t mutex;

int main(int argc,char **argv){
    
    uint16_t port;
    
    if(argc < 3){
        printf("Command should be: <port>  <size of cache in MBs>\n");
        return 1;
    }
    
    port = atoi(argv[1]);
    if(port <1024 || port >65535){
        printf("Port number should be equal to or larger than 1024 and smaller than 65535\n");
        return 1;
    }
    
    CacheSize=atoi(argv[2]);
    
    //Implementation Details: 2. Make sure that your proxy ignores SIGPIPE signals by installing the ignore function as handler.
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
        printf("enter while loop\n");
       
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


string response_from_cache(string url){
    for (deque<CacheEntry>::iterator it=myCache.begin(); it!=myCache.end(); ++it) {
        if (it->p_url.compare(url)==0 ){
            CacheEntry move2back;
            move2back.p_url=it->p_url;
            move2back.response_body=it->response_body;
            myCache.erase(it);
            myCache.push_back(move2back);
            return move2back.response_body;
        }
    }
    

    
    return "";
}

void cache_response(string url,string response){
    
    if(DEBUG)cout<<"Caching Response"<<endl;
    CacheEntry entry;
    entry.p_url=url;
    entry.response_body=response;
    /*remove the last recently used entry(one at the back of deque)*/
    
    while(myCache.size()>= CacheSize) {
        if(DEBUG) cout<<"Not enough space in cache, popthe last one"<<endl;
        myCache.pop_back();
        
    }
    myCache.push_front(entry);
    
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
    if (DEBUG) printf("Connect Server socket created\n");
    
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

void proxy_to_browser(int new_s, string response){
    
    size_t nleft = response.length();
    ssize_t nwritten;
    char *bufp =strdup(response.c_str());
    
    while (nleft > 0) {
        if ((nwritten = write(new_s, bufp, nleft)) <= 0) {
            if (errno == EINTR) // interrupted by sig handler return
                nwritten = 0;    // and call write() again
            else
                break;       // errorno set by write()
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    
    
    
}

/* 4.Upon receiving a request for a new connection, the proxy should spawn a thread to process it.*/

void *webtalk(void * socket_desc){
    
    //
     pthread_mutex_lock(&mutex);
    if(DEBUG)printf("Enter handler\n");
    
    int new_s= *(int *)socket_desc;
    int serverPort;
    
    char url[MAX_MSG_LENGTH];
    char buf[MAX_MSG_LENGTH];
    char host[MAX_MSG_LENGTH];
    char  *token,*cmd, *version, *file, *brkt;
    
    char * request_buf;
    ssize_t bytes_read;
    token=" \r\n";
    
    
    while( (bytes_read = read (new_s,buf,sizeof(buf)) ) >0){
        if(DEBUG)printf("read succeed\n");
        if(DEBUG)printf("HTTP REQUEST IS : %s\n", buf);
        
        
    }
    
    buf[bytes_read]=0;
    
    bool keep_alive=false;
    char * p=strstr(buf, "Connection: keep-alive\r\n");
    if(p!=NULL){
        keep_alive=true;
    }
    
    /*Parsing HTTP*/
    char *nbuf=strdup(buf);
    cmd=strtok_r(nbuf, token, &brkt);
    strcpy(url, strtok_r(NULL, token, &brkt));
    version = strtok_r(NULL, token, &brkt);
    parseAddress(url,host,&file, &serverPort);
    
    char reply[MAX_MSG_LENGTH];
    string response;
    
    if (strcmp(cmd, "GET")==0) {
        if(DEBUG) cout<<"GET request received"<<endl;
        response=response_from_cache(url);
        
        if (response.length()>0) {/*return from cache*/
            cout<<"Response from cache"<<endl;
            /*foward http_resonse to browser*/
            proxy_to_browser(new_s, response);
            
        }
        
        else{/*read from server*/
            cout<<"Reading from server"<<endl;
            int sock=conn_server(buf, host,serverPort );
            ssize_t recv_len = 0;
            
            /*send http_request to server*/
            if (send(sock, buf, sizeof(buf), 0) < 0) {
                perror("Send error:");
            }
            
            bzero(reply, sizeof(reply));
            
            /*read http_response from server*/
            while((recv_len = read(sock, reply,sizeof(reply)))>0){
                string append;
                append.copy(reply, recv_len);
                response+=append;
                memset(reply, 0, sizeof(reply));
            }
            if(DEBUG) cout<<"HTTP Response is: "<<response<<endl;
            proxy_to_browser(new_s,response);
            cache_response(url,response);
        }
        
        
    }
    
    close(new_s);
    if (DEBUG) cout<<"Exit Handler"<<endl;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}


