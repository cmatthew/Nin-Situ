/**  In-Situ Network Tester.

     Chris Matthews
     University of Victoria
     cmatthew@cs.uvic.ca

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>
#include <netdb.h>

#define perror(x)  return make_result(FAILED, x,"")

enum test_status { NOT_DONE = 0, PASSED, WARNING, NO_RESULT, FAILED };

char * status_name[] = {"Not Completed", "Passed", "Warning", "No Result", "Failed"};


typedef struct test_result_s {
    enum test_status status;
    char * problem;
    char * description;
} result;

void print_result(result * r) {
    printf("%s: %s\n", status_name[r->status], r->problem);
    printf("%s\n", r->description);
    
}

result * get_blank_result() {
    result * r = (result *) malloc(sizeof(result));
    memset(r, 0, sizeof(result));
    return r;

}

result * make_result( enum test_status tst, char * problem, char * description) {
    result * r = get_blank_result();
    r->status = tst;
    r->problem = problem;
    r->description = description;
    return r;
}

#define LINK_LOCAL_PREFIX "169.254."
#define PRIVATE_PREFIX1 "10."
#define PRIVATE_PREFIX3 "192.168."

#define starts_with(x,y) strncmp(x, y, strlen(x)) == 0


/** Check the condition of the local IP address.

Is is private, is it routable

*/
result * test_ip_address() {
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


result * test_dns_resolution() {

    struct hostent *hp = gethostbyname("atmcc.trans-cloud.net");

    if (hp == NULL) {
        perror("DNS resolution failed");
    }

    struct in_addr* address = (struct in_addr *) hp -> h_addr_list[0];
    char * str_address = inet_ntoa( *( struct in_addr*) address );
    if (strcmp(str_address, "1.2.3.4")==0){
        return make_result(PASSED, "DNS resolution worked","query of atmcc."
                           "trans-cloud.net returned the expected 1.2.3.4");
    } else {
        return make_result(FAILED, "DNS resolution failed",
                           "query of atmcc.trans-cloud.net did not return"
                           " the expected 1.2.3.4");
    }

    return make_result(PASSED, "DNS resolution works","");

}

#define ECHO_MESSAGE "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"


result * test_echo_server() {

    struct hostent     *he;
    struct sockaddr_in  server;
    int sockfd = 0;
    memset(&server, 0, sizeof(server));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("opening socket failed");
    }
    /* resolve localhost to an IP (should be 127.0.0.1) */
    if ((he = gethostbyname("localhost")) == NULL) {
        perror( "error resolving the hostname");
    }

    /*
     * copy the network address part of the structure to the 
     * sockaddr_in structure which is passed to connect() 
     */
    memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(9999);
    
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
    int rc2 = read( sockfd, &buf, 65);
    if (rc2 != strlen(ECHO_MESSAGE) + 1) {
        return make_result(FAILED, "Could not read data from the server","");
    }

    if (strcmp(ECHO_MESSAGE, buf)==0) {
        return make_result(PASSED, "TCP Echo test works","I message was relayed by the server");
    } else {
        return make_result(FAILED, "TCP Echo test failed","Payload was not as expected");
    }
}



int main(int argc, char *argv[])
{

    print_result(test_ip_address());

    print_result(test_dns_resolution());

    print_result(test_echo_server());

    return 0;
}
