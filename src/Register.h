/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <cstdint>

class Register {
public:
	Register(uint32_t index) : index(index) {}

	uint32_t get_index() const { return index; }

private:
	uint32_t index;
};