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

#define MYPORT 5000
#define BACKLOG 10
#define MAXDATASIZE 100

int main(void)
{
    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size, numbytes;
    int yes = 1, count = 0;
    char sendbuf[MAXDATASIZE];
    char recbuf[MAXDATASIZE];
    int flag = 0;

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
        printf("Primit: %s\n", recbuf);

        if (send(new_fd, "SYN-ACK", MAXDATASIZE-1, 0) == -1) perror("send");

        if ((numbytes=recv(new_fd, recbuf, MAXDATASIZE-1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }
        recbuf[numbytes] = '\0';
        printf("Primit: %s\n", recbuf);

        while(1)
        {
            fflush(stdout);
            
            if(count == 5)
            {
                
                while(1)
                {
                    sprintf(sendbuf, "%s[%d]", "CHECK", count);
                    if (send(new_fd, sendbuf, MAXDATASIZE - 1, 0) == -1) perror("send");
                    if ((numbytes=recv(new_fd, recbuf, MAXDATASIZE-1, 0)) == -1) 
                    {
                        perror("recv");
                        exit(1);
                    }
                    if(strcmp(recbuf, "ACK") == 0) 
                    {
                        printf("CEPLM");
                        sprintf(sendbuf, "ACK(%d)", count);
                        count++;
                        break;
                    }
                }
            }
            else
            {
                if ((numbytes=recv(new_fd, recbuf, MAXDATASIZE-1, 0)) == -1) 
                {
                    perror("recv");
                    exit(1);
                }
                recbuf[numbytes] = '\0';
                printf("Primit: %s\n", recbuf);
                if(strcmp(recbuf, "FIN") == 0) break;
                sprintf(sendbuf, "ACK(%d)", count);
                if (send(new_fd, sendbuf, MAXDATASIZE - 1, 0) == -1) perror("send");

                 count++;
            }
        }
       
        if (send(new_fd, "ACK", MAXDATASIZE-1, 0) == -1) perror("send");
        if (send(new_fd, "FIN", MAXDATASIZE-1, 0) == -1) perror("send");
        printf("AM INCHEIAT CONEXIUNEA\n");
        close(new_fd);
    }

    return 0;
}
