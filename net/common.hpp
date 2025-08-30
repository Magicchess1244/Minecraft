#ifndef __NETWORK_COMMON_H__
#define __NETWORK_COMMON_H__

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using sock_t = SOCKET;
#define CLOSESOCKET closesocket
#define GET_LAST_ERROR WSAGetLastError()

inline int initSockets() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

inline int cleanupSockets() { return WSACleanup(); }
inline int getLastError() { return WSAGetLastError(); }
#else
// Linux / POSIX sockets
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using sock_t = int;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSESOCKET close
#define GET_LAST_ERROR errno

inline int initSockets() { return 0; }
inline int cleanupSockets() { return 0; }
inline int getLastError() { return errno; }
#endif

#include <cstring>
#include <stdexcept>
#include <string>

class Socket {
   public:
    Socket(int domain, int type, int protocol) {
        fd = ::socket(domain, type, protocol);
        if (fd == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create socket, error: " + lastErrorString());
        }
    }

    explicit Socket(sock_t existing) : fd(existing) {
        if (fd == INVALID_SOCKET) {
            throw std::runtime_error("Invalid socket handle");
        }
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : fd(other.fd) { other.fd = INVALID_SOCKET; }
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            fd = other.fd;
            other.fd = INVALID_SOCKET;
        }
        return *this;
    }

    ~Socket() { close(); }

    void setReuseAddr(bool enable = true) {
        int opt = enable ? 1 : 0;
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt),
                         sizeof(opt)) < 0) {
            throw std::runtime_error("setsockopt(SO_REUSEADDR) failed: " + lastErrorString());
        }
    }

#ifdef SO_REUSEPORT
    void setReusePort(bool enable = true) {
        int opt = enable ? 1 : 0;
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&opt),
                         sizeof(opt)) < 0) {
            throw std::runtime_error("setsockopt(SO_REUSEPORT) failed: " + lastErrorString());
        }
    }
#endif

    sock_t get() const { return fd; }

    void close() {
        if (fd != INVALID_SOCKET) {
#ifdef _WIN32
            ::closesocket(fd);
#else
            ::close(fd);
#endif
            fd = INVALID_SOCKET;
        }
    }

   private:
    sock_t fd{INVALID_SOCKET};

    std::string lastErrorString() const {
#ifdef _WIN32
        return std::to_string(WSAGetLastError());
#else
        return std::strerror(errno);
#endif
    }
};

constexpr int MAX_PLAYERS = 8;

#endif  //  __NETWORK_COMMON_H__
