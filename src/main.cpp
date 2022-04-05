/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>
#include <fstream>

#include "context.h"
#include "parser.h"
#include "ast.h"

using namespace std;

void interpret_cmdline() {
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

void interpret_file(string filename) {
	ifstream source_file(filename);
	
	if (!source_file.is_open()) {
		cout << "Unable to open file: " << filename << ".\n";
		exit(1);
	}

	stringstream program;
	string line;
	while (getline(source_file, line)) {
		program << line;
	}

	auto ast = parse(program.str());	
	cout << ast->visit() << "\n";

	global_context->dump();
}

int main(int argc, char *argv[]) {
	initialize_global_context();

	global_context->dump();

	if (argc > 1)
		interpret_file(argv[1]);
	else
		interpret_cmdline();
}