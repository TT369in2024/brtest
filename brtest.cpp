
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define ECHO_PORT (7)

int64_t Sendbytes = 0;
int64_t Sendframes = 0;
int64_t Recvbytes = 0;
int64_t Recvframes = 0;
int64_t g_start = 0;
bool gExit = true;
pthread_mutex_t glook = PTHREAD_MUTEX_INITIALIZER;

static void run(char* ifName)
{
    char message[4096];

    int fd;
    struct sockaddr_in sin;
    socklen_t sin_len = sizeof(struct sockaddr_in);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port=htons(ECHO_PORT);

    fd=socket(AF_INET, SOCK_DGRAM, 0);
    int r = bind(fd, (struct sockaddr *)&sin, sin_len);
    if(r < 0)
    {
        printf( "socket() bind fail!!!(%d)%s\n", errno, strerror(errno));
        close(fd);
        gExit = true;
        return ;
    }
    if(ifName && strlen(ifName) > 0)
    {
        printf( "bound net_if_name: %s\n",ifName);
        setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
            ifName, strlen(ifName));
    }
    Sendbytes = 0;
    Sendframes = 0;
    Recvbytes = 0;
    Recvframes = 0;

    timespec nowTime;
    clock_gettime(CLOCK_MONOTONIC, &nowTime);
    g_start = nowTime.tv_sec * 1000 + nowTime.tv_nsec / 1000 / 1000;

    fd_set sockfd_set;
    struct timeval timeout_val;

    printf( "start run \n");
    while(!gExit)
    {
        timeout_val.tv_sec = 1;
        timeout_val.tv_usec = 0;
        FD_SET(fd, &sockfd_set);
        int ret = select(fd+1,&sockfd_set,NULL,NULL, &timeout_val);
        if(ret == 0)
        {
            continue;
        }
        else if(ret < 0)
        {
            printf( "select() fail!!!(%d)%s\n", errno, strerror(errno));
            break;
        }
        struct sockaddr_in sout;
        socklen_t sout_len = sizeof(struct sockaddr_in);
        ssize_t datalen = recvfrom(
            fd,
            message,
            sizeof(message),
            0,
            (struct sockaddr *)&sout,
            &sout_len);
        if(datalen < 0)
        {
            continue;
        }

        pthread_mutex_lock(&glook);
        Recvbytes += datalen;
        Recvframes ++;
        pthread_mutex_unlock(&glook);

        int r = sendto(fd, message,datalen, 0,
            (struct sockaddr *)&sout, sout_len);
        if(r < 0)
        {
            printf("send fail!!!(%d)%s \n", errno, strerror(errno));
        }
        else
        {
            pthread_mutex_lock(&glook);
            Sendbytes += datalen;
            Sendframes++;
            pthread_mutex_unlock(&glook);
        }
    }
    close(fd);
    gExit = true;
}

int main(int c, char* a[])
{
    gExit = false;
    run(NULL);
}
