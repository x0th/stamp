/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */


#pragma once

#include <string>
#include <vector>

#include "Instruction.h"

class Instruction;

class BasicBlock {
public:
	BasicBlock(uint32_t index) : index(index) {}
	~BasicBlock();

	std::string to_string() const;
	void dump() const;

	uint32_t get_index() const { return index; }
	std::vector<Instruction*> &get_instructions() { return instructions; }

	void add_instruction(Instruction *instruction) { instructions.push_back(instruction); }
private:
	uint32_t index;

	std::vector<Instruction*> instructions;
};