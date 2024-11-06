#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <cstring>

#define FILE_TRANSFER_FLAG 0x01//FILE_TRANSFER_FLAG is 1 (in decimal).
#define FILE_ACK_FLAG 0x11//FILE_ACK_FLAG is 17 (in decimal).
#define DEFAULT_BUFLEN 10'000'000
#define DEFAULT_PORT "27015"

int __cdecl main()
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    char* sendbuf = new char[DEFAULT_BUFLEN];
    char recvbuf[3];
    int iResult;
    int recvbuflen = 3;
    std::fstream fp;

    // Input server address, file name, and flag manually
    std::string serverAddress;
    std::string fileName;
    std::string flag;

    std::cout << "Enter the server address: ";
    std::cin >> serverAddress;

    std::cout << "Enter the file name to send: ";
    std::cin >> fileName;

    std::cout << "Enter the flag (-f for file transfer): ";
    std::cin >> flag;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; //AF_INET
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(serverAddress.c_str(), DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        // Connect to server
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!: %d\n" ,WSAGetLastError());
        WSACleanup();
        return 1;
    }

    bool doTransfer = false;

    if (flag == "-f") {
        // Telling the server I want to send a file
        sendbuf[0] = FILE_TRANSFER_FLAG;
        sendbuf[1] = 0;
        int length = fileName.length();
        for (int i = 0; i < length + 1; i++) {  //length + 1 is so that the string is NULL-ed
            sendbuf[i + 1] = fileName[i];
        }
        sendbuf[length + 1] = 0;

        iResult = send(ConnectSocket, sendbuf, length + 2, 0);
        if (iResult == SOCKET_ERROR) {
            std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        // Zeroing buffer for safety
        recvbuf[0] = 0;
        recvbuf[1] = 0;
        recvbuf[2] = 0;
        while (1) {
            iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                if ((int)recvbuf[0] == FILE_ACK_FLAG) {
                    doTransfer = true;
                } else {
                    std::cout << "Error - wrong answer flag" << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
                break;
            }
            else if (iResult == 0) {
                std::cout << "No bytes received, continuing" << std::endl;
            }
            else {
                std::cout << "recv failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
        }

        // Sending the file
        if (doTransfer) {
            fp.open(fileName, std::ios::in | std::ios::binary);
            if (!fp) {
                std::cout << "File doesn't exist: " << fileName << std::endl;
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            while (!fp.eof()) {
                fp.read(sendbuf, DEFAULT_BUFLEN);
                iResult = send(ConnectSocket, sendbuf, fp.gcount(), 0);
                if (iResult == SOCKET_ERROR) {
                    std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
            }
            fp.close();
        }
    } else if (flag == "-g") {
        std::cout << "Flag -g not implemented in this version." << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    } else {
        std::cout << "Invalid argument for flag" << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            std::cout << "Bytes received: " << iResult << std::endl;
        else if (iResult == 0)
            std::cout << "Connection closed" << std::endl;
        else
            std::cout << "recv failed with error: " << WSAGetLastError() << std::endl;

    } while (iResult > 0);

    // Cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
