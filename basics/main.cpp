// #include <stdio.h>
// #include <sys/types.h>
#include <sys/socket.h>
// #include <netdb.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <unistd.h>
// #include <string.h>
// #include <netinet/in.h>
#include <iostream>

// #include <stdio.h>
// #include <string.h>
// #include <time.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
#include <netinet/in.h>


/*
Some notes on the difference between 'const' and 'volitile'
    - https://publications.gbdirect.co.uk//c_book/chapter8/const_and_volatile.html


Order of function calls on client and server sides: https://www.youtube.com/watch?v=b_TUtu3PemQ
Details: https://docs.freebsd.org/en/books/developers-handbook/sockets/
    - I found useful -> where I learnt most of what's on this page.

  On the client side:
    - int socket(int domain, int type, int protocol);
        - int domain: what protocol family is being used (options declared in sys/socket.h)
            - use PF_INET (Protocol Family _ Internetwork ) for UDP, TP and other internet protocols (IPv4)
        - int type: 5 options (in sys/socket.h) all start with "SOCK_"
            - most common is SOCK_STREAM => asking for a "reliable tream delivery service" (which is TCP when used with PF_INET)
            - SOCK_DGRAM => asking for a "connectionless datagram delivery sservice" (which is UDP when used with PF_INET)
            - SOCK_RAW => asking to be in charge of the low-level protocols (e.g. IP) or network interfaces (e.g. Ethernet)
        - int protocol: depends on previous two arguments & is not always meaningful.
            - Use 0 for now
            - TODO: What does int protocol control/represent.

    - struct sockaddr 
            struct sockaddr {
                __uint8_t       sa_len;         // total length 
                sa_family_t     sa_family;      // [XSI] address family
                char            sa_data[14];    // [XSI] addr value (actually larger)
            };
            - "used by kernel to store most addresses"
        - sa_family_t sa_family -> address family -> decides how sa_data will be used (hence why it can be defined vaguely)
            - AF_INET used for IP
            - whenever the address family is AF_INET, 'struct sockaddr_in' in netinet/in.h can be used whenever 'sockaddr' is expected

        - char sa_data[14] is defined vaguly deliberately (can be larger) since sockets can be used for "interprocess communications" (which IP (the internet) is only one of the uses).
        - 

    - struct sockaddr_in (defined in netinet/in.h)
            - Used whenever the 'address family' is AF_INET
            - used wherever 'sockaddr' is expected
        - struct in_addr sin_addr 
            - defined as a struct (in_addr - also defined in netinet/in.h)
            - struct in_addr only contains 'in_addr_t s_addr' a 4-byte (32-bit) integer
                - represents the Internet adress, which is kept in a struct for historical reasons
                - 192.43.244.18 is just convenient notation - expresses a 32-bit integer listing all of its 8-bit bytes (starts with the 'most significant' one)
                - Don't know whether the computer we are talking to stores data 'most significant byte' (MSB) or 'least significant byte' (LSB) first.
            - since the socket only knows 'struct sockaddr' (sa_len, sa_family, sa_data), the extra fields in 'struct sockaddr_in' has to contained in sa_data[]
            - depending on the address family, sockets just forwards that data to its destination
            - sockets do not care about port number (does not interpret it in anyway)
                -> only needs to forward it as data so the receiving computer knows what service we are asking for.
            - sockets do not care about the IP address 
                -> forwards it on as data
                -> IP address is only used to tell everyone on the way where to send our data.
                -> hence why it is our responsibilty (as the programmer) to care about the byte ordering (and not sockets)
                - Host (byte) order = the byte order our computer uses
                - Network (byte) order = byte order mulit-byte data is sent over IP
                - convention = send multi-byte data over IP MSB first (the Network order)
                - use htons & htonl to convert from host order to newtork order (for short and long respectively) (host to network short/long)
                    - ntohs & ntohl for the over way (network to host short/long)
                    -> ensures the data will be in the ccorrect order, no matter on what system (only changes order when necessary)
   
    - int connect(int s, const struct sockaddr *name, socklen_t namelen);
            - success = 0, otherwise = -1 (and stores the error code in errno)
        - int s = socket = the value that is returned by the 'int socket(int domain, int type, int protocol)' function.
        - const struct sockaddr *name = pointer to a sockaddr
        - socklen_t namelen = number of bytes in out sockaddr structure.



read/write = std operations universal File discriptors
recv/send  = only work on socket descriptors
            

Typical Client functions
    - socket()
    - connection()
    - write()
    - read()
    - close()

Typical Server functions
    - int bind(int s, const struct sockaddr *addr, socklen_t addrlen);
            - tells sockets which port we want to serve (i.e. bind to)
        - int s => socket
        - const struct sockaddr *addr =>
        - socklen_t addrlen =>


    - int listen(int s, int backlog);
        - int s => socket
        - int backlog => the maximum length of the queue waiting for the server to respond

    - int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
            - How the server 'accepts' an incoming connection
            - The connection remains active until the client or server hangs up 
        - int s => socket (value returned from 'int socket(...)')
        - struct sockaddr *addr =>
        - socklen_t *addrlen =>
        - returns a new socket (int) -> will use this socket to communicate with the client
            - the original socket continues to listen for more requests (until we close it)
            - the new socket is fully connected -> can only use it for communications -> cannot pass it to listen again 




Example: for the daytime protocol (get time)
    - want to up TCP/IP, so sin_family = AF_INET
    - writes to port 13, so sin_port = 13
    - IP address = 192.43.244.18


*/


int main() {
    std::cout << "Hello" << std::endl;

    int domain = PF_INET;
    int type = SOCK_STREAM;
    int protocol = 0;
    int s = socket(domain, type, protocol);  // int socket(int domain, int type, int protocol);

    std::cout << "domain = " << domain;
    std::cout << "\ntype = " << type;
    std::cout << "\nprotocol = " << protocol;
    std::cout << "\ns = " << s << std::endl;  // at this point, only created the socket and left it hanging there therefore expect -1

    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(13);
    sa.sin_addr.s_addr = htonl((((((192 << 8) | 43) << 8) | 244) << 8) | 18);

    std::cout << "port: " << sa.sin_port;
    std::cout << "\nsin_addr.s_addr = " << sa.sin_addr.s_addr << std::endl;

    sa.sin_addr.s_addr = (((((192 << 8) | 43) << 8) | 244) << 8) | 18;
    std::cout << "sin_addr.s_addr = " << sa.sin_addr.s_addr << std::endl;

    return 0;
}
