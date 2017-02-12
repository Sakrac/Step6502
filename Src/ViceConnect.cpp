// start a telnet connection with a local instance of VICE
#include "stdafx.h"
#include "winsock2.h"
#include <ws2tcpip.h>
#include <inttypes.h>
#include "machine.h"
#include "MainFrm.h"
#include "Step6502.h"

class ViceConnect
{
public:
	enum { RECEIVE_SIZE = 4096 };

	ViceConnect() : activeConnection(false), threadHandle(INVALID_HANDLE_VALUE) {}

	bool openConnection(char* address, int port);
	void connectionThread();

	bool connect();

	void close();

	bool open(char* address, int port);

	struct sockaddr_in addr;
	SOCKET s;
	HANDLE threadHandle;

	bool activeConnection;
	bool closeRequest;
};

static ViceConnect sVice;

bool ViceAction()
{
	if (sVice.activeConnection) {
		sVice.close();
		return false;
	}
	return sVice.connect();
}

void ViceConnectShutdown()
{
	if (sVice.activeConnection) {
		sVice.close();
	}
}

void ViceSend(const char *string, int length)
{
	if (sVice.activeConnection) {
		send(sVice.s, string, length, NULL);
	}
}

unsigned long WINAPI ViceConnectThread(void* data)
{
	((ViceConnect*)data)->connectionThread();
	return 0;
}

bool ViceConnect::connect()
{
	closeRequest = false;
	activeConnection = false;
	if (openConnection("127.0.0.1", 6510)) {
		unsigned long threadId;
		threadHandle = CreateThread(NULL, 16384, ViceConnectThread, this, NULL, &threadId);
	}
	return false;
}


void ViceConnect::close()
{
	if (threadHandle!=INVALID_HANDLE_VALUE) {
		CloseHandle(threadHandle);
		threadHandle = INVALID_HANDLE_VALUE;
	}

	closesocket(s);
	WSACleanup();
	activeConnection = false;
	closeRequest = false;

}

// Open a connection to a remote host
bool ViceConnect::open(char* address, int port)
{
	// Make sure the user has specified a port
	if (port<0||port > 65535) { return false; }

	WSADATA wsaData = {0};
	int iResult = 0;

	DWORD dwRetval;

	struct sockaddr_in saGNI;
	char hostname[NI_MAXHOST];
	char servInfo[NI_MAXSERV];

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return false;
	}
	//-----------------------------------------
	// Set up sockaddr_in structure which is passed
	// to the getnameinfo function
	saGNI.sin_family = AF_INET;

	inet_pton(AF_INET, address, &(saGNI.sin_addr.s_addr));

	//	saGNI.sin_addr.s_addr =
	//	InetPton(AF_INET, strIP, &ipv4addr)
	//	inet_addr(address);
	saGNI.sin_port = htons(port);

	//-----------------------------------------
	// Call getnameinfo
	dwRetval = getnameinfo((struct sockaddr *) &saGNI,
						   sizeof (struct sockaddr),
						   hostname,
						   NI_MAXHOST, servInfo, NI_MAXSERV, NI_NUMERICSERV);

	if (dwRetval != 0) {
		return false;
	}

	iResult = ::connect(s, (struct sockaddr *)&saGNI, sizeof(saGNI));
	return iResult==0;
}

bool ViceConnect::openConnection(char* address, int port)
{
	WSADATA ws;

	// Load the WinSock dll
	long status = WSAStartup(0x0101,&ws);
	if (status!=0) { return false; }

	memset(&addr, 0, sizeof(addr));
	s = socket(AF_INET, SOCK_STREAM, 0);

	// Open the connection
	if (!open(address, port)) { return false; }

	activeConnection = true;
	return true;
}

enum ViceUpdate
{
	Vice_None,
	Vice_Start,
	Vice_Memory,
	Vice_Registers,
	Vice_Wait
};


bool ProcessViceLine(const char* line, int len)
{
	while (len) {
		if (len>3 && line[0]=='>' && line[1]=='C' && line[2]==':') {
			char* end;
			line += 3;
			len -= 3;
			uint32_t addr = strtol(line, &end, 16);
			len += (int)(end-line);
			line = end;
			while (len>=3 && !(line[0]==' ' && line[1]==' ' && line[2]==' ') &&line[0]!=0x0a) {
				while (len && *line==' ') { ++line; --len; }
				uint32_t byte = strtol(line, &end, 16);
				len += (int)(end-line);
				line = end;
				Set6502Byte(uint16_t(addr++), uint8_t(byte));
				if (addr==0x10000) return true;
			}
			break;
		} else {
			++line;
			--len;
		}
	}
	return false;
}

bool ProcessViceRegisters(char* regs, int left)
{
	//   ADDR A  X  Y  SP 00 01 NV-BDIZC LIN CYC  STOPWATCH
	// .;e5d1 00 00 0a f3 2f 37 00100010 000 000    9533160
	if (regs[0] == '.' && left > 32) {
		Regs r;
		char* end = regs + left, *erp = end;
		r.PC = (uint16_t)strtol(regs+2, &end, 16); end = erp;
		r.A = (uint8_t)strtol(regs+6, &end, 16); end = erp;
		r.X = (uint8_t)strtol(regs+9, &end, 16); end = erp;
		r.Y = (uint8_t)strtol(regs+12, &end, 16); end = erp;
		r.S = (uint8_t)strtol(regs+14, &end, 16); end = erp;
		r.P = (uint8_t)strtol(regs+23, &end, 2);
		SetRegs(r);
		ResetUndoBuffer();
		return true;
	}
	return false;
}

const char* sMemory = "m $0000 $ffff\n";
const char* sRegisters = "r\n";

// waits for vice break after connection is established
void ViceConnect::connectionThread()
{
	DWORD timeout = 100;// SOCKET_READ_TIMEOUT_SEC*1000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	char recvBuf[RECEIVE_SIZE];
	char line[512];
	int offs = 0;

	ViceUpdate state = Vice_None;

	while(activeConnection) {
		if (closeRequest) {
			threadHandle = INVALID_HANDLE_VALUE;
			close();
			break;
		}

		int bytesReceived = recv(s, recvBuf, RECEIVE_SIZE,0);
		if (bytesReceived==SOCKET_ERROR) {
			if (WSAGetLastError()==WSAETIMEDOUT) {
				Sleep(100);
			} else {
				activeConnection = false;
				break;
			}
		}
		else if (state==Vice_None) {
			// first connection => start reading memory
			send(s, sMemory, (int)strlen(sMemory), NULL);
			state = Vice_Memory;
		} else {
			int read = 0;
			while (read<bytesReceived) {
				char c = line[offs++] = recvBuf[read++];
				// vice sends lines so process one line at a time
				if (c==0x0a||offs==sizeof(line)||(state==Vice_Wait && read==bytesReceived)) {
					if (state==Vice_Memory) {
						if (ProcessViceLine(line, offs)) {
							while (recv(s, recvBuf, RECEIVE_SIZE, 0)!=SOCKET_ERROR) { Sleep(1); }
							send(s, sRegisters, (int)strlen(sRegisters), NULL);
							state = Vice_Registers;
							break;
						}
					} else if (state==Vice_Registers) {
						if (ProcessViceRegisters(line, offs)) {
							if (CMainFrame *pFrame = theApp.GetMainFrame()) {
								pFrame->FocusPC();
								pFrame->MachineUpdated();
								pFrame->ViceInput(true);
							}
							state = Vice_Wait;
						}
					} else {
						if (CMainFrame *pFrame = theApp.GetMainFrame()) {
							pFrame->VicePrint(recvBuf, bytesReceived);
							pFrame->ViceInput(true);
						}
					}
					offs = 0;
				}
			}
		}
	}
}
