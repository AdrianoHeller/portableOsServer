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

#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2,2),&d)) {
        cout << stderr << "Failed to initialize.\n";
        return 1;
    }
#endif

    cout << "Configuring local address...\n";

    addrinfo hints;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* bind_address;

    getaddrinfo(0,"8080", &hints, &bind_address);

    cout << "Creating Socket...\n";

    SOCKET socket_listen;

    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

    if(!ISVALIDSOCKET(socket_listen)){
        cout << stderr << " Socket " << GETSOCKETERRNO() << "failed.";
        return 1;
    };

    cout << "Binding Socket to Local Address...\n";

    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)){
      cout << stderr << " Bind failed " << GETSOCKETERRNO() << ".\n";
      return 1;
    };

    freeaddrinfo(bind_address);

    cout << "Listening...\n";

    if(listen(socket_listen, 10) < 0){
      cout << stderr << "Listen failed " << GETSOCKETERRNO() << ".\n";
        return 1;
    };

    cout << "Waiting for connection...\n";

    sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen, (sockaddr*)&client_address, &client_len);

    if(!ISVALIDSOCKET(socket_client)){
      cout << stderr << "Invalid Socket. Accept failed " << GETSOCKETERRNO() << ".\n";
        return 1;
    };

    cout << "Client is connected... ";

    char address_buffer[100];

    getnameinfo((sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);

    cout << address_buffer << "\n";

    cout << "Reading request...\n";

    char request[1024];
    int bytes_received = recv(socket_client, request, 1024, 0);

    cout << "Received " << bytes_received << " bytes.\n";
    printf("%.*s",bytes_received, request);

    cout << "Sending response...\n";

    const char* response =
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Local time is: ";

    int bytes_sent = send(socket_client, response, strlen(response), 0);

    cout << " Sent " << bytes_sent << "of " << (int)strlen(response) << " bytes." << endl;

    time_t timeStamp;
    time(&timeStamp);
    char* time_msg = ctime(&timeStamp);

    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);

    cout << "Sent " << bytes_sent << "of " << (int)strlen(time_msg) << " bytes." << endl;

    cout << "Closing connection...\n";

    CLOSESOCKET(socket_client);

    cout << "Close listening socket...\n";

    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif

    cout << "Finished.\n";

    return 0;
};