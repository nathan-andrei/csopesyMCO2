#pragma once
#ifndef frameH
#define frameH

#include <string>
#include <string.h>

struct Frame {
	int id;
	int startAddress;
	int endAddress;
	std::string pid = "";

	bool operator==(const struct Frame& f) const{
		return f.id == this->id;
	}
};

#endif