#pragma once

#include "network.hpp"
#include <memory>
#include <functional>

class SocketRAII {
private:
    sock_t socket_fd;
    bool is_valid;

public:
    explicit SocketRAII(sock_t fd = INVALID_SOCKET) : socket_fd(fd), is_valid(fd != INVALID_SOCKET) {}
    
    ~SocketRAII() {
        if (is_valid) {
            CLOSESOCKET(socket_fd);
        }
    }

    // Move constructor
    SocketRAII(SocketRAII&& other) noexcept : socket_fd(other.socket_fd), is_valid(other.is_valid) {
        other.is_valid = false;
        other.socket_fd = INVALID_SOCKET;
    }

    // Move assignment
    SocketRAII& operator=(SocketRAII&& other) noexcept {
        if (this != &other) {
            if (is_valid) {
                CLOSESOCKET(socket_fd);
            }
            socket_fd = other.socket_fd;
            is_valid = other.is_valid;
            other.is_valid = false;
            other.socket_fd = INVALID_SOCKET;
        }
        return *this;
    }

    // Delete copy constructor and assignment
    SocketRAII(const SocketRAII&) = delete;
    SocketRAII& operator=(const SocketRAII&) = delete;

    sock_t get() const { return socket_fd; }
    bool valid() const { return is_valid; }
    
    void reset(sock_t fd = INVALID_SOCKET) {
        if (is_valid) {
            CLOSESOCKET(socket_fd);
        }
        socket_fd = fd;
        is_valid = (fd != INVALID_SOCKET);
    }

    operator sock_t() const { return socket_fd; }
};

// RAII wrapper for socket initialization
class SocketInitializer {
private:
    bool initialized;

public:
    SocketInitializer() : initialized(false) {
        if (initSockets() == 0) {
            initialized = true;
        }
    }

    ~SocketInitializer() {
        if (initialized) {
            cleanupSockets();
        }
    }

    // Delete copy and move
    SocketInitializer(const SocketInitializer&) = delete;
    SocketInitializer& operator=(const SocketInitializer&) = delete;
    SocketInitializer(SocketInitializer&&) = delete;
    SocketInitializer& operator=(SocketInitializer&&) = delete;

    bool is_initialized() const { return initialized; }
};
