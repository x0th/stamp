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

BasicBlock &Generator::add_basic_block() {
	basic_blocks.push_back(BasicBlock(num_basic_blocks));
	return basic_blocks[num_basic_blocks++];
}

uint32_t Generator::add_scope_beginning(uint32_t flags) {
	scopes.push_back(LexicalScope(add_basic_block().get_index(), flags));
	return num_scopes++;
}

uint32_t Generator::add_scope_beginning_current_bb(uint32_t flags) {
	scopes.push_back(LexicalScope(basic_blocks[num_basic_blocks - 1].get_index(), flags));
	return num_scopes++;
}

void Generator::end_scope(uint32_t scope_id) {
	scopes[scope_id].end_scope(basic_blocks[num_basic_blocks - 1].get_index());
}

void Generator::dump() {
	for (auto const &bb : basic_blocks)
		std::cout << bb.to_string();
}

void Generator::dump_basic_blocks() {
	for (auto const &bb : basic_blocks)
		bb.dump();
}

void Generator::dump_scopes() {
	std::cout << "Lexical scopes:\n";
	for (auto const &scope : scopes)
		std::cout << scope.to_string() << "\n";
}