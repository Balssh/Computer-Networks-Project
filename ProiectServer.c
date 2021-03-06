#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define MYPORT 5000
#define BACKLOG 10
#define MAXDATASIZE 100

typedef struct { int sockfd; } thread_config_t;

void Sync(int new_fd, char *recbuf)
{
    int numbytes;
    while(1)
        {
            if ((numbytes=recv(new_fd, recbuf, MAXDATASIZE-1, 0)) == -1) 
            {
                perror("recv");
                exit(1);
            }
            if(strcmp(recbuf, "SYN") == 0) break;
            if (send(new_fd, "", MAXDATASIZE-1, 0) == -1) perror("send");
        }
        recbuf[numbytes] = '\0';
        printf("FROM[%d]: %s\n", new_fd, recbuf);

        if (send(new_fd, "SYN-ACK", MAXDATASIZE-1, 0) == -1) perror("send");

        if ((numbytes=recv(new_fd, recbuf, MAXDATASIZE-1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }
        recbuf[numbytes] = '\0';
        printf("FROM[%d]: %s\n", new_fd, recbuf);
}

void Receive(int new_fd, char *recbuf, char *sendbuf, int *count)
{
    int numbytes;
    int flag = 0;
    while(1)
    {
        fflush(stdout);
        
        if ((numbytes=recv(new_fd, recbuf, MAXDATASIZE-1, 0)) == -1) 
        {
            perror("recv");
            exit(1);
        }
        recbuf[numbytes] = '\0';
        printf("FROM[%d]: %s\n", new_fd, recbuf);
        if(strcmp(recbuf, "FIN") == 0) return;

        if((*count) > 5) flag = rand() % 2;
        if(flag == 1)
        {
            sprintf(sendbuf, "CHECK(%d)", *count);
            if (send(new_fd, sendbuf, MAXDATASIZE - 1, 0) == -1) perror("send");

            while(1)
            {
                if ((numbytes=recv(new_fd, recbuf, MAXDATASIZE-1, 0)) == -1) 
                {
                    perror("recv");
                    exit(1);
                }
                if(strcmp(recbuf, "ACK") == 0) 
                {
                    recbuf[numbytes] = '\0';
                    printf("FROM[%d]: %s\n", new_fd, recbuf);
                    sprintf(sendbuf, "ACK(%d)", *count);
                    if (send(new_fd, sendbuf, MAXDATASIZE - 1, 0) == -1) perror("send");
                    (*count)++;
                    break;
                }
                else if(strcmp(recbuf, "FIN") == 0) return;
                sprintf(sendbuf, "CHECK(%d)", *count);
                if (send(new_fd, sendbuf, MAXDATASIZE - 1, 0) == -1) perror("send");
            }
        }
        else
        {
            sprintf(sendbuf, "ACK(%d)", *count);
            if (send(new_fd, sendbuf, MAXDATASIZE - 1, 0) == -1) perror("send");

            (*count)++;
        }
    }
}

void ServeConnection(int new_fd)
{
    int *count = malloc(sizeof(int));
    char sendbuf[MAXDATASIZE];
    char recbuf[MAXDATASIZE];
    (*count) = 0;

    Sync(new_fd, recbuf); // SYNCING WITH CLIENT
    Receive(new_fd, recbuf, sendbuf, count); // RECEIVING MESSAGES FROM CLIENT

    if (send(new_fd, "ACK", MAXDATASIZE-1, 0) == -1) perror("send");
    if (send(new_fd, "FIN", MAXDATASIZE-1, 0) == -1) perror("send");
    printf("CONNECTION TERMINATED WITH[%d]\n", new_fd);
    close(new_fd);
}

void* ServerThread(void* arg)
{
    thread_config_t* config = (thread_config_t*)arg;
    int sockfd = config->sockfd;
    free(config);

    unsigned long id = (unsigned long)pthread_self();
    printf("Thread %lu created to handle connection with socket %d\n", id, sockfd);
    ServeConnection(sockfd);
    printf("Thread %lu done\n", id);
    return 0;
}

int main(void)
{
    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size, numbytes;
    int yes = 1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
    {
        perror("setsockopt");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) 
    {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) 
    {
        perror("listen");
        exit(1);
    }
    
    while(1) 
    {  
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) 
        {
            perror("accept");
            continue;
        }
        printf("server: conexiune de la: %s\n", inet_ntoa(their_addr.sin_addr));

        pthread_t the_thread;
        thread_config_t* config = (thread_config_t*)malloc(sizeof(*config));

        config->sockfd = new_fd;
        pthread_create(&the_thread, NULL, ServerThread, config);
        pthread_detach(the_thread);
    }
    return 0;
}
