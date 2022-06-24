/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>
#include <stdio.h>

#include "Generator.h"
#include "Register.h"
#include "Error.h"
#include "AST.h"
#include "Parser.h"

Register Generator::next_register() {
	return Register(register_number++);
}

BasicBlock *Generator::add_basic_block() {
	basic_blocks.push_back(new BasicBlock(num_basic_blocks));
	return basic_blocks[num_basic_blocks++];
}

LexicalScope *Generator::add_scope_beginning(uint32_t flags, bool can_be_global) {
	if (can_be_global && (scopes.empty() || scopes[num_scopes - 1]->is_global))
		scopes.push_back(new LexicalScope(add_basic_block()->get_index(), flags, can_be_global));
	else
		scopes.push_back(new LexicalScope(add_basic_block()->get_index(), flags, false));
	return scopes[num_scopes++];
}

LexicalScope *Generator::add_scope_beginning_current_bb(uint32_t flags, bool can_be_global) {
	scopes.push_back(new LexicalScope(basic_blocks[num_basic_blocks - 1]->get_index(), flags, can_be_global));
	return scopes[num_scopes++];
}

void Generator::end_scope(LexicalScope *scope) {
	scope->end_scope(basic_blocks[num_basic_blocks - 1]->get_index());
}

void Generator::dump() {
	for (auto const &bb : basic_blocks)
		std::cout << bb->to_string();
}

void Generator::dump_basic_blocks() {
	for (auto const &bb : basic_blocks)
		bb->dump();
}

void Generator::dump_scopes() {
	std::cout << "Lexical scopes:\n";
	for (auto const &scope : scopes)
		std::cout << scope->to_string() << "\n";
}

void Generator::write_to_file(std::string &filename) {
	std::ofstream outfile(filename, std::ios::binary);

	for (auto bb : basic_blocks) {
		uint8_t bbyte = 0xbb;
		outfile.write(reinterpret_cast<char*>(&bbyte), sizeof(uint8_t));
		for (auto instr : bb->get_instructions()) {
			instr->to_file(outfile);
		}
	}

	for (auto ls : scopes) {
		ls->to_file(outfile);
	}

	outfile.close();
}

void Generator::read_from_file(std::string &filename) {
	std::ifstream infile(filename, std::ios::binary);

	BasicBlock *bb;
	while (!infile.eof()) {
		uint8_t first_byte = 0x00;
		infile.read(reinterpret_cast<char*>(&first_byte), sizeof(uint8_t));
		if (first_byte == 0xbb)
			bb = add_basic_block();
		else if (first_byte == 0xaa) {
			scopes.push_back(LexicalScope::from_file(infile));
			num_scopes++;
		} else if (first_byte == 0x00)
			break;
		else if (bb) {
			auto instruction = Instruction::from_file(infile, first_byte);
			bb->add_instruction(instruction);
			auto biggest_reg = instruction->biggest_reg;
			if (biggest_reg > register_number)
				register_number = biggest_reg + 1;
		} else {
			terminating_error(StampError::FileParsingError, "Unexpected first byte of file: " + std::to_string(first_byte) + ". The file might not be an object-stamp file.");
		}
	}

	infile.close();
}

ASTNode *Generator::include_from(std::string &filename) {
	std::string resolved_filename;
	if (filename.find(".stamp") != std::string::npos)
		resolved_filename = filename;
	else
		resolved_filename = filename + ".stamp";
	for (auto const &dir : dirs) {
		// FIXME: only works on unix systems
		auto temp = dir + "/" + resolved_filename;
		std::ifstream test(temp);
		if (test.good()) {
			resolved_filename = temp;
			break;
		}
	}

	std::ifstream source_file(resolved_filename);

	if (!source_file.is_open()) {
		std::cerr << "Cannot find file to use: " << filename << ".\n";
		exit(1);
	}

	std::vector<std::string> program;
	std::string line;
	while (getline(source_file, line)) {
		program.push_back(line);
	}

	return parse(filename, program, this);
}