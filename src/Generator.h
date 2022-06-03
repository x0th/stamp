/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <vector>

#include "Instruction.h"
#include "BasicBlock.h"

class Generator {
public:
	Generator() {}
	~Generator() = default;

	Register next_register();

	uint32_t add_basic_block();

	void dump();
	void dump_basic_blocks();

	template<class T, typename... Args>
	void append(Args&&... args) {
		basic_blocks[basic_blocks.size() - 1].add_instruction(static_cast<Instruction*>(new T(std::forward<Args>(args)...)));
	}
private:
	uint32_t register_number = { 0 };
	uint32_t num_basic_blocks = { 0 };

	std::vector<BasicBlock> basic_blocks;
};