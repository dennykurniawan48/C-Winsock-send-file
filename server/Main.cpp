#include <iostream>
#include <fstream>
#include <ws2tcpip.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	if (WSAStartup(ver, &wsData) != 0) {
		std::cerr << "Error starting winsock!" << std::endl;
		return -1;
	}

	SOCKET listenerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenerSock == INVALID_SOCKET) {
		std::cerr << "Error creating listener socket! " << WSAGetLastError() << std::endl;
		WSACleanup();
		return -1;
	}

	sockaddr_in listenerHint;
	listenerHint.sin_family = AF_INET;
	listenerHint.sin_port = htons(55000);
	listenerHint.sin_addr.S_un.S_addr = INADDR_ANY;
	
	bind(listenerSock, (sockaddr*)&listenerHint, sizeof(listenerHint));
	listen(listenerSock, SOMAXCONN);

	sockaddr_in clientHint;
	int clientSize = sizeof(clientHint);

	SOCKET clientSock = accept(listenerSock, (sockaddr*)&clientHint, &clientSize);

	if (clientSock == SOCKET_ERROR) {
		std::cerr << "Error accept socket! " << WSAGetLastError() << std::endl;
		closesocket(listenerSock);
		WSACleanup();
		return -1;
	}

	char host[NI_MAXHOST];
	char serv[NI_MAXSERV];

	if (getnameinfo((sockaddr*)&clientHint, sizeof(clientHint), host, NI_MAXHOST, serv, NI_MAXSERV, 0) == 0) {
		std::cout << "Host: " << host << " connected on port: " << serv << std::endl;
	}
	else {
		inet_ntop(AF_INET, &clientHint.sin_addr, host, NI_MAXHOST);
		std::cout << "Host: " << host << " connected on port: " << ntohs(clientHint.sin_port) << std::endl;
	}

	closesocket(listenerSock);

	const char* welcomeMsg = "Welcome to file server.";
	bool clientClose = false;
	char fileRequested[FILENAME_MAX];
	const int fileAvailable = 200;
	const int fileNotfound = 404;
	const int BUFFER_SIZE = 1024;
	char bufferFile[BUFFER_SIZE];
	std::ifstream file;

	// sending welcome message
	int bysendMsg = send(clientSock, welcomeMsg, strlen(welcomeMsg), 0);
	
	if (bysendMsg == 0) {
		// error sending data - break loop
		closesocket(clientSock);
		WSACleanup();
		return -1;
	}

	do {
		memset(fileRequested, 0, FILENAME_MAX);
		int byRecv = recv(clientSock, fileRequested, FILENAME_MAX, 0);
		
		if (byRecv == 0 || byRecv == -1) {
			// error receive data - break loop
			clientClose = true;
		}
		
		// open file
		file.open(fileRequested, std::ios::binary);

		if (file.is_open()) {
			// file is available
			int bySendinfo = send(clientSock, (char*)&fileAvailable, sizeof(int), 0);
			if (bySendinfo == 0 || bySendinfo == -1) {
				// error sending data - break loop
				clientClose = true;
			}

			// get file size
			file.seekg(0, std::ios::end);
			long fileSize = file.tellg();

			// send filesize to client
			bySendinfo = send(clientSock, (char*)&fileSize, sizeof(long), 0);
			if (bySendinfo == 0 || bySendinfo == -1) {
				// error sending data - break loop
				clientClose = true;
			}
			file.seekg(0, std::ios::beg);
			// read file with do-while loop
			do {
				// read and send part file to client
				file.read(bufferFile, BUFFER_SIZE);
				if(file.gcount() > 0)
					bySendinfo = send(clientSock, bufferFile, file.gcount(), 0);

				if (bySendinfo == 0 || bySendinfo == -1) {
					// error sending data - break loop
					clientClose = true;
					break;
				}
			} while (file.gcount() > 0);
			file.close();
		}
		else {
			// Can't open file or file not found
			int bySendCode = send(clientSock, (char*)&fileNotfound, sizeof(int), 0);
			if (bySendCode == 0 || bySendCode == -1) {
				// error sending data - break loop
				clientClose = true;
			}
		}
	} while (!clientClose);

	return 0;
}
