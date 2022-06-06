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

#define SCOPE_CAN_CONTINUE  0b1
#define SCOPE_CAN_BREAK     0b10
#define SCOPE_CAN_RETURN    0b100

class Instruction;
class BasicBlock;

class LexicalScope {
public:
	LexicalScope(int32_t scope_beginning, int32_t flags) : scope_beginning(scope_beginning), scope_end(-1) {
		can_continue = flags & 0b1;
		can_break = (flags & 0b10) >> 1;
		can_return = (flags & 0b100) >> 2;
	}
	~LexicalScope() = default;

	std::string to_string() const {
		auto out = "[" + std::to_string(scope_beginning) + ":" + std::to_string(scope_end) + "]";
		if (can_continue)
			out += " (can continue)";
		if (can_break)
			out += " (can break)";
		if (can_return)
			out += " (can return)";
		return out;
	}

	void end_scope(uint32_t end) { scope_end = end; }

	bool starts_at(uint32_t index) { return scope_beginning == (int32_t)index; }
	bool ends_at(uint32_t index) { return scope_end == (int32_t)index; }
private:
	int32_t scope_beginning, scope_end;
	bool can_continue = { false };
	bool can_break = { false };
	bool can_return = { false };
};

class Generator {
public:
	Generator() {}
	~Generator() = default;

	Register next_register();

	BasicBlock *add_basic_block();

	uint32_t add_scope_beginning(uint32_t flags);
	uint32_t add_scope_beginning_current_bb(uint32_t flags);
	uint32_t get_num_bbs() const { return num_basic_blocks; }
	std::vector<BasicBlock*> &get_bbs() { return basic_blocks; }
	void end_scope(uint32_t scope_id);

	void dump();
	void dump_basic_blocks();
	void dump_scopes();

	LexicalScope *get_scope(uint32_t index) {
		return &scopes[index];
	}

	template<class T, typename... Args>
	T *append(Args&&... args) {
		auto inst = new T(std::forward<Args>(args)...);
		basic_blocks[basic_blocks.size() - 1]->add_instruction(static_cast<Instruction*>(inst));
		return inst;
	}
private:
	uint32_t register_number = { 0 };
	uint32_t num_basic_blocks = { 0 };
	uint32_t num_scopes = { 0 };

	std::vector<BasicBlock*> basic_blocks;
	std::vector<LexicalScope> scopes;
};