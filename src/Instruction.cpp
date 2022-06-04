/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <sstream>

#include "Instruction.h"

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
		else
			s << "r" << std::get<Register>(*stamp).get_index();
	}
	return s.str();
}

std::string Store::to_string() const {
	std::stringstream s;
	s << "Store r" << obj.get_index() << ", " << store_name << ", r" << store.get_index();
	return s.str();
}

std::string JumpTrue::to_string() const {
	std::stringstream s;
	s << "JumpTrue r" << condition.get_index() << ", " << block_index;
	return s.str();
}

std::string JumpFalse::to_string() const {
	std::stringstream s;
	s << "JumpFalse r" << condition.get_index() << ", " << block_index;
	return s.str();
}