#ifndef EzSock_H
#define EzSock_H

#include <sstream>
#include <vector>
#include <fcntl.h>
#include <ctype.h>
#include <winsock2.h>

class Packet;

using namespace std;

class EzSock {
public:
	enum SockState{
		skDISCONNECTED = 0, 
		skUNDEF1, //Not implemented
		skLISTENING, 
		skUNDEF3, //Not implemented
		skUNDEF4, //Not implemented
		skUNDEF5, //Not implemented
		skUNDEF6, //Not implemented
		skCONNECTED, 
		skERROR 
	};

	bool blocking;
	bool Valid;
	
	struct sockaddr_in addr;
    struct sockaddr_in fromAddr;
	unsigned long fromAddr_len;

	SockState state;

	int lastCode;

	EzSock();
	~EzSock();

	bool create();
	bool create(int Protocol);
	bool create(int Protocol, int Type);
	bool bind(unsigned short port);
	bool listen();
	bool accept(EzSock* socket);
	int connect(const char* host, unsigned short port);
	void close();

	long uAddr();
	bool IsError();
	
	bool CanRead();
	
	int sock;
	int SendRaw(const char * data, int dataSize);

private:
	WSADATA wsda;

	int MAXCON;

	fd_set  *scks;
	timeval *times;
	
	unsigned int totaldata;
	bool check();
};

#endif
