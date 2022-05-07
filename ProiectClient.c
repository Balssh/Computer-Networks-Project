#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 5000

#define MAXDATASIZE 100

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char sendbuf[MAXDATASIZE];
    char recbuf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr;
    int count = 0;

    if (argc != 2) 
    {
        fprintf(stderr,"utilizare: client host\n");
        exit(1);
    }

    if ((he=gethostbyname(argv[1])) == NULL) 
    {
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(PORT);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) 
    {
        perror("connect");
        exit(1);
    }

    while(1)
    {
        scanf("%s", sendbuf);
        if (send(sockfd, sendbuf, MAXDATASIZE-1, 0) == -1) perror("send");

        if ((numbytes=recv(sockfd, recbuf, MAXDATASIZE-1, 0)) == -1) 
        {
            perror("recv");
            exit(1);
        }
        if(strcmp(recbuf, "SYN-ACK") == 0) break;
    }
    recbuf[numbytes] = '\0';
    printf("Primit: %s\n", recbuf);

    if (send(sockfd, "ACK", MAXDATASIZE-1, 0) == -1) perror("send");

    while(1)
    {   
        fflush(stdout);

        scanf("%s", sendbuf);
        if (send(sockfd, sendbuf, MAXDATASIZE - 1, 0) == -1) perror("send");

        if ((numbytes=recv(sockfd, recbuf, MAXDATASIZE - 1, 0)) == -1) 
        {
            perror("recv");
            exit(1);
        }
        recbuf[numbytes] = '\0';
        printf("Primit: %s\n", recbuf);
        if(strcmp(recbuf, "ACK") == 0) break;
    }
    if ((numbytes=recv(sockfd, recbuf, MAXDATASIZE-1, 0)) == -1) 
    {
        perror("recv");
        exit(1);
    }
    recbuf[numbytes] = '\0';
    printf("Primit: %s\n", recbuf);

    close(sockfd);

    return 0;
}
