#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/resource.h>
#include "my_client.h"
#include "../util/my_util.h"


static const char g_serverAddr[]="127.0.0.1";
static const uint16_t g_port=17989;
static int g_count = 0;
static int g_theryNum=0;
static const int g_stackSize=20480;

void* PthreadSendMsg(void* para){
    int sockFd=ClientConnect(g_serverAddr,g_port);
    int seq=*(int*)para;
    char buffer[128]={0};
    sprintf(buffer,"%d",seq);
    //ClientSendMsg(sockFd,buffer);
    printf("NO.%d pthread's sockFd=%d Send:%s  Count:%d  TheryNum:%d\n",seq,sockFd,buffer,g_count++,g_theryNum);
    pthread_exit(0);
}
int main(int argc,char* argv[]){
    /* 设置每个进程允许打开的最大文件数 */ 
    struct rlimit rt;
    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE; 
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) { 
        perror("ServerListen:setrlimit"); 
        exit(1); 
    }
    int i=0;
    g_theryNum=atoi(argv[1]);
    for(; i<g_theryNum; i++){
        pthread_t tid;
        pthread_attr_t attr;
        if(0 != pthread_attr_init(&attr)){
            perror("client main:pthread_attr_init error!\n");
        }
        /*size_t size1=0;
        pthread_attr_getstacksize(&attr,&size1);
        printf("before stack size:0x%x\n",size1);*/
        if(0 != pthread_attr_setstacksize(&attr,g_stackSize)){
            perror("client main:pthread_attr_setstacksize error!\n");
        }
        /*size_t size2=0;
        pthread_attr_getstacksize(&attr,&size2);
        printf("after stack size:0x%x\n",size2);*/
        int status=pthread_create(&tid,&attr,PthreadSendMsg,(void*)&i);
        if(status != 0){
            perror("client main:pthread_create error!\n");
        }
        pthread_detach(tid);
        /*if(0 != pthread_attr_destroy(&attr)){
            perror("client main:pthread_attr_destroy error!\n");
        }*/
    }
    sleep(5);
    return 0;
}
