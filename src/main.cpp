/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>

#include "context.h"
#include "parser.h"
#include "ast.h"

using namespace std;

void interpret() {
	while (1) {
		cout << "> ";
		string line;
		getline(cin, line);

		auto ast = parse(line);

		cout << ast->to_string();

		cout << ast->visit() << "\n";

		global_context->dump();
	}
}

int main() {
	initialize_global_context();

	global_context->dump();

	interpret();
}