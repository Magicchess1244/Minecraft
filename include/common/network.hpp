#pragma once

#ifdef _WIN32
    #include <WinSock2.h>
    #include <ws2tcpip.h>
    //#pragma comment(lib, "ws2_32.lib")

    using sock_t = SOCKET;
    #define CLOSESOCKET closesocket
    #define GET_LAST_ERROR WSAGetLastError()

    inline int initSockets() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2,2), &wsaData);
    }

    inline int cleanupSockets() {
        return WSACleanup();
    }

#else
    // Linux / POSIX sockets
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>

    using sock_t = int;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
    #define CLOSESOCKET close
    #define GET_LAST_ERROR errno

    inline int initSockets() { return 0; }
    inline int cleanupSockets() { return 0; }
#endif
