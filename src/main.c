/**  In-Situ Network Tester.

     Chris Matthews
     University of Victoria
     cmatthew@cs.uvic.ca

*/

/* Setttings */

/* what port to do the TCP tests on.  Server must wait on this. */
#define TCP_PORT 9999
#define TCP_PORT_STR "9999"

/* what port to do the UDP tests on. */
#define UDP_PORT 9999

/* This DNS name is preregistered, so it should resolve if DNS is working. */
#define KNOWN_DNS_NAME "atmcc.trans-cloud.net"

/* This is the address that the DNS name should resolve to.  */
#define KNOWN_DNS_ADDRESS "1.2.3.4"

/* This is the name of the server to do the TCP and UDP tests with.  */
#define SERVER_NAME "hypervisor-ilo.cs.uvic.ca"

/*Default TCP accept connection backlog.  */
#define TCP_SERVER_BACKLOG 10

/* use this so asprintf is here. */
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


void *get_in_addr(struct sockaddr *sa);


/* As each test is run, we would like to know what its status was. */
enum test_status { NOT_DONE = 0, PASSED, WARNING, NO_RESULT, FAILED };


/*  How do each of test_status map to written names for printing on screen.  */
char * status_name[] = {"Not Completed", "Passed", "Warning", "No Result", "Failed"};


/* The details to keep about each test.  */
typedef struct test_result_s {
    enum test_status status;  /* what happened. */
    const char * problem;  /* If there was a problem, what was it?  */
    const char * description;  /* What are the specific details of the problem?  */
} result;


/*  Write the result R out to a file handle, FD. */
static void output_result(struct _IO_FILE * fd, result * r) {
    fprintf(fd, "%s: %s\n", status_name[r->status], r->problem);
    fprintf(fd, "%s\n", r->description);

}


/* Print the result R out to stdout.  */
static void print_result(result * r) {
    output_result(stdout, r);
}


/*  Give the test status TST, PROBLEM, and DESCRIPTION, build a result struct.  */
static result * make_result( enum test_status tst, const char * problem, const char * description) {
    result * r = (result *) malloc(sizeof(result));
    assert(r != NULL);
    memset(r, 0, sizeof(result));
    r->status = tst;
    r->problem = NULL;
    r->problem = problem;
    r->description = description;
    return r;
}


/* Perror doesnt really work in Lind, so we want its uses here to just fail
   the test and continue.  */
#define perror(x)  return make_result(FAILED, x, strerror(errno))


static void print_result(result * r) ;

#define LINK_LOCAL_PREFIX "169.254."
#define PRIVATE_PREFIX1 "10."
#define PRIVATE_PREFIX3 "192.168."

#define starts_with(x,y) (strncmp(x, y, strlen(x)) == 0)


/** Check the condition of the local IP address.

Is is private, is it routable?

*/
static result * test_ip_address(void) {
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    ssize_t buf_siz = 64 * sizeof(char);
    char * buf = (char *) malloc(buf_siz);
    memset(buf, 0, buf_siz);
    if(getifaddrs(&myaddrs) != 0) {
        perror("getifaddrs failed.");
    }

    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        if (!(ifa->ifa_flags & IFF_UP)) {
            continue;
        }

        switch (ifa->ifa_addr->sa_family) {
            case AF_INET: 
                {
                struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
                in_addr = &s4->sin_addr;
                break;
                }

            default:
                continue;
        }

        if (inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, buf_siz) == NULL) {
            freeifaddrs(myaddrs);
            perror("inet_ntop failed");
        } else {
            // Are we a private IP address
            if ( starts_with(LINK_LOCAL_PREFIX, buf) ) {
                freeifaddrs(myaddrs);
                return make_result( FAILED, "Private Link Local IP address found", buf);
            }
            // Are we a non-routable IP
            if ( starts_with(PRIVATE_PREFIX1, buf)) {
                #warning "20-bit private addressing check is not implemented yet"
                freeifaddrs(myaddrs);
                return make_result( PASSED, "Private non-routable IP address found", buf);
            }
            // Are we a private IP address
            if ( starts_with(PRIVATE_PREFIX3, buf)) {
                freeifaddrs(myaddrs);
                return make_result( PASSED, "Private non-routable IP address found", buf);
            }
        }
    }
    result * r =  make_result(PASSED, "Routable IP addresses found.", strdup(buf));
    free(buf);
    freeifaddrs(myaddrs);
    return r;
}


static result * test_dns_resolution(void) {

    struct hostent *hp = gethostbyname(KNOWN_DNS_NAME);

    if (hp == NULL) {
        perror("DNS resolution failed");
    }

    struct in_addr* address = (struct in_addr *) hp -> h_addr_list[0];
    char * str_address = inet_ntoa( *( struct in_addr*) address );
    if (strcmp(str_address, KNOWN_DNS_ADDRESS)==0){
        return make_result(PASSED, "DNS resolution worked","query of " KNOWN_DNS_NAME
                           " returned the expected 1.2.3.4");
    } else {
        return make_result(FAILED, "DNS resolution failed",
                           "query of " KNOWN_DNS_NAME " did not return"
                           " the expected 1.2.3.4");
    }

    return make_result(PASSED, "DNS resolution works","");

}


#define ECHO_MESSAGE "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"


static result * test_echo_server(void) {

    struct hostent     *he = NULL;
    struct sockaddr_in  server;
    int sockfd = -1;

    memset(&server, 0, sizeof(server));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("opening socket failed");
    }
    /* resolve localhost to an IP (should be 127.0.0.1) */
    if ((he = gethostbyname(SERVER_NAME)) == NULL) {
        perror( "error resolving the hostname");
    }

    /*
     * copy the network address part of the structure to the 
     * sockaddr_in structure which is passed to connect() 
     */
    memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(TCP_PORT);
    
    /* connect */
    if (connect(sockfd, (struct sockaddr *) &server, sizeof(server))) {
        perror("error connecting to server");
    }

    int rc = write( sockfd, ECHO_MESSAGE, strlen(ECHO_MESSAGE)+1);

    if (rc != strlen(ECHO_MESSAGE) + 1) {
        return make_result(FAILED, "Could not send data to the server","");
    }

    char buf[65];
    memset(&buf, 0, 65);

    int read_len = read( sockfd, &buf, 65);

    if (read_len != strlen(ECHO_MESSAGE) + 1) {
        return make_result(FAILED, "Could not read data from the server","");
    }

    if (strcmp(ECHO_MESSAGE, buf)==0) {
        return make_result(PASSED, "TCP Echo test works","I message was relayed by the server");
    } else {
        return make_result(FAILED, "TCP Echo test failed","Payload was not as expected");
    }

}


/* get sockaddr, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/** http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleserver 
  Used by the TCP callback test, setup a socket listening on port_number, so 
  that all is needed to be done is call accept on that port to 
*/
static int setup_waiting_socket_on_port(char * port_number) {

    int sockfd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage; // connector's address information
    int yes=1;
    int rv;
    char buf[128];
    memset(buf, 0, 128);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port_number, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        printf("checking for connections on %s\n", inet_ntop(p->ai_family,get_in_addr(p->ai_addr), buf, sizeof(buf)));
        
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            printf("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            printf("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            printf("server: bind");
            continue;
        }
     
        if (fcntl(sockfd, F_SETFD, O_NONBLOCK) == -1) {
            printf("Error:  TCP socket setup, making socket nonblocking did not work.");
            exit(EXIT_FAILURE);
        }

        printf("Waiting for connections on %s\n", inet_ntop(p->ai_family,get_in_addr(p->ai_addr), buf, sizeof(buf)));
        
    }

    
    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, TCP_SERVER_BACKLOG) == -1) {
        printf("listen");
        exit(1);
    }

    printf("server: waiting for connections on %s...\n", port_number);

    return sockfd;

}


static result * test_tcp_callback(void) {

    /* Init things */
    struct hostent     *he = NULL;
    struct sockaddr_in  server;
    int sockfd = -1;
    int waiting_sockfd = -1;
    memset(&server, 0, sizeof(server));

    /* setup a waiting socket which we can call back too */
    waiting_sockfd = setup_waiting_socket_on_port("10000");

    /* Now call out to the server, asking for a call back on the waiting port.*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("opening socket failed");
    }
    /* resolve localhost to an IP (should be 127.0.0.1) */
    if ((he = gethostbyname(SERVER_NAME)) == NULL) {
        perror( "error resolving the hostname");
    }

    /*
     * copy the network address part of the structure to the 
     * sockaddr_in structure which is passed to connect() 
     */
    memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(TCP_PORT);
    
    /* connect */
    if (connect(sockfd, (struct sockaddr *) &server, sizeof(server))) {
        perror("error connecting to server");
    }
    
    char * port = NULL;
    if (asprintf(&port, "%d", TCP_PORT + 1) == -1) {
        perror("Formatting prot string failed.");
    }
    unsigned int rc = write( sockfd, port , strlen(port)+1);

    if (rc != strlen(port) + 1) {
        return make_result(FAILED, "Could not send data to the server","");
    }
    free(port);
    
    char buf[65];
    memset(&buf, 0, 65);

    struct sockaddr_storage their_addr; // connector's address information
    memset(&their_addr, 0, sizeof(their_addr));
    char s[INET6_ADDRSTRLEN];

    socklen_t sin_size = sizeof(their_addr);
    int new_fd = -1;
    printf("accept: %d %p\n", waiting_sockfd, (void *) &their_addr);

    int timeout = 4;

    new_fd = accept(waiting_sockfd, (struct sockaddr *)&their_addr, &sin_size);

    while (timeout > 0 && new_fd == -1 ) {
        sleep(1);
        timeout--;
        new_fd = accept(waiting_sockfd, (struct sockaddr *)&their_addr, &sin_size);
    }

   if (new_fd == -1) {
        perror("non blocking accept in callback did not work.  Callback has failed.");
    }
    
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
    printf("server: got connection from %s\n", s);
    
    if (recv(new_fd, buf, 65, 0) == -1) {
        perror("recv in TCP callback");
    }
    printf("Callback got: %s\n", buf);
    close(new_fd);

    return make_result(PASSED, "TCP callback test works","A callback was made on port " TCP_PORT_STR);

    
}


int main(void) {
    result * r = NULL;
    print_result(r = test_ip_address());
    free((void*)r->description);
    free(r);
    print_result(r = test_dns_resolution());
    free(r);
    print_result(r = test_echo_server());
    free(r);
    print_result(r = test_tcp_callback());
    free(r);
    return EXIT_SUCCESS;
}


#undef perror 
