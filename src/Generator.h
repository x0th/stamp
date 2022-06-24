/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <vector>
#include <fstream>

#include "Instruction.h"
#include "BasicBlock.h"

#define SCOPE_CAN_CONTINUE  0b1
#define SCOPE_CAN_BREAK     0b10
#define SCOPE_CAN_RETURN    0b100

class Instruction;
class BasicBlock;
class ASTNode;

class LexicalScope {
public:
	LexicalScope(int32_t scope_beginning, uint8_t flags, bool is_global) : is_global(is_global), scope_beginning(scope_beginning), scope_end(-1) {
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
			out += " (can break : " + std::to_string(break_dest) + ")";
		if (can_return)
			out += " (can return)";
		return out;
	}

	void set_continue_dest(uint32_t dest) { continue_dest = dest; }
	uint32_t get_continue_dest() { return continue_dest; }
	void set_break_dest(uint32_t dest) { break_dest = dest; }
	uint32_t get_break_dest() { return break_dest; }

	void end_scope(uint32_t end) {
		for (auto pb : pending_breaks)
			pb->set_jump(end);
		scope_end = end;
	}

	bool starts_at(uint32_t index) { return scope_beginning == (int32_t)index; }
	bool contains(uint32_t index) { return scope_beginning >= (int32_t)index && (int32_t)index <= scope_end; }
	bool ends_at(uint32_t index) { return scope_end == (int32_t)index; }

	void add_pending_break(Jump *pb) { pending_breaks.push_back(pb); }

	void to_file(std::ofstream &outfile) {
		uint8_t indicator = 0xaa;
		uint8_t flags = 0;
		if (can_continue)
			flags = flags | 0b1;
		if (can_break)
			flags = flags | 0b10;
		if (can_return)
			flags = flags | 0b100;
		uint8_t is_glbl = is_global ? 1 : 0;
		outfile.write(reinterpret_cast<char*>(&indicator), sizeof(uint8_t));
		outfile.write(reinterpret_cast<char*>(&is_glbl), sizeof(uint8_t));
		outfile.write(reinterpret_cast<char*>(&scope_beginning), sizeof(int32_t));
		outfile.write(reinterpret_cast<char*>(&flags), sizeof(uint8_t));
		if (can_continue)
			outfile.write(reinterpret_cast<char*>(&continue_dest), sizeof(uint32_t));
		if (can_break)
			outfile.write(reinterpret_cast<char*>(&break_dest), sizeof(uint32_t));
		outfile.write(reinterpret_cast<char*>(&scope_end), sizeof(int32_t));
	}

	static LexicalScope *from_file(std::ifstream &infile) {
		uint8_t is_global;
		infile.read(reinterpret_cast<char*>(&is_global), sizeof(uint8_t));
		int32_t scope_beginning;
		infile.read(reinterpret_cast<char*>(&scope_beginning), sizeof(int32_t));
		uint8_t flags;
		infile.read(reinterpret_cast<char*>(&flags), sizeof(uint8_t));

		auto ls = new LexicalScope(scope_beginning, flags, is_global);

		if (ls->can_continue) {
			uint32_t dest;
			infile.read(reinterpret_cast<char*>(&dest), sizeof(uint32_t));
			ls->set_continue_dest(dest);
		}
		if (ls->can_break) {
			uint32_t dest;
			infile.read(reinterpret_cast<char*>(&dest), sizeof(uint32_t));
			ls->set_break_dest(dest);
		}

		int32_t end;
		infile.read(reinterpret_cast<char*>(&end), sizeof(int32_t));
		ls->end_scope(end);

		return ls;
	}

	bool can_continue = { false };
	bool can_break = { false };
	bool can_return = { false };

	bool is_global = { false };
private:
	int32_t scope_beginning, scope_end;
	uint32_t continue_dest = { 0 };
	uint32_t break_dest = { 0 };

	std::vector<Jump*> pending_breaks;
};

class Generator {
public:
	Generator(std::vector<std::string> &dirs) : dirs(dirs) {}
	~Generator() = default;

	Register next_register();

	BasicBlock *add_basic_block();

	LexicalScope *add_scope_beginning(uint32_t flags, bool can_be_global);
	LexicalScope *add_scope_beginning_current_bb(uint32_t flags, bool can_be_global);
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

	ASTNode *include_from(std::string &filename);
private:
	uint32_t register_number = { 0 };
	uint32_t num_basic_blocks = { 0 };
	uint32_t num_scopes = { 0 };

	std::vector<BasicBlock*> basic_blocks;
	std::vector<LexicalScope*> scopes;

	std::vector<std::string> dirs;
};