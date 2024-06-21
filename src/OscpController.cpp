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

void OscpController::checkAndSendDimensionalityChange(const char* prevDimens, const char* currDimens, void *buffer,
                                                      int size) {
    if (prevDimens == currDimens) {
        return;
    }
    int prevDimensInBinary[] = {0, 0, 0, 0};
    int currDimensInBinary[]  = {0, 0, 0, 0};

    convert_dimensions(prevDimens, prevDimensInBinary);
    convert_dimensions(currDimens, currDimensInBinary);
    const auto p1 = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            p1.time_since_epoch()).count();

    OSCPP::Client::Packet packet(buffer, size);
    packet.openBundle(timestamp)
            .openMessage("/s_dimensionality_change",  OSCPP::Tags::array(8) * 2)
                .openArray()
                    .string("x")
                    .int32(prevDimensInBinary[0])
                    .string("y")
                    .int32(prevDimensInBinary[1])
                    .string("z")
                    .int32(prevDimensInBinary[2])
                    .string("w")
                    .int32(prevDimensInBinary[3])
                .closeArray()
                .openArray()
                    .string("x")
                    .int32(currDimensInBinary[0])
                    .string("y")
                    .int32(currDimensInBinary[1])
                    .string("z")
                    .int32(currDimensInBinary[2])
                    .string("w")
                    .int32(currDimensInBinary[3])
                .closeArray()
            .closeMessage()
            .closeBundle();

    send(buffer, packet);
}

void OscpController::convert_dimensions(const char *prevDimens, int *prevDimensions) const {
    if (strchr(prevDimens, 'x')) {
        prevDimensions[0] = 1;
    }
    if (strchr(prevDimens, 'y')) {
        prevDimensions[1] = 1;
    }
    if (strchr(prevDimens, 'z')) {
        prevDimensions[2] = 1;
    }
    if (strchr(prevDimens, 'y')) {
        prevDimensions[3] = 1;
    }
}
