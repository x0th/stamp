/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <iomanip>
#include <map>
#include <iostream>

#include "message.h"
#include "token.h"

using namespace std;

class Object;
class ASTNode;
class Context;

class Store {
public:
	enum class Type { Object, Literal, Executable, List, Int, Char };

	Store(Object *_obj) {
		store_type = Type::Object;
		obj = _obj;
	}

	Store(string *_lit) {
		store_type = Type::Literal;
		lit = _lit;
	}

	Store(ASTNode *_exe) {
		store_type = Type::Executable;
		exe = _exe;
	}

	Store(vector<Store *> *_list) {
		store_type = Type::List;
		list = _list;
	}

	Store(int i) {
		store_type = Type::Int;
		integer = i;
	}

    Store(char c) {
        store_type = Type::Char;
        character = c;
    }

	string *get_lit() const { return lit; }
	Object *get_obj() const { return obj; }
	ASTNode *get_exe() const { return exe; }
	vector<Store *> *get_list() const { return list; }
	Type get_store_type() const { return store_type; }

private:
	Type store_type;
	union {
		Object *obj;
		string *lit;
		ASTNode *exe;
		vector<Store *> *list;
		int integer;
		char character;
	};
};

class Object {
public:
	Object(Object *_prototype);

	int get_hash() const { return hash; }
	map<string, Store *> &get_stores() { return stores; }
	Context *get_context() { return context; }

	string to_string();
	
	Store *send(Message &message, string *out);
	void store_obj(string store_name, Object *obj);
	void store_lit(string store_name, string *lit);
	void store_exe(string store_name, ASTNode *exe);
	void store_list(string store_name, vector<Store *> *list);
	void store_int(string store_name, int integer);
	void store_char(string store_name, char character);
	
	Object *clone(ASTNode *sender);
	Object *clone_list(ASTNode *sender);
	Object *clone_callable(ASTNode *sender);
private:
	int hash;
	Object *prototype;
	map<string, Store *> stores;
	Context *context;

	Store *handle_default(string &lit, ASTNode *sender, string *out, Object *reqester);
};
