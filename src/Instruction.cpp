/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <sstream>
#include <cstring>

#include "Instruction.h"
#include "Register.h"
#include "Interpreter.h"

void Instruction::execute(Interpreter &interpreter) {
#define __INSTRUCTION_TYPES(t, b)                       \
		case Instruction::Type::t:                      \
			static_cast<t&>(*this).execute(interpreter);         \
			break;

	switch(type) {
		ENUMERATE_INSTRUCTION_TYPES(__INSTRUCTION_TYPES)
		default:
			// FIXME: Error!
			break;
	}

#undef __INSTRUCTION_TYPES
}

void Load::execute(Interpreter &interpreter) {
	interpreter.store_at(dst.get_index(), interpreter.fetch_object(value));
}

void Store::execute(Interpreter &interpreter) {
	auto object = std::get_if<Object*>(&interpreter.at(obj.get_index()));
	if (object) {
		auto st_register = interpreter.at(store.get_index());
		auto st = std::get_if<Object*>(&st_register);
		if (st)
			(*object)->add_store<StoreObject>(store_name, *st);
		else
			(*object)->add_store<StoreLiteral>(store_name, std::get<std::string>(st_register));
	} else {
		// FIXME: Error!
	}
}

void Send::execute(Interpreter &interpreter) {
	auto object = std::get_if<Object*>(&interpreter.at(obj.get_index()));
	if (object) {
		interpreter.store_at(dst.get_index(), (*object)->send(msg, stamp, interpreter));
	} else {
		// FIXME: Error!
	}
}

void Jump::execute(Interpreter &interpreter) {
	interpreter.jump_bb(block_index);
}

void JumpTrue::execute(Interpreter &interpreter) {
	auto object = std::get_if<Object*>(&interpreter.at(condition.get_index()));
	if (*object == interpreter.fetch_global_object("true"))
		interpreter.jump_bb(block_index);
}

void JumpFalse::execute(Interpreter &interpreter) {
	auto object = std::get_if<Object*>(&interpreter.at(condition.get_index()));
	if (*object == interpreter.fetch_global_object("false"))
		interpreter.jump_bb(block_index);
}

void Instruction::dealloc() {
#define __INSTRUCTION_TYPES(t, b)                          \
		case Instruction::Type::t:                      \
			static_cast<t&>(*this).~t();                   \
			return;

	switch(type) {
		ENUMERATE_INSTRUCTION_TYPES(__INSTRUCTION_TYPES)
		default: {
			// FIXME: Error!
		}
	}

#undef __INSTRUCTION_TYPES
}

std::string Instruction::to_string() const {
#define __INSTRUCTION_TYPES(t, b)                          \
		case Instruction::Type::t:                      \
			return static_cast<t const&>(*this).to_string();

	switch(type) {
		ENUMERATE_INSTRUCTION_TYPES(__INSTRUCTION_TYPES)
		default:
			return "";
	}

#undef __INSTRUCTION_TYPES
}

std::string Load::to_string() const {
	std::stringstream s;
	s << "Load r" << dst.get_index() << ", " << value;
	return s.str();
}

std::string Send::to_string() const {
	std::stringstream s;
	s << "Send r" << dst.get_index() << ", r" << obj.get_index() << ", " << msg;
	if (stamp.has_value()) {
		s << ", ";
		auto *str = std::get_if<std::string>(&*stamp);
		if (str)
			s << *str;
		else {
			auto *bb = std::get_if<uint32_t>(&*stamp);
			if (bb)
				s << "BB" << std::to_string(*bb);
			else
				s << "r" << std::get<Register>(*stamp).get_index();
		}
	}
	return s.str();
}

std::string Store::to_string() const {
	std::stringstream s;
	s << "Store r" << obj.get_index() << ", " << store_name << ", r" << store.get_index();
	return s.str();
}

std::string Jump::to_string() const {
	std::stringstream s;
	s << "Jump BB" << block_index;
	return s.str();
}

std::string JumpTrue::to_string() const {
	std::stringstream s;
	s << "JumpTrue r" << condition.get_index() << ", BB" << block_index;
	return s.str();
}

std::string JumpFalse::to_string() const {
	std::stringstream s;
	s << "JumpFalse r" << condition.get_index() << ", BB" << block_index;
	return s.str();
}

void Instruction::to_file(std::ofstream &outfile) const {
#define __INSTRUCTION_TYPES(t, b)                          \
		case Instruction::Type::t:                      \
			static_cast<t const&>(*this).to_file(outfile, b); break;

	switch(type) {
		ENUMERATE_INSTRUCTION_TYPES(__INSTRUCTION_TYPES)
		default: {
			// FIXME: Error!
		}
	}

#undef __INSTRUCTION_TYPES
}

void Load::to_file(std::ofstream &outfile, uint8_t code) const {
	uint32_t dst_index = dst.get_index();
	outfile.write(reinterpret_cast<char*>(&code), sizeof(uint8_t));
	outfile.write(reinterpret_cast<char*>(&dst_index), sizeof(uint32_t));
	uint32_t size = value.size();
	const char *str = value.c_str();
	outfile.write(reinterpret_cast<char*>(&size), sizeof(uint32_t));
	outfile.write(str, size);
}

void Send::to_file(std::ofstream &outfile, uint8_t code) const {
	uint32_t dst_index = dst.get_index();
	uint32_t obj_index = obj.get_index();
	uint32_t msg_size = msg.size();
	const char *msg_str = msg.c_str();
	outfile.write(reinterpret_cast<char*>(&code), sizeof(uint8_t));
	outfile.write(reinterpret_cast<char*>(&dst_index), sizeof(uint32_t));
	outfile.write(reinterpret_cast<char*>(&obj_index), sizeof(uint32_t));
	outfile.write(reinterpret_cast<char*>(&msg_size), sizeof(uint32_t));
	outfile.write(msg_str, msg_size);

	uint8_t stamp_type = 0x00;
	if (stamp.has_value()) {
		stamp_type = std::holds_alternative<Register>(*stamp) ? 0x01 :
				(std::holds_alternative<std::string>(*stamp) ? 0x02 : 0x03);
	}
	outfile.write(reinterpret_cast<char*>(&stamp_type), sizeof(uint8_t));
	if (stamp_type == 0x01) {
		uint32_t st = std::get<Register>(*stamp).get_index();
		outfile.write(reinterpret_cast<char*>(&st), sizeof(uint32_t));
	} else if (stamp_type == 0x02) {
		std::string st = std::get<std::string>(*stamp);
		uint32_t size = st.size();
		const char *st_str = st.c_str();
		outfile.write(reinterpret_cast<char*>(&size), sizeof(uint32_t));
		outfile.write(st_str, size);
	} else if (stamp_type == 0x03) {
		uint32_t st = std::get<uint32_t>(*stamp);
		outfile.write(reinterpret_cast<char*>(&st), sizeof(uint32_t));
	}
}

void Store::to_file(std::ofstream &outfile, uint8_t code) const {
	uint32_t obj_index = obj.get_index();
	uint32_t store_size = store_name.size();
	const char *store_str = store_name.c_str();
	uint32_t store_index = store.get_index();
	outfile.write(reinterpret_cast<char*>(&code), sizeof(uint8_t));
	outfile.write(reinterpret_cast<char*>(&obj_index), sizeof(uint32_t));
	outfile.write(reinterpret_cast<char*>(&store_size), sizeof(uint32_t));
	outfile.write(store_str, store_size);
	outfile.write(reinterpret_cast<char*>(&store_index), sizeof(uint32_t));
}

void Jump::to_file(std::ofstream &outfile, uint8_t code) const {
	outfile.write(reinterpret_cast<char*>(&code), sizeof(uint8_t));
	// have to do this because otherwise reinterpret_cast does not like it
	uint32_t b_index = block_index;
	outfile.write(reinterpret_cast<char*>(&b_index), sizeof(uint32_t));
}

void JumpTrue::to_file(std::ofstream &outfile, uint8_t code) const {
	outfile.write(reinterpret_cast<char*>(&code), sizeof(uint8_t));
	// have to do this because otherwise reinterpret_cast does not like it
	uint32_t b_index = block_index;
	outfile.write(reinterpret_cast<char*>(&b_index), sizeof(uint32_t));
	uint32_t condition_index = condition.get_index();
	outfile.write(reinterpret_cast<char*>(&condition_index), sizeof(uint32_t));
}

void JumpFalse::to_file(std::ofstream &outfile, uint8_t code) const {
	outfile.write(reinterpret_cast<char*>(&code), sizeof(uint8_t));
	// have to do this because otherwise reinterpret_cast does not like it
	uint32_t b_index = block_index;
	outfile.write(reinterpret_cast<char*>(&b_index), sizeof(uint32_t));
	uint32_t condition_index = condition.get_index();
	outfile.write(reinterpret_cast<char*>(&condition_index), sizeof(uint32_t));
}

Instruction* Instruction::from_file(std::ifstream &infile, uint8_t code) {
#define __INSTRUCTION_TYPES(t, b) \
    case b: \
	return static_cast<Instruction *>(t::from_file(infile));

	switch (code) {
		ENUMERATE_INSTRUCTION_TYPES(__INSTRUCTION_TYPES)
		default: {
			// Fixme: Error!
			return nullptr;
		}
	}

#undef __INSTRUCTION_TYPES
}

Load *Load::from_file(std::ifstream &infile) {
	uint32_t index;
	infile.read(reinterpret_cast<char*>(&index), sizeof(uint32_t));
	uint32_t size;
	infile.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
	char value[size+1];
	infile.read(reinterpret_cast<char*>(&value), size);
	value[size] = '\0';

	return new Load(Register(index), value);
}

Send *Send::from_file(std::ifstream &infile) {
	uint32_t dst;
	infile.read(reinterpret_cast<char*>(&dst), sizeof(uint32_t));
	uint32_t obj;
	infile.read(reinterpret_cast<char*>(&obj), sizeof(uint32_t));
	uint32_t msg_size;
	infile.read(reinterpret_cast<char*>(&msg_size), sizeof(uint32_t));
	char msg[msg_size+1];
	infile.read(reinterpret_cast<char*>(&msg), msg_size);
	msg[msg_size] = '\0';

	uint8_t stamp_type;
	infile.read(reinterpret_cast<char*>(&stamp_type), sizeof(uint8_t));
	if (stamp_type == 0x00) {
		std::optional<uint32_t> st = std::nullopt;
		return new Send(dst, obj, msg, st);
	} else if (stamp_type == 0x01) {
		uint32_t r_index;
		infile.read(reinterpret_cast<char*>(&r_index), sizeof(uint32_t));
		std::optional<Register> st = Register(r_index);
		return new Send(dst, obj, msg, st);
	} else if (stamp_type == 0x02) {
		uint32_t st_size;
		infile.read(reinterpret_cast<char*>(&st_size), sizeof(uint32_t));
		char stamp[st_size+1];
		infile.read(reinterpret_cast<char*>(&stamp), st_size);
		stamp[st_size] = '\0';
		std::optional<std::string> st = std::string(stamp);
		return new Send(dst, obj, msg, st);
	} else if (stamp_type == 0x03) {
		uint32_t bb_index;
		infile.read(reinterpret_cast<char*>(&bb_index), sizeof(uint32_t));
		std::optional<uint32_t> st = bb_index;
		return new Send(dst, obj, msg, st);
	} else {
		// FIXME: Error!
		return nullptr;
	}
}

Store *Store::from_file(std::ifstream &infile) {
	uint32_t dst;
	infile.read(reinterpret_cast<char*>(&dst), sizeof(uint32_t));;
	uint32_t store_size;
	infile.read(reinterpret_cast<char*>(&store_size), sizeof(uint32_t));
	char store_name[store_size+1];
	infile.read(reinterpret_cast<char*>(&store_name), store_size);
	store_name[store_size] = '\0';
	uint32_t store;
	infile.read(reinterpret_cast<char*>(&store), sizeof(uint32_t));

	return new Store(dst, store_name, store);
}

Jump *Jump::from_file(std::ifstream &infile) {
	uint32_t block_index;
	infile.read(reinterpret_cast<char*>(&block_index), sizeof(uint32_t));

	return new Jump(block_index);
}

JumpTrue *JumpTrue::from_file(std::ifstream &infile) {
	uint32_t block_index;
	infile.read(reinterpret_cast<char*>(&block_index), sizeof(uint32_t));
	uint32_t condition;
	infile.read(reinterpret_cast<char*>(&condition), sizeof(uint32_t));

	return new JumpTrue(block_index, Register(condition));
}

JumpFalse *JumpFalse::from_file(std::ifstream &infile) {
	uint32_t block_index;
	infile.read(reinterpret_cast<char*>(&block_index), sizeof(uint32_t));
	uint32_t condition;
	infile.read(reinterpret_cast<char*>(&condition), sizeof(uint32_t));

	return new JumpFalse(block_index, Register(condition));
}