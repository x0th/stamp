/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "Object.h"

std::string Object::to_string() const {
	std::stringstream s;
	s << type << "-" << std::hex << hash;
	return s.str();
}