#ifndef CLASS_PACKET
#define CLASS_PACKET

#include "SIF.h"
#include <BromScript.h>
#include "EzSock.h"

class Packet{
public:
	bool Valid;
	EzSock* Sock;
	unsigned int OutPos;
	unsigned int OutSize;
	unsigned int InPos;
	unsigned int InSize;
	byte* InBuffer;
	byte* OutBuffer;
	
	Packet();
	Packet(EzSock*);
	~Packet();
	
	void Clear();
	
	void WriteByte(byte);
	void WriteBytes(byte*, int, bool);
	void WriteShort(short);
	void WriteInt(int);
	void WriteLong(long);
	void WriteFloat(float);
	void WriteDouble(double);
	void WriteString(const char*);
	void WriteString(const char*, bool);
	
	byte ReadByte();
	byte* ReadBytes(int len);
	short ReadShort();
	int ReadInt();
	long ReadLong();
	float ReadFloat();
	double ReadDouble();
	Scratch::CString ReadString();
	
	bool CanRead(int);
	bool HasDataLeft();
	int DataLeft();
	
	void Send();

	void CheckSpaceOut(int);
	void AllocateMoreSpaceOut(int);
	void CheckSpaceIn(int);
	void AllocateMoreSpaceIn(int);
};

#endif