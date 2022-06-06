/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <map>
#include <string>

#include "Object.h"

class Context {
public:
	Context() {}

	// FIXME: maybe error when name is taken?
	void add(std::string name, Object *object);
	Object *get(std::string &name);
	void dump();

	static Context *make_global_context();
private:
	std::map<std::string, Object*> context;
};