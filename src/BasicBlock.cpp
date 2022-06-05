#include <iostream>

#include "BasicBlock.h"

BasicBlock::~BasicBlock() {
	for (auto i : instructions) {
		i->dealloc();
	}
}

std::string BasicBlock::to_string() const {
	std::string out;
	for (auto i : instructions)
		out += i->to_string() + "\n";
	return out;
}

void BasicBlock::dump() const {
	std::cout << "BB" << std::to_string(index) << ":\n" << to_string() << "\n";
}