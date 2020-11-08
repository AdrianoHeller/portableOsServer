/*
 * Checking if the system is Windows or Unix-like
 */

#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <string.h>

/*
 * Defining Some Macros to Parse Between Windows and Unix-like
 */

#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

using namespace std;

int main(){
// Case in a Windows Env, Use WSAStartup v2.2 to init Winsocket
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2,2),&d)) {
        cout << stderr << "Failed to initialize.\n";
        return 1;
    }
#endif

    cout << "Configuring local address...\n";

    addrinfo hints;
    /**
     * Resetting hints instance to value 0,
     * choosing IPv4 as defaults and
     * leave getaddrinfo to fill and set the address props
     */
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // Set to Wildcard

    addrinfo* bind_address;
    // Fixing 8080 listening port to the service and addressing the receiving pointers
    getaddrinfo(0,"8080", &hints, &bind_address);

    cout << "Creating Socket...\n";
    // Creating instance of SOCKET struct
    SOCKET socket_listen;
    // Initializing listener pointing the bind_address pointer inner values of family,
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    // Checking whether socket is valid or not
    if(!ISVALIDSOCKET(socket_listen)){
        cout << stderr << " Socket " << GETSOCKETERRNO() << "failed.";
        return 1;
    };

    cout << "Binding Socket to Local Address...\n";
    // Binding Listening socket to local interface address
    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)){
      cout << stderr << " Bind failed " << GETSOCKETERRNO() << ".\n";
      return 1;
    };
    // Releasing memory address
    freeaddrinfo(bind_address);

    cout << "Listening...\n";
    // Stablishing a QUEUE of 10 peers before closing
    if(listen(socket_listen, 10) < 0){
      cout << stderr << "Listen failed " << GETSOCKETERRNO() << ".\n";
        return 1;
    };

    cout << "Waiting for connection...\n";
    /**
     * Creating a storage instance to get the client address,
     * allocate memory with the client length
     * and set ready to accept connections casting client address to a socketaddr pointer
     */
    sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen, (sockaddr*)&client_address, &client_len);
    // Checking if the Client socket is valid
    if(!ISVALIDSOCKET(socket_client)){
      cout << stderr << "Invalid Socket. Accept failed " << GETSOCKETERRNO() << ".\n";
        return 1;
    };

    cout << "Client is connected... ";
    // Creating string buffer of 100 bytes to receive the address
    char address_buffer[100];
    // Querying the IPv4 ip numeric address
    getnameinfo((sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);

    cout << address_buffer << "\n";

    cout << "Reading request...\n";
    // Starting the request object with 1024 bytes memsize
    char request[1024];
    // Starting the receiving object with recv
    int bytes_received = recv(socket_client, request, 1024, 0);
    // Using std namespace and format string to print the output
    cout << "Received " << bytes_received << " bytes.\n";
    printf("%.*s",bytes_received, request);

    cout << "Sending response...\n";
    // Creating the response Header
    const char* response =
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Local time is: ";
    // Starting the response object
    int bytes_sent = send(socket_client, response, strlen(response), 0);

    cout << " Sent " << bytes_sent << "of " << (int)strlen(response) << " bytes." << endl;
    // Calling the ctime header built-in functions to address time data to pointers
    time_t timeStamp;
    time(&timeStamp);
    char* time_msg = ctime(&timeStamp);
    // Casting timestamp bytes
    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);

    cout << "Sent " << bytes_sent << "of " << (int)strlen(time_msg) << " bytes." << endl;

    cout << "Closing connection...\n";
    // Closing client socket
    CLOSESOCKET(socket_client);

    cout << "Close listening socket...\n";
    // Closing server socket
    CLOSESOCKET(socket_listen);
// If in Windows env let SO clean up the sockets
#if defined(_WIN32)
    WSACleanup();
#endif

    cout << "Finished.\n";

    return 0;
};