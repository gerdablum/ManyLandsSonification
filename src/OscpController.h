//
// Created by Alina on 17.05.2024.
//

#ifndef MANYLANDS_OSCPCONTROLLER_H
#define MANYLANDS_OSCPCONTROLLER_H
#include <iostream>
#include <oscpp/client.hpp>
#include <winsock2.h>

#define SERVER "127.0.0.1"  // or "localhost" - ip address of UDP server
#define BUFLEN 512  // max length of answer
#define PORT 57120  // the port on which to listen for incoming data

class OscpController {
private:
    WSADATA wsaData;
    SOCKET client_socket;
    sockaddr_in server;
public:

    OscpController(){
        // initialise winsock
        std::cout << "Initialising Winsock...\n";
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cout << "Failed. Error Code: " << WSAGetLastError() << "\n";
            exit(EXIT_FAILURE);
        }
        std::cout << "Initialised.\n";
    }

    ~OscpController() {
        closesocket(client_socket);
        WSACleanup();
    }
    void start();
    void stop();
    void sendStartMessage(void* buffer, int size);
    void sendStopMessage(void* buffer, int size);
    void sendFrequencyChange(float frequency, void* buffer, int size);
    void checkAndSendDimensionalityChange(std::string prevDimens, std::string currDimens, void* buffer, int size);

private:
    void send(const void *buffer, const OSCPP::Client::Packet &packet) const;

    static void convert_dimensions(const char *prevDimens, int *prevDimensions) ;
};


#endif //MANYLANDS_OSCPCONTROLLER_H
