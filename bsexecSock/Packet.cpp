#include "Packet.h"
#include <stdlib.h>
#include <stdio.h>

Packet::Packet(){
	this->Valid = false;

	this->Sock = null;
	this->InPos = 0;
	this->InSize = 0;
	
	this->OutPos = 0;
	this->OutSize = 0;
	this->OutBuffer = null;
	this->InBuffer = null;
}

Packet::Packet(EzSock* sock){
	this->Valid = true;

	this->Sock = sock;
	this->InPos = 0;
	this->InSize = 0;
	
	this->OutPos = 0;
	this->OutSize = 0;
	this->OutBuffer = null;
	this->InBuffer = null;
}

Packet::~Packet(){
	this->Clear();
}

void Packet::Clear(){
	this->InPos = 0;
	this->InSize = 0;
	this->InBuffer = null;

	this->OutPos = 0;
	this->OutSize = 0;
	this->OutBuffer = null;
}

void Packet::CheckSpaceOut(int needed){
	if (this->OutPos + needed >= this->OutSize){
		this->AllocateMoreSpaceOut(needed < 128 ? 128 : needed);
	}
}

void Packet::AllocateMoreSpaceOut(int addsize){
	this->OutSize += addsize;
	byte* newbuff = (byte*)malloc(this->OutSize);

	if(this->OutBuffer != null) {
		memcpy(newbuff, this->OutBuffer, this->OutSize - addsize);
		free(this->OutBuffer);
	}

	this->OutBuffer = newbuff;
}

void Packet::CheckSpaceIn(int needed){
	if (this->InPos + needed >= this->InSize){
		this->AllocateMoreSpaceIn(needed);
	}
}

void Packet::AllocateMoreSpaceIn(int addsize){
	this->InSize += addsize;
	byte* newbuff = (byte*)malloc(this->InSize);

	if(this->InBuffer != null) {
		memcpy(newbuff, this->InBuffer, this->InSize - addsize);
		free(this->InBuffer);
	}

	this->InBuffer = newbuff;
}

byte Packet::ReadByte(){
	if (!this->CanRead(1)) return 0;

	return this->InBuffer[this->InPos++];
}

byte* Packet::ReadBytes(int len){
	if (!this->CanRead(len)) return null;

	this->InPos += len;
	return this->InBuffer + (this->InPos - len);
}

short Packet::ReadShort(){
	if (!this->CanRead(2)) return 0;
	
	byte a = this->InBuffer[this->InPos++];
	byte b = this->InBuffer[this->InPos++];

	return a << 8 | b;
}

float Packet::ReadFloat(){
	float num;
	byte b[] = {
		this->InBuffer[this->InPos + 0],
		this->InBuffer[this->InPos + 1],
		this->InBuffer[this->InPos + 2],
		this->InBuffer[this->InPos + 3]
	};

	memcpy(&num, &b, 4);
	this->InPos += 4;
	return num;
}

double Packet::ReadDouble(){
	double num;
	byte b[] = {
		this->InBuffer[this->InPos + 0],
		this->InBuffer[this->InPos + 1],
		this->InBuffer[this->InPos + 2],
		this->InBuffer[this->InPos + 3],
		this->InBuffer[this->InPos + 4],
		this->InBuffer[this->InPos + 5],
		this->InBuffer[this->InPos + 6],
		this->InBuffer[this->InPos + 7]
	};

	memcpy(&num, &b, 8);
	this->InPos += 8;
	return num;
}

int Packet::ReadInt(){
	if (!this->CanRead(4)) return 0;

	int num = 0;
	num = (num << 8) + this->InBuffer[this->InPos + 3];
	num = (num << 8) + this->InBuffer[this->InPos + 2];
	num = (num << 8) + this->InBuffer[this->InPos + 1];
	num = (num << 8) + this->InBuffer[this->InPos + 0];

	this->InPos += 4;
	return num;
}

long Packet::ReadLong(){
	if (!this->CanRead(8)) return 0;

	long num = 0;
	num = (num << 8) + this->InBuffer[this->InPos + 7];
	num = (num << 8) + this->InBuffer[this->InPos + 6];
	num = (num << 8) + this->InBuffer[this->InPos + 5];
	num = (num << 8) + this->InBuffer[this->InPos + 4];
	num = (num << 8) + this->InBuffer[this->InPos + 3];
	num = (num << 8) + this->InBuffer[this->InPos + 2];
	num = (num << 8) + this->InBuffer[this->InPos + 1];
	num = (num << 8) + this->InBuffer[this->InPos + 0];

	this->InPos += 8;
	return num;
}

Scratch::CString Packet::ReadString() {
	int len = this->ReadInt();
	if (!this->CanRead(len)) return "";

	Scratch::CString str;
	for (int i = 0; i < len; i++)
		str += this->InBuffer[this->InPos + i];

	this->InPos += len;
	return str;
}

int Packet::DataLeft(){
	return this->InSize - this->InPos;
}

bool Packet::HasDataLeft(){
	return this->InSize - this->InPos > 0;
}

bool Packet::CanRead(int numofbytes){
	bool res = this->InSize - this->InPos >= (unsigned int)numofbytes;
	if (res == false && this->Sock != null){
		byte* tmp = new byte[numofbytes];
		int recamount = recv(this->Sock->sock, (char*)tmp, numofbytes, 0);

		if (recamount == -1){
			this->Sock->state = EzSock::skERROR;
			this->Valid = false;
			return false;
		}	

		if (recamount != numofbytes){
			this->Valid = false;
			return false;
		}

		this->CheckSpaceIn(numofbytes);
		memcpy(this->InBuffer + this->InPos, tmp, numofbytes);
		delete tmp;

		return true;
	}

	return res;
}

void Packet::WriteByte(byte num){
	this->CheckSpaceOut(1);
	this->OutBuffer[this->OutPos++] = num;
}

void Packet::WriteShort(short num){
	byte tmp[2];
	memcpy(&tmp, &num, 2);

	this->CheckSpaceOut(2);
	this->OutBuffer[this->OutPos++] = tmp[1];
	this->OutBuffer[this->OutPos++] = tmp[0];
}

void Packet::WriteInt(int num){
	this->CheckSpaceOut(4);
	memcpy(this->OutBuffer + this->OutPos, &num, 4);
	this->OutPos += 4;
}

void Packet::WriteLong(long num){
	this->CheckSpaceOut(8);
	memcpy(this->OutBuffer + this->OutPos, &num, 8);
	this->OutPos += 8;
}

void Packet::WriteFloat(float num){
	this->CheckSpaceOut(4);
	memcpy(this->OutBuffer + this->OutPos, &num, 4);
	this->OutPos += 4;
}

void Packet::WriteDouble(double num){
	this->CheckSpaceOut(8);
	memcpy(this->OutBuffer + this->OutPos, &num, 8);
	this->OutPos += 8;
}

void Packet::WriteString(const char* str){
	int size = strlen(str);
	this->CheckSpaceOut(size * 2 + 2);

	this->WriteShort(size);

	for (int i = 0; i < size; i++){
		this->OutBuffer[this->OutPos++] = 0;
		this->OutBuffer[this->OutPos++] = str[i];
	}
	
	this->OutPos += size * 2;
}

void Packet::WriteString(const char* str, bool sendsize){
	int size = strlen(str);
	this->CheckSpaceOut(size * 2);

	if (sendsize)
		this->WriteShort(size);

	for (int i = 0; i < size; i++){
		this->OutBuffer[this->OutPos++] = 0;
		this->OutBuffer[this->OutPos++] = str[i];
	}
	
	this->OutPos += size * 2;
}

void Packet::Send(){
	this->Sock->SendRaw((char*)this->OutBuffer, this->OutSize);

	delete this->OutBuffer;
	this->OutPos = 0;
	this->OutSize = 0;
}