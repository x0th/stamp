/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>
#include <fstream>

//#include "context.h"
#include "Parser.h"
#include "AST.h"
#include "Generator.h"

using namespace std;

void interpret_cmdline() {
	while (1) {
		cout << "> ";
		string line;
		getline(cin, line);
		vector<string> program;
		program.push_back(line);
		string filename;

		auto ast = parse(filename, program);
		if (!ast)
			continue;

#ifdef DEBUG
		cout << ast->to_string();
#endif

		//cout << ast->visit() << "\n";

#ifdef DEBUG
		//global_context->dump();
#endif
	}
}

void interpret_file(string filename) {
	ifstream source_file(filename);
	
	if (!source_file.is_open()) {
		cout << "Unable to open file: " << filename << ".\n";
		exit(1);
	}

	vector<string> program;
	string line;
	while (getline(source_file, line)) {
		program.push_back(line);
	}

	auto ast = parse(filename, program);
	if (!ast)
		return;

	std::cout << ast->to_string();

	Generator generator;
	ast->generate_bytecode(generator);
	generator.dump();

	#ifdef DEBUG
		//cout << ast->to_string();
	#endif

	// cout << ast->visit() << "\n";

#ifdef DEBUG
	//global_context->dump();
#endif
}

int main(int argc, char *argv[]) {
	//initialize_global_context();

#ifdef DEBUG
	//global_context->dump();
#endif

	if (argc > 1)
		interpret_file(argv[1]);
	else
		interpret_cmdline();
}