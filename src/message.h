/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vector>
#include <string>

using namespace std;

class ASTNode;
class Object;

class Message {
public:
	Message(string name, ASTNode *sender, Object *requester);

	string get_name() const { return name; }
	ASTNode *get_sender();
	Object *get_requester();
	void set_requester(Object *);
	string to_string() const;

private:
	string name;
	ASTNode *sender;
	Object *requester;
};