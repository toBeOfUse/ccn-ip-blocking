/* timeserv.c - a socket-based time of day server
 */

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTNUM 8080 /* our time service phone number */
#define HOSTLEN 256
#define oops(msg)                                                              \
    {                                                                          \
        printf("error happened:\n");                                           \
        perror(msg);                                                           \
        exit(1);                                                               \
    }

int main(int ac, char* av[])
{
    // load allowed ips
    FILE * whitelist;
    whitelist = fopen("whitelist.txt", "r");
    if (whitelist == NULL){
        oops("whitelist file didn't work");
    }
    size_t line_length;
    char* addresses[32];
    int addresses_count = 0;
    char * line = NULL;
    while (getline(&line, &line_length, whitelist) !=- 1) {
        char * line_break = strchr(line, '\n');
        if (line_break){
            *line_break = 0;
        }
        addresses[addresses_count] = line;
        ++addresses_count;
        printf("registered allowed ip address: %s\n", line);
    }
    printf("registered %d addresses\n", addresses_count);

    fclose(whitelist);


    struct sockaddr_in saddr; /* build our address here */
    struct hostent* hp; /* this is part of our    */
    char hostname[HOSTLEN]; /* address 	         */
    int sock_id, sock_fd; /* line id, file desc     */
    FILE* sock_fp; /* use socket as stream   */
    char* ctime(); /* convert secs to string */
    time_t thetime; /* the time we report     */

    /*
     * Step 1: ask kernel for a socket
     */

    sock_id = socket(PF_INET, SOCK_STREAM, 0); /* get a socket */
    if (sock_id == -1)
        oops("socket");

    /*
     * Step 2: bind address to socket.  Address is host,port
     */

    bzero((void*)&saddr, sizeof(saddr)); /* clear out struct     */

    gethostname(hostname, HOSTLEN); /* where am I ?         */
    hp = gethostbyname(hostname); /* get info about host  */
    /* fill in host part    */
    bcopy((void*)hp->h_addr, (void*)&saddr.sin_addr, hp->h_length);
    saddr.sin_port = htons(PORTNUM); /* fill in socket port  */
    saddr.sin_family = AF_INET; /* fill in addr family  */

    if (bind(sock_id, (struct sockaddr*)&saddr, sizeof(saddr)) != 0)
        oops("bind");

    /*
     * Step 3: allow incoming calls with Qsize=1 on socket
     */

    if (listen(sock_id, 1) != 0)
        oops("listen");

    /*
     * main loop: accept(), write(), close()
     */

	printf("listening for calls...\n");

    while (1) {
        struct sockaddr client_address;
        int client_address_length;
        sock_fd = accept(sock_id, &client_address, &client_address_length); /* wait for call */
        printf("Wow! got a call!\n");
        struct sockaddr_in* client_address_in = (struct sockaddr_in*)&client_address;
        struct in_addr final_client_address = client_address_in->sin_addr;
        char address_as_str[INET_ADDRSTRLEN];
        inet_ntop( AF_INET, &final_client_address, address_as_str, INET_ADDRSTRLEN );
        printf("client has address: %s\n", address_as_str);
        int allowed = 0;
        for (int i=0; i<addresses_count; i++){
            if (strcmp(address_as_str, addresses[i]) == 0) {
                allowed = 1;
            }
        }

        if (!allowed){
            printf("hanging up on client\n");
            fclose(sock_fp);
            continue;
        } else {
            printf("client is allowed\n");
        }


        if (sock_fd == -1)
            oops("accept"); /* error getting calls  */

        sock_fp = fdopen(sock_fd, "w"); /* we'll write to the   */
        if (sock_fp == NULL) /* socket as a stream   */
            oops("fdopen"); /* unless we can't      */

        thetime = time(NULL); /* get time             */
        /* and convert to strng */
        fprintf(sock_fp, "The time here is: ");
        fprintf(sock_fp, "%s", ctime(&thetime));
        fclose(sock_fp); /* release connection   */
    }
}
