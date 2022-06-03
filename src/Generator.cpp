/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>

#include "Generator.h"
#include "Register.h"

Register Generator::next_register() {
	return Register(register_number++);
}

uint32_t Generator::add_basic_block() {
	basic_blocks.push_back(BasicBlock(num_basic_blocks++));
	return num_basic_blocks - 1;
}

void Generator::dump() {
	for (auto const &bb : basic_blocks)
		std::cout << bb.to_string();
}

void Generator::dump_basic_blocks() {
	for (auto const &bb : basic_blocks)
		bb.dump();
}