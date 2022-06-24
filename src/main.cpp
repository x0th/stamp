/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <iostream>
#include <optional>
#include <vector>
#include <string>

#include "Parser.h"
#include "Generator.h"
#include "Interpreter.h"

bool dump_ast = false;
bool dump_bytecode = false;
bool dump_all_registers = false;
bool generate_bytecode_file = false;
std::optional<std::string> bytecode_file = std::nullopt;
bool interpret_from_bytecode_file = false;
std::vector<std::string> dirs{"."};

void interpret_cmdline() {
	Generator generator(dirs);
	std::string prelude = "prelude.ostamp";
	generator.read_from_file(prelude);

	Interpreter interpreter(generator);

	while (1) {
		std::cout << "> ";
		std::string line;
		getline(std::cin, line);
		std::vector<std::string> program;
		program.push_back(line);
		std::string filename;

		auto ast = parse(filename, program, &generator);
		if (!ast)
			continue;

		if (dump_ast)
			std::cout << ast->to_string() << "\n";

		ast->generate_bytecode(generator);

		if (dump_bytecode)  {
			generator.dump_basic_blocks();
			generator.dump_scopes();
		}

		if (generate_bytecode_file)
			generator.write_to_file(*bytecode_file);

		interpreter.run();
		std::cout << "\n";
		interpreter.dump();
	}
}

void interpret_file(std::string &filename) {
	Generator generator(dirs);
	std::string prelude = "prelude.ostamp";
	generator.read_from_file(prelude);

	if (interpret_from_bytecode_file)
		generator.read_from_file(filename);
	else {
		auto ast = generator.include_from(filename);
		if (!ast)
			return;

		if (dump_ast)
			std::cout << ast->to_string() << "\n";

		ast->generate_bytecode(generator);
	}

	if (dump_bytecode)  {
		generator.dump_basic_blocks();
		generator.dump_scopes();
	}

	if (generate_bytecode_file)
		generator.write_to_file(*bytecode_file);

	Interpreter interpreter(generator);
	interpreter.run();
	std::cout << "\n";
	interpreter.dump();
}

void help_message() {
	printf("Usage: stamp [-h] [-a] [-b] [-r] [-o [bytecode_file]] [-f bytecode_input] [-d dirs] [input_file]\n\n");
	printf("Arguments:\n");
	printf("-h                  Print this help message and exit.\n");
	printf("-a                  Print the output abstract syntax tree.\n");
	printf("-b                  Print the generated bytecode.\n");
	printf("-r                  Print all register values after an interpreter run.\n");
	printf("-o [bytecode_file]  Output generated bytecode to bytecode_file. If no bytecode_file is given, the name of the file will be parsed from input_file.\n");
	printf("-f bytecode_input   Take input from a bytecode file bytecode_input.\n");
	printf("-d dirs             Specifies which directories to search for use keyword. dirs is a comma-separated list of directories.\n");
}

int main(int argc, char *argv[]) {
	std::optional<std::string> filename = std::nullopt;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'a':
					dump_ast = true;
					break;
				case 'b':
					dump_bytecode = true;
					break;
				case 'r':
					dump_all_registers = true;
					break;
				case 'h':
					help_message();
					exit(0);
				case 'o': {
					if (i + 1 == argc) {
						std::cout << "No file given for -o option.\n";
						exit(1);
					}
					generate_bytecode_file = true;
					if (argv[i + 1][0] != '-') {
						i += 1;
						bytecode_file = argv[i];
					}
					break;
				}
				case 'f': {
					if (i + 1 == argc || argv[i + 1][0] == '-') {
						std::cerr << "No file given for -f option.\n";
						exit(1);
					}
					interpret_from_bytecode_file = true;
					if (argv[i + 1][0] != '-') {
						i += 1;
						filename = argv[i];
					}
					break;
				}
				case 'd': {
					if (i + 1 == argc || argv[i + 1][0] == '-') {
						std::cerr << "No source file directories given for -d option.\n";
						exit(1);
					}
					i += 1;
					std::string dirlist = argv[i];
					auto start = 0;
					auto end = dirlist.find(",");
					do {
						dirs.push_back(dirlist.substr(start, end - start));
						start = end + 1;
						end = dirlist.find(",", start);
					} while (end != std::string::npos);
					break;
				}
				default:
					std::cerr << "Unrecognized option: " << argv[i][1] << "\n";
			}
		} else {
			filename = argv[i];
		}
	}

	if (generate_bytecode_file) {
		if (!bytecode_file.has_value()) {
			std::string clean_filename = (*filename).substr(0, (*filename).find(".stamp"));
			bytecode_file = clean_filename + ".ostamp";
		} else if (!filename.has_value()) {
			std::string clean_filename = (*bytecode_file).substr(0, (*bytecode_file).find(".stamp"));
			if (clean_filename == bytecode_file)
				filename = clean_filename;
			else
				filename = bytecode_file;
			bytecode_file = clean_filename + ".ostamp";
		}
	}

	if (filename.has_value())
		interpret_file(*filename);
	else
		interpret_cmdline();
}