#include <iostream>
#include "packet.h"
#include "EzSock.h"

#pragma comment(lib,"wsock32.lib")
typedef int socklen_t;

#if !defined(SOCKET_ERROR)
	#define SOCKET_ERROR -1
#endif

#if !defined(SOCKET_NONE)
	#define SOCKET_NONE 0
#endif

#if !defined(INVALID_SOCKET)
	#define INVALID_SOCKET -1
#endif

EzSock::EzSock(){
	MAXCON = 5;
	memset (&addr,0,sizeof(addr));
	
	WSAStartup( MAKEWORD(1,1), &wsda );

	this->sock = INVALID_SOCKET;
	this->blocking = false;
	this->Valid = false;
	this->scks = new fd_set;
	this->times = new timeval;
	this->times->tv_sec = 0;
	this->times->tv_usec = 0;
	this->state = skDISCONNECTED;
	this->totaldata = 0;
}

EzSock::~EzSock(){
	close();
	delete scks;
	delete times;
}

bool EzSock::check(){
	return sock > SOCKET_NONE;
}

bool EzSock::create(){
	return create(IPPROTO_TCP, SOCK_STREAM);
}

bool EzSock::create(int Protocol){
	switch(Protocol){
		case IPPROTO_TCP: return create(IPPROTO_TCP, SOCK_STREAM);
		case IPPROTO_UDP: return create(IPPROTO_UDP, SOCK_DGRAM);
		default:          return create(Protocol, SOCK_RAW);
	}
}

bool EzSock::create(int Protocol, int Type){
	state = skDISCONNECTED;
	sock = socket(AF_INET, Type, Protocol);
	lastCode = sock;

	return sock > SOCKET_NONE;
}

bool EzSock::bind(unsigned short port){
	if(!check()) return false;

	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port        = htons(port);
	lastCode = ::bind(sock,(struct sockaddr*)&addr, sizeof(addr));

	return !lastCode;
}

bool EzSock::listen(){
	lastCode = ::listen(sock, MAXCON);
	if (lastCode == SOCKET_ERROR) return false;

	state = skLISTENING;
	this->Valid = true;
	return true;
}

bool EzSock::accept(EzSock* socket){
	if (!blocking && !CanRead()) return false;

	int length = sizeof(socket->addr);
	socket->sock = ::accept(sock,(struct sockaddr*) &socket->addr, (socklen_t*) &length);

	lastCode = socket->sock;
	if ( socket->sock == SOCKET_ERROR )
		return false;

	socket->state = skCONNECTED;
	return true;
}

void EzSock::close(){
	state = skDISCONNECTED;

	::closesocket(sock);
}

long EzSock::uAddr(){
	return addr.sin_addr.s_addr;
}

int EzSock::connect(const char* host, unsigned short port){
	if(!check())
		return 1;

	struct hostent* phe;
	phe = gethostbyname(host);
	if (phe == NULL)
		return 2;

	memcpy(&addr.sin_addr, phe->h_addr, sizeof(struct in_addr));

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);

	if(::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		return 3;

	state = skCONNECTED;
	this->Valid = true;
	return 0;
}

bool EzSock::CanRead(){
	FD_ZERO(scks);
	FD_SET((unsigned)sock, scks);

	return select(sock+1,scks,NULL,NULL,times) > 0;
}

bool EzSock::IsError(){
	if (state == skERROR)
		return true;

	FD_ZERO(scks);
	FD_SET((unsigned)sock, scks);

	if (select(sock+1, NULL, NULL, scks, times) >=0 )
		return false;

	state = skERROR;
	return true;
}

int EzSock::SendRaw(const char* data, int dataSize){
	return send(this->sock, data, dataSize, 0);
}
