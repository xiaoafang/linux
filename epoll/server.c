#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
int statup(int port)
{
    int sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0)
    {
        perror("socket\n");
        return 1;
    }
    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(port);
    printf("dddddddddddd\n");
    
    if(bind(sock_fd,(struct sockaddr*)&server,sizeof(server))<0)
    {
        perror("bind");
        return 3;
    }
    if(listen(sock_fd,10)<0)
    {
        perror("listen");
        return 2;
    }
    printf("listen bind sussecc...\n");
    return sock_fd;
}


void handler_events(int epoll_fd,struct epoll_event *ev,int listen_fd,int num)
{
    struct sockaddr_in client;
    socklen_t len=sizeof(client);
    struct epoll_event eve;
    int i=0;
    for(;i<num;i++)
    {
        if(ev[i].data.fd==listen_fd && ev[i].events==EPOLLIN)
        {
            int lis_sock = accept(listen_fd,(struct sockaddr*)&client,&len);   
            if(lis_sock<0)
            {
                perror("accpet\n");
                continue;
            }
            printf("get a new client...\n");
            eve.events=EPOLLIN;
            eve.data.fd=lis_sock;
            epoll_ctl(epoll_fd,EPOLL_CTL_ADD,lis_sock,&eve);
            continue;
        }
        if(ev[i].events == EPOLLIN)
        {
            char buf[10240];
            size_t s= read(ev[i].data.fd,buf,sizeof(buf)-1);
            if(s>0)
            {
                buf[s]='\0';
                printf("%s",buf);
                eve.events=EPOLLOUT;
                eve.data.fd=ev[i].data.fd;
                epoll_ctl(epoll_fd,EPOLL_CTL_MOD,ev[i].data.fd,&eve);
            }
            else if(s==0)
            {
                printf("client quit ...\n");
                close(ev[i].data.fd);
                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,ev[i].data.fd,NULL);
            }
            else
            {
                perror("read\n");
                close(ev[i].data.fd);
                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,ev[i].data.fd,NULL);
            }
            continue;
        }
        if(ev[i].events==EPOLLOUT)
        {
            const char *echo = "HTTP/1.1 200 OK \r\n\r\n<html>hello epoll</html>";
            write(ev[i].data.fd,echo,strlen(echo));
            close(ev[i].data.fd);
            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,ev[i].data.fd,NULL);
        }
    }
}
int main(int argc,char *argv[])
{
    if(argc!=2)
    {
        printf("usage :%s port\n",argv[0]);
        exit(1);
    }
    int listen_sock =statup(atoi(argv[1])); 
    int epoll_fd = epoll_create(22);
    if(epoll_fd<0)
    {
        perror("epoll_fd\n");
        return 2;
    }

    struct epoll_event ev;
    ev.events=EPOLLIN;
    ev.data.fd=listen_sock;

    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_sock,&ev)<0)
    {
        perror("epoll_ctl\n");
        return 3;
    }
    for(;;)
    {
        struct epoll_event  events[11];
        int n=sizeof(events)/sizeof(events[0]);
        int ret=epoll_wait(epoll_fd,events,n,1000);
        switch(ret)
        {
            case -1:
                perror("epoll_wait\n");
                break;
            case 0:
                printf("timeout\n");
                break;
            default:
                handler_events(epoll_fd,events,listen_sock,n);
        }
    }
}
