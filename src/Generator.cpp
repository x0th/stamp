/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>

#include "Generator.h"
#include "Register.h"

Generator::~Generator() {
	for (auto i : instructions) {
		i->dealloc();
	}
}

Register Generator::next_register() {
	return Register(register_number++);
}

void Generator::dump() {
	for (auto const &i : instructions)
		std::cout << i->to_string() << "\n";
}