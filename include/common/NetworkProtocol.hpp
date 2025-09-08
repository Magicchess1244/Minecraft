#pragma once

#include "common.hpp"
#include "SocketRAII.hpp"
#include <string>
#include <vector>
#include <cstring>

enum class MessageType : uint8_t {
    SEED_REQUEST = 0x01,
    SEED_RESPONSE = 0x02,
    COLOR_REQUEST = 0x03,
    COLOR_RESPONSE = 0x04,
    PLAYER_UPDATE = 0x05,
    CHUNK_REQUEST = 0x06,
    CHUNK_DATA = 0x07,
    BLOCK_UPDATE = 0x08,
    PING = 0x09,
    PONG = 0x0A,
    DISCONNECT = 0xFF
};

struct NetworkMessage {
    MessageType type;
    uint32_t length;
    std::vector<uint8_t> data;

    NetworkMessage() : type(MessageType::PING), length(0) {}
    NetworkMessage(MessageType t, const std::vector<uint8_t>& d) : type(t), length(d.size()), data(d) {}
    NetworkMessage(MessageType t, const std::string& str) : type(t), length(str.length()), data(str.begin(), str.end()) {}
};

class NetworkProtocol {
private:
    SocketInitializer socket_init;

public:
    SocketRAII socket;

public:
    NetworkProtocol() = default;
    
    static bool parse_ipv4(const std::string& host, in_addr* out) {
        // Try inet_pton if available, otherwise fallback to inet_addr
        #ifdef _WIN32
        unsigned long addr = inet_addr(host.c_str());
        if (addr == INADDR_NONE) return false;
        out->s_addr = addr;
        return true;
        #else
        return inet_pton(AF_INET, host.c_str(), out) == 1;
        #endif
    }

    static bool recv_all(sock_t s, uint8_t* buf, size_t total) {
        size_t received_total = 0;
        while (received_total < total) {
            int r = recv(s, reinterpret_cast<char*>(buf + received_total), (int)(total - received_total), 0);
            if (r <= 0) return false;
            received_total += (size_t)r;
        }
        return true;
    }

    bool connect_to_server(const std::string& host, int port) {
        if (!socket_init.is_initialized()) {
            std::cerr << "Failed to initialize sockets" << std::endl;
            return false;
        }

        sock_t serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << GET_LAST_ERROR << std::endl;
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (!parse_ipv4(host, &serverAddr.sin_addr)) {
            std::cerr << "Invalid address: " << host << std::endl;
            CLOSESOCKET(serverSocket);
            return false;
        }

        if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to server: " << GET_LAST_ERROR << std::endl;
            CLOSESOCKET(serverSocket);
            return false;
        }

        socket.reset(serverSocket);
        std::cout << "Connected to server at " << host << ":" << port << std::endl;
        return true;
    }

    bool send_message(const NetworkMessage& message) {
        if (!socket.valid()) return false;

        // Send header (type + length)
        uint8_t header[5];
        header[0] = static_cast<uint8_t>(message.type);
        uint32_t len_n = htonl(message.length);
        std::memcpy(&header[1], &len_n, sizeof(uint32_t));

        if (send(socket.get(), reinterpret_cast<const char*>(header), 5, 0) != 5) {
            std::cerr << "Failed to send message header: " << GET_LAST_ERROR << std::endl;
            return false;
        }

        // Send data if any
        if (message.length > 0) {
            if (send(socket.get(), reinterpret_cast<const char*>(message.data.data()), (int)message.length, 0) != static_cast<int>(message.length)) {
                std::cerr << "Failed to send message data: " << GET_LAST_ERROR << std::endl;
                return false;
            }
        }

        return true;
    }

    bool receive_message(NetworkMessage& message) {
        if (!socket.valid()) return false;

        // Receive header
        uint8_t header[5];
        if (!recv_all(socket.get(), header, 5)) {
            // cannot differentiate cleanly, treat as failure
                std::cout << "Server disconnected" << std::endl;
            return false;
        }

        message.type = static_cast<MessageType>(header[0]);
        uint32_t len_n;
        std::memcpy(&len_n, &header[1], sizeof(uint32_t));
        message.length = ntohl(len_n);

        // Receive data if any
        if (message.length > 0) {
            message.data.resize(message.length);
            if (!recv_all(socket.get(), message.data.data(), message.length)) {
                std::cerr << "Failed to receive message data: " << GET_LAST_ERROR << std::endl;
                return false;
            }
        } else {
            message.data.clear();
        }

        return true;
    }

    bool is_connected() const {
        return socket.valid();
    }

    void disconnect() {
        if (socket.valid()) {
            NetworkMessage disconnect_msg(MessageType::DISCONNECT, "");
            send_message(disconnect_msg);
            socket.reset();
        }
    }
};
