#pragma once
#ifndef frameH
#define frameH

#include <string>
#include <string.h>

using std::string;

struct Frame {
	int id;
	string startAddress; //hex representation. E.x. if 0x00 and end is 0x500, Then integer representation is 0 and 250
	string endAddress;   //hex representation
	int startAddress_i;  //decimal representation
	int endAddress_i;  //decimal representation
	string pid = "";

	bool operator==(const struct Frame& f) const{
		return f.id == this->id;
	}
};

#endif