// start a telnet connection with a local instance of VICE
#include "stdafx.h"
#include "winsock2.h"
#include <ws2tcpip.h>
#include <inttypes.h>
#include "machine.h"
#include "MainFrm.h"
#include "Step6502.h"
#include "Expressions.h"
#include "Sym.h"

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
	bool viceRunning;
	bool monitorOn;
	bool stopRequest;
};

static ViceConnect sVice;

static const char* sViceRunning = "\n<VICE started>\n";
static const char* sViceStopped = "<VICE stopped>\n";
static const char* sViceConnected = "<VICE connected>\n";
static const char* sViceDisconnected = "<VICE disconnected>\n";
static const char* sViceLost = "<VICE connection lost>\n";
static const int sViceLostLen = (const int)strlen(sViceLost);
static const char* sViceRun = "x\n";
static const char* sViceDel = "del\n";
static char sViceExit[64];
static int sViceExitLen;

void ViceSend(const char *string, int length)
{
	if (sVice.activeConnection) {
		if (length&&(string[0]=='x'||string[0]=='X'||string[0]=='g'||string[0]=='G')) {

			memcpy(sViceExit, string, length);
			sViceExitLen = length;
			sVice.viceRunning = true;
		}
		send(sVice.s, string, length, NULL);
	}
}

bool ViceAction()
{
	if (sVice.activeConnection) {
		if (sVice.monitorOn) {
			ViceSend(sViceRun, (int)strlen(sViceRun));
			return true;
		}
		sVice.close();
		if (CMainFrame *pFrame = theApp.GetMainFrame()) {
			pFrame->VicePrint(sViceDisconnected, (int)strlen(sViceDisconnected));
		}
		return false;
	}
	return sVice.connect();
}

bool ViceRunning()
{
	return sVice.activeConnection && !sVice.monitorOn;
}

void ViceBreak()
{
	sVice.stopRequest = true;
}

void ViceConnectShutdown()
{
	if (sVice.activeConnection) {
		sVice.close();
	}
}

unsigned long WINAPI ViceConnectThread(void* data)
{
	((ViceConnect*)data)->connectionThread();
	return 0;
}

bool ViceConnect::connect()
{
	monitorOn = false;
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
	Vice_Labels,
	Vice_Breakpoints,
	Vice_Registers,
	Vice_Wait,

	Vice_SendBreakpoints,
	Vice_Return,
	Vice_Break,
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

// (C:$e5d4) shl
// $0421 .Label2
// $1234 .Label1
bool ProcessViceLabel(char* line, int len)
{
	if (*line=='$' && line[6]=='.') {
		wchar_t name[256];
		char *end;
		uint16_t addr = (uint16_t)strtol(line+1, &end, 16);
		size_t namelen;
		line[len] = 0;
		mbstowcs_s(&namelen, name, line+6, 256);
		AddSymbol(addr, name, namelen);
	}
	return false;	
}

static uint32_t lastBPID = ~0UL;

bool ProcessBKLine(char* line, int len)
{
	// check "No breakpoints are set"
	if( _strnicmp(line, "No breakpoints", 14)==0 ) { return true; }

	// "BREAK: 3  C:$1300  (Stop on exec)"

	if( _strnicmp(line, "BREAK:", 6)==0) {
		while (len>=4&&*line!='$') { ++line; --len; }
		char *end;
		lastBPID = SetPCBreakpoint((uint16_t)strtol(line+1, &end, 16));
		return false;
	}

	// "Condition: A != $00"
	if( _strnicmp(line, "\tCondition:", 11)==0 && lastBPID != ~0UL ) {
		while( len && ((uint8_t)line[len-1])<=' ') { --len; }
		line[len] = 0;
		wchar_t display[128];
		size_t displen;
		mbstowcs_s(&displen, display, line + 12, 128);
		uint8_t ops[64];
		uint32_t len = BuildExpression(display, ops, sizeof(ops));
		SetBPCondition(lastBPID, ops, len);
		if (CMainFrame *pFrame = theApp.GetMainFrame()) {
			pFrame->BPDisplayExpression( lastBPID, display );
		}
		return false;
	}

	if (CMainFrame *pFrame = theApp.GetMainFrame()) {
		pFrame->VicePrint(line, len);
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
		r.A = (uint8_t)strtol(regs+7, &end, 16); end = erp;
		r.X = (uint8_t)strtol(regs+10, &end, 16); end = erp;
		r.Y = (uint8_t)strtol(regs+13, &end, 16); end = erp;
		r.S = (uint8_t)strtol(regs+16, &end, 16); end = erp;
		r.P = (uint8_t)strtol(regs+25, &end, 2);
		SetRegs(r);
		ResetUndoBuffer();
		return true;
	}
	return false;
}

const char* sMemory = "m $0000 $ffff\n";
const char* sRegisters = "r\n";
const char* sLabels = "shl\n";
const char* sBreakpoints = "bk\n";

// waits for vice break after connection is established
void ViceConnect::connectionThread()
{
	DWORD timeout = 100;// SOCKET_READ_TIMEOUT_SEC*1000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	char recvBuf[RECEIVE_SIZE];
	char line[512];
	int offs = 0;

	sViceExit[0] = 0;

	ViceUpdate state = Vice_None;
	viceRunning = false;

	if (CMainFrame *pFrame = theApp.GetMainFrame()) {
		pFrame->VicePrint(sViceConnected, (int)strlen(sViceConnected));
	}

	uint8_t *RAM = nullptr;

	int currBreak = 0;

	stopRequest = false;

	while(activeConnection) {
		if (closeRequest) {
			threadHandle = INVALID_HANDLE_VALUE;
			close();
			break;
		}

		if (viceRunning) {
			state = Vice_SendBreakpoints;
			currBreak = 0;
			viceRunning = false;
			send(s, sViceDel, 4, NULL);
		}

		int bytesReceived = recv(s, recvBuf, RECEIVE_SIZE,0);
		if (bytesReceived==SOCKET_ERROR) {
			if (WSAGetLastError()==WSAETIMEDOUT) {
				if (state==Vice_None && stopRequest) {
					send(s, "\n", 1, NULL);
					stopRequest = false;
				}
				Sleep(100);
			} else {
				activeConnection = false;
				break;
			}
		} else {
			int read = 0;
			while (read<bytesReceived) {
				char c = line[offs++] = recvBuf[read++];

				// vice prompt = (C:$????)
				bool prompt = offs>=9&&line[8]==')'&&strncmp(line, "(C:$", 4)==0;

				// vice sends lines so process one line at a time
				if (c==0x0a||offs==sizeof(line)||prompt||((state==Vice_Wait) && read==bytesReceived)) {
					if (state==Vice_Return) {
						if (prompt) { state = Vice_None; }
						else if (CMainFrame *pFrame = theApp.GetMainFrame()) {
							pFrame->VicePrint(line, offs);
						}
					} else  if (state==Vice_None) {
						if (prompt) {
							// first connection => start reading memory
							send(s, sMemory, (int)strlen(sMemory), NULL);
							state = Vice_Memory;
							monitorOn = true;
							if (CMainFrame *pFrame = theApp.GetMainFrame()) {
								pFrame->VicePrint(sViceStopped, (int)strlen(sViceStopped));
								pFrame->Invalidate();
							}
						}
					} else if (state==Vice_Memory) {
						if (prompt || ProcessViceLine(line, offs)) {
							while (recv(s, recvBuf, RECEIVE_SIZE, 0)!=SOCKET_ERROR) { Sleep(1); }
							if( RAM ) { free(RAM); }
							RAM = (uint8_t*)malloc(64*1024);
							memcpy(RAM, Get6502Mem(), 64*1024);
							RemoveBreakpointByID(-1);
							lastBPID = ~0UL;
							state = Vice_Labels;
							ShutdownSymbols();
							send(s, sLabels, (int)strlen(sLabels), NULL);
							offs = 0;
							break;
						}
					} else if (state==Vice_Labels) {
						if (prompt || ProcessViceLabel(line, offs)) {
							while (recv(s, recvBuf, RECEIVE_SIZE, 0)!=SOCKET_ERROR) { Sleep(1); }
							RemoveBreakpointByID(-1);
							lastBPID = ~0UL;
							state = Vice_Breakpoints;
							send(s, sBreakpoints, (int)strlen(sBreakpoints), NULL);
							offs = 0;
							break;
						}
					} else if (state==Vice_Breakpoints) {
						if (prompt || ProcessBKLine(line, offs)) {
							if (CMainFrame *pFrame = theApp.GetMainFrame()) { pFrame->BreakpointChanged(); }
							send(s, sRegisters, (int)strlen(sRegisters), NULL);
							state = Vice_Registers;
							offs = 0;
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
					} else if( state == Vice_SendBreakpoints ) {
						if (prompt) {
							offs = 0;
							uint16_t numDis;
							uint16_t *pBP = nullptr;
							uint32_t *pID = nullptr;
							uint16_t num = GetPCBreakpointsID(&pBP, &pID, numDis);
							if ((int)num<=currBreak) {
								while (recv(s, recvBuf, RECEIVE_SIZE, 0)!=SOCKET_ERROR) { Sleep(1); }
								// update registers
								{
									char reg[64];
									const Regs r = GetRegs();
									int l = sprintf_s( reg, sizeof(reg), "r pc=$%04x,a=$%02x,x=$%02x,y=$%02x,sp=$%02x,fl=$%02x\n", 
													   r.PC, r.A, r.X, r.Y, r.S, r.P );
									send(s, reg, l, 0);
								}
								while (recv(s, recvBuf, RECEIVE_SIZE, 0)!=SOCKET_ERROR) { Sleep(1); }
								// update memory if changed
								if (RAM) {
									char memstr[6+4*256+4];
									int sl = (int)sizeof(memstr);
									uint16_t a = 0;
									const uint8_t *m = Get6502Mem();
									do {
										if( RAM[a] != m[a] ) {
											int bytes = 0;
											int o = sprintf_s(memstr, sl, ">$%04x", a);
											uint16_t n = a;
											while( n && (n-a)<256 ) {
												o += sprintf_s(memstr+o, sl-o, " $%02x", m[n]);
												++n;
												if( n && RAM[n]==m[n] ) {
													uint16_t p = n;
													while( p && (p-n)<4 && RAM[p]==m[p] ) { ++p; }
													if( !p || (p-n)==4) { break; }
												}
											}
											a = n;
											o += sprintf_s(memstr+o, sl-o, "\n");
											send(s, memstr, o, 0);
											while (recv(s, recvBuf, RECEIVE_SIZE, 0)!=SOCKET_ERROR) { Sleep(1); }
										} else { ++a; }
									} while( a );
									free(RAM);
									RAM = nullptr;
								}
								// finally send the exit command
								send(s, sViceExit, sViceExitLen, 0);
								sViceExit[0] = 0;
								monitorOn = false;
								if (CMainFrame *pFrame = theApp.GetMainFrame()) {
									pFrame->ViceMonClear();
									pFrame->VicePrint(sViceRunning, (int)strlen(sViceRunning));
								}
								state = Vice_None;
								read = bytesReceived;
							} else {
								char buf[256];
								int l = sprintf_s(buf, sizeof(buf), "bk $%04x", pBP[currBreak]);
								if (CMainFrame *pFrame = theApp.GetMainFrame()) {
									if (const wchar_t *pExpr = pFrame->GetBPExpression(pID[currBreak])) {
										char expr[128];
										size_t lex;
										wcstombs_s(&lex, expr, pExpr, sizeof(expr));
										l += sprintf_s(buf+l, sizeof(buf)-l, " if %s", expr);
									}
								}
								l += sprintf_s(buf+l, sizeof(buf)-l, "\n");
								send(sVice.s, buf, l, NULL);
								if (CMainFrame *pFrame = theApp.GetMainFrame()) {
									pFrame->VicePrint(buf, l);
								}
								++currBreak;
							}
						}
					} else {
						if (CMainFrame *pFrame = theApp.GetMainFrame()) {
							pFrame->VicePrint(line, offs);
							pFrame->ViceInput(true);
						}
					}
					offs = 0;
				}
			}
		}
	}
	if( RAM ) { free(RAM); }
	if (CMainFrame *pFrame = theApp.GetMainFrame()) {
		pFrame->VicePrint(sViceLost, sViceLostLen);
	}
}
