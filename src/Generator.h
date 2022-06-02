/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <vector>

#include "Instruction.h"

class Generator {
public:
	Generator() {}
	~Generator();

	Register next_register();

	void dump();

	template<class T, typename... Args>
	void append(Args&&... args) {
		instructions.push_back(static_cast<Instruction*>(new T(std::forward<Args>(args)...)));
	}
private:
	uint32_t register_number = { 0 };

	std::vector<Instruction*> instructions;
};