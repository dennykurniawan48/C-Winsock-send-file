#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

int main() {
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	
	if (WSAStartup(ver, &wsData) != 0) {
		std::cerr << "Error starting winsock!" << std::endl;
		return -1;
	}

	SOCKET clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (clientSock == INVALID_SOCKET) {
		std::cerr << "Error creating socket!, " << WSAGetLastError() << std::endl;
		return -1;
	}

	char serverAddress[NI_MAXHOST];
	memset(serverAddress, 0, NI_MAXHOST);
	
	std::cout << "Enter server address: ";
	std::cin.getline(serverAddress, NI_MAXHOST);

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(55000);
	inet_pton(AF_INET, serverAddress, &hint.sin_addr);

	char welcomeMsg[255];
	const int BUFFER_SIZE = 1024;
	char bufferFile[BUFFER_SIZE];
	char fileRequested[FILENAME_MAX];
	std::ofstream file;


	if (connect(clientSock, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
		std::cerr << "Error connect to server!, " << WSAGetLastError() << std::endl;
		closesocket(clientSock);
		WSACleanup();
		return -1;
	}

	int byRecv = recv(clientSock, welcomeMsg, 255, 0);
	if (byRecv == 0 || byRecv == -1) {
		closesocket(clientSock);
		WSACleanup();
		return -1;
	}

	bool clientClose = false;
	int codeAvailable = 404;
	const int fileAvailable = 200;
	const int fileNotfound = 404;
	long fileRequestedsize = 0;
	do {
		int fileDownloaded = 0;
		memset(fileRequested, 0, FILENAME_MAX);
		std::cout << "Enter file name: " << std::endl;
		std::cin.getline(fileRequested, FILENAME_MAX);
		
		byRecv = send(clientSock, fileRequested, FILENAME_MAX, 0);
		if (byRecv == 0 || byRecv == -1) {
			clientClose = true;
			break;
		}

		byRecv = recv(clientSock, (char*)&codeAvailable, sizeof(int), 0);
		if (byRecv == 0 || byRecv == -1) {
			clientClose = true;
			break;
		}
		if (codeAvailable == 200) {
			byRecv = recv(clientSock, (char*)&fileRequestedsize, sizeof(long), 0);
			if (byRecv == 0 || byRecv == -1) {
				clientClose = true;
				break;
			}

			file.open(fileRequested, std::ios::binary | std::ios::trunc);

			do {
				memset(bufferFile, 0, BUFFER_SIZE);
				byRecv = recv(clientSock, bufferFile, BUFFER_SIZE, 0);

				if (byRecv == 0 || byRecv == -1) {
					clientClose = true;
					break;
				}

				file.write(bufferFile, byRecv);
				fileDownloaded += byRecv;
			} while (fileDownloaded < fileRequestedsize);
			file.close();
		}
		else if (codeAvailable == 404) {
			std::cout << "Can't open file or file not found!" << std::endl;
		}
	} while (!clientClose);
	closesocket(clientSock);
	WSACleanup();
	return 0;
}
