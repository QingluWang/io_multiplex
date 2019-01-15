#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/resource.h>
#include "../util/my_util.h"
#include "../tcp/my_tcp.h"

static int g_count = 0;
int setnonblocking(int sockfd) 
{ 
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) { 
        return -1; 
    } 
    return 0; 
}
void* PthreadHandleMsg(void* para){
    char buffer[MAX_LEN];
    memset(buffer,0,sizeof(buffer));
    int flag=1;
    int receBytes=0;
    int sockFd=*(int*)para;
    /*while(flag){
        receBytes=recv(sockFd,buffer,MAX_LEN,0);
        if(receBytes<0){
            if(errno == EAGAIN){
                printf("ServerListen:EAGAIN\n");
                break;
            }
            else{
                printf("ServerListen:receive error!\n");
                break;
            }
        }
        else if(receBytes == 0){
            flag=0;
        }
        if(receBytes == sizeof(buffer)){
            flag=1;
        }
        else
            flag=0;
    }*/
    if((receBytes=recv(sockFd,buffer,MAX_LEN,0))>0){
        printf("Received:%s  Count:%d\n",buffer,g_count++);
        int sendBytes=send(sockFd,buffer,strlen(buffer),0);
        if(sendBytes == -1){
           perror("PthreadHandleMsg:send msg to client error!"); 
        }
        close(sockFd);
    }
    close(sockFd);
    pthread_exit(0);
}

int ServerInit(uint16_t port){
    /* 设置每个进程允许打开的最大文件数 */ 
    struct rlimit rt;
    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE; 
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) { 
        perror("ServerListen:setrlimit"); 
        exit(1); 
    }

    int sockId = TcpCreate();
    int set = 1;  
    setsockopt(sockId, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(int)); 
    struct sockaddr_in server;
    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockId,(struct sockaddr*)&server,sizeof(server)) < 0){
        perror("ServerInit: socket bind error");
    };
    if(listen(sockId,MAXEPOLLSIZE) < 0){
        perror("ServerInit: socket listen error");
    };
    setnonblocking(sockId);
    return sockId;
}
void ServerListen(int serverSockId){
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    int epfd = epoll_create(MAXEPOLLSIZE);
    if( -1 == epfd ){  
        perror ("ServerListen:epoll create error!");  
    }
    ev.data.fd=serverSockId;
    //ev.events=EPOLLIN | EPOLLET;
    ev.events=EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, serverSockId, &ev); 
    if(-1 == ret){
        perror ("ServerListen:epoll ctl error!");  
    }
    int newFd,nfds,curds=1;
    struct sockaddr_in client;
    socklen_t length = sizeof(struct sockaddr_in);
    while(1){
        nfds = epoll_wait(epfd,events,curds,-1);
        if(-1 == nfds){
            perror("ServerListen:epoll wait error!");
            continue;
        }
        //printf("curds:%d\n",curds);
        for(int n=0; n<nfds; n++){
            if(events[n].data.fd == serverSockId)//new link
            {
                newFd=accept(serverSockId,(struct sockaddr*)&client,&length);
                if(newFd<0){
                    perror("ServerListen:accept error!");
                    continue;
                }
                setnonblocking(newFd);
                //ev.events=EPOLLIN | EPOLLET;
                ev.events = EPOLLIN; 
                ev.data.fd = newFd;
                if(epoll_ctl(epfd,EPOLL_CTL_ADD,newFd,&ev)<0){
                    printf("ServerListen:put socket %d to epoll failed",newFd);
                }
                curds++;
            }
            else if(events[n].events&EPOLLIN)//recevied data
            {
                if(events[n].data.fd<0)
                    continue;
                int sockFd=events[n].data.fd;
                char buffer[MAX_LEN];
                memset(&buffer,0,sizeof(buffer));
                int receBytes=recv(sockFd,buffer,MAX_LEN,0);
                if(receBytes>0)
                    printf("Received:%s Count:%d\n",buffer,g_count++);
                /*pthread_t tid;
                pthread_attr_t attr;
                if(0 != pthread_attr_init(&attr)){
                    perror("client main:pthread_attr_init error!\n");
                }
                if(0 != pthread_attr_setstacksize(&attr,20480)){
                    perror("ServerListen:pthread_attr_setstacksize error!\n");
                }

                int status=pthread_create(&tid,&attr,PthreadHandleMsg,(void*)&(events[n].data.fd));
                if(status != 0){
                    perror("ServerListen:pthread_create error!\n");
                }
                pthread_detach(tid);
                usleep(200);*/
                fflush(stdout);
            }
        }
    }
}



