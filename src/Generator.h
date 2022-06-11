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
			out += " (can continue : " + std::to_string(continue_dest) + ")";
		if (can_break)
			out += " (can break  : " + std::to_string(break_dest) + ")";
		if (can_return)
			out += " (can return  : " + std::to_string(return_dest) + ")";
		return out;
	}

	void set_continue_dest(uint32_t dest) { continue_dest = dest; }
	uint32_t get_continue_dest() { return continue_dest; }
	void set_break_dest(uint32_t dest) { break_dest = dest; }
	uint32_t get_break_dest() { return break_dest; }
	void set_return_dest(uint32_t dest) { return_dest = dest; }
	uint32_t get_return_dest() { return return_dest; }

	void end_scope(uint32_t end) {
		for (auto pb : pending_breaks)
			pb->set_jump(end);
		scope_end = end;
	}

	bool starts_at(uint32_t index) { return scope_beginning == (int32_t)index; }
	bool ends_at(uint32_t index) { return scope_end == (int32_t)index; }

	void add_pending_break(Jump *pb) { pending_breaks.push_back(pb); }

	bool can_continue = { false };
	bool can_break = { false };
	bool can_return = { false };
private:
	int32_t scope_beginning, scope_end;
	uint32_t continue_dest = { 0 };
	uint32_t break_dest = { 0 };
	uint32_t return_dest = { 0 };

	std::vector<Jump*> pending_breaks;
};

class Generator {
public:
	Generator() {}
	~Generator() = default;

	Register next_register();

	BasicBlock *add_basic_block();

	LexicalScope *add_scope_beginning(uint32_t flags);
	LexicalScope *add_scope_beginning_current_bb(uint32_t flags);
	uint32_t get_num_bbs() const { return num_basic_blocks; }
	std::vector<BasicBlock*> &get_bbs() { return basic_blocks; }
	void end_scope(LexicalScope *scope);

	void dump();
	void dump_basic_blocks();
	void dump_scopes();

	LexicalScope *get_scope(uint32_t index) { return scopes[index]; }
	uint32_t get_num_scopes() const { return num_scopes; }

	template<class T, typename... Args>
	T *append(Args&&... args) {
		auto inst = new T(std::forward<Args>(args)...);
		basic_blocks[basic_blocks.size() - 1]->add_instruction(static_cast<Instruction*>(inst));
		return inst;
	}

	void write_to_file(std::string &filename);
	void read_from_file(std::string &filename);
private:
	uint32_t register_number = { 0 };
	uint32_t num_basic_blocks = { 0 };
	uint32_t num_scopes = { 0 };

	std::vector<BasicBlock*> basic_blocks;
	std::vector<LexicalScope*> scopes;
};