/**  In-Situ Network Tester.

     Chris Matthews
     University of Victoria
     cmatthew@cs.uvic.ca

*/

/* Setttings */
#define TCP_PORT 9999
#define UDP_PORT 9999
#define KNOWN_DNS_NAME "atmcc.trans-cloud.net"
#define KNOWN_DNS_ADDRESS "1.2.3.4"
#define SERVER_NAME "hypervisor-ilo.cs.uvic.ca"


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
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


enum test_status { NOT_DONE = 0, PASSED, WARNING, NO_RESULT, FAILED };


char * status_name[] = {"Not Completed", "Passed", "Warning", "No Result", "Failed"};


typedef struct test_result_s {
    enum test_status status;
    const char * problem;
    const char * description;
} result;


static void print_result(result * r) {
    printf("%s: %s\n", status_name[r->status], r->problem);
    printf("%s\n", r->description);
    
}


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


#define perror(x)  return make_result(FAILED, x,"")


static void print_result(result * r) ;

#define LINK_LOCAL_PREFIX "169.254."
#define PRIVATE_PREFIX1 "10."
#define PRIVATE_PREFIX3 "192.168."

#define starts_with(x,y) (strncmp(x, y, strlen(x)) == 0)


/** Check the condition of the local IP address.

Is is private, is it routable

*/
static result * test_ip_address(void) {
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[64];

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

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf))) {
            perror("inet_ntop failed");
        } else {
            // Are we a private IP address
            if ( starts_with(LINK_LOCAL_PREFIX, buf) ) {
                return make_result( FAILED, "Private Link Local IP address found", buf);
            }
            // Are we a non-routable IP
            if ( starts_with(PRIVATE_PREFIX1, buf)) {
                #warning "20-bit private addressing check is not implemented yet"
                return make_result( PASSED, "Private non-routable IP address found", buf);
            }
            // Are we a private IP address
            if ( starts_with(PRIVATE_PREFIX3, buf)) {
                return make_result( PASSED, "Private non-routable IP address found", buf);
            }
        }
    }
    result * r =  make_result(PASSED, "Routable IP addresses found.", strdup(buf));
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


static result * test_tcp_callback(void) {

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
    
    char * port = NULL;
    (void) asprintf(&port, "%d", TCP_PORT + 1);
    unsigned int rc = write( sockfd, port , strlen(port)+1);

    if (rc != strlen(port) + 1) {
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



int main(void)
{
    result * r = NULL;
    print_result(r = test_ip_address());
    free(r);
    print_result(r = test_dns_resolution());
    free(r);
    print_result(r = test_echo_server());
    free(r);
    print_result(r = test_tcp_callback());
    free(r);
    return EXIT_SUCCESS;
}
