/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <sstream>

#include "Instruction.h"
#include "Register.h"
#include "Interpreter.h"

void Instruction::execute(Interpreter &interpreter) {
#define __INSTRUCTION_TYPES(t)                          \
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
		interpreter.store_at(dst.get_index(), (*object)->send(msg, stamp));
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
#define __INSTRUCTION_TYPES(t)                          \
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
#define __INSTRUCTION_TYPES(t)                          \
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