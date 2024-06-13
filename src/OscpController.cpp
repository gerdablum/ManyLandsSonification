//
// Created by Alina on 17.05.2024.
//

#include <chrono>
#include "OscpController.h"

void OscpController::start() {
    // create socket
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
        std::cout << "socket() failed with error code: " << WSAGetLastError() << "\n";
        exit(EXIT_FAILURE);
    }

    // setup address structure
    memset((char*)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.S_un.S_addr = inet_addr(SERVER);
}

void OscpController::stop() {
    closesocket(client_socket);
}


void OscpController::sendStartMessage(void *buffer, int size) {

    const auto p1 = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            p1.time_since_epoch()).count();

    OSCPP::Client::Packet packet(buffer, size);

    packet.openBundle(timestamp)
            .openMessage("/s_control", 1)
            .int32(1)
            .closeMessage()
            .closeBundle();

    send(buffer, packet);
}

void OscpController::send(const void *buffer, const OSCPP::Client::Packet &packet) const {
    if(sendto(client_socket, (const char*) buffer, packet.size(), 0, (sockaddr*)&server, sizeof(sockaddr_in)) == SOCKET_ERROR) {
        std::cout << "sendto() failed with error code: " << WSAGetLastError() << "\n";
        exit(EXIT_FAILURE);
    }
}

void OscpController::sendStopMessage(void *buffer, int size) {
    const auto p1 = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            p1.time_since_epoch()).count();

    OSCPP::Client::Packet packet(buffer, size);

    packet.openBundle(timestamp)
            .openMessage("/s_control", 1)
            .int32(0)
            .closeMessage()
            .closeBundle();

    send(buffer, packet);
}

void OscpController::sendFrequencyChange(float frequency, void *buffer, int size) {
    const auto p1 = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            p1.time_since_epoch()).count();

    OSCPP::Client::Packet packet(buffer, size);

    packet.openBundle(timestamp)
            .openMessage("/s_freq", 1)
            .float32(frequency)
            .closeMessage()
            .closeBundle();

    send(buffer, packet);
}
