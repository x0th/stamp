/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <iostream>
#include <string>

#define ENUMERATE_ERROR_TYPES(T) \
    T(LexingError, "LexingError")          \
	T(ParsingError, "ParsingError")        \
	T(BytecodeGenerationError, "BytecodeGenerationError") \
	T(DefaultStoreError, "DefaultStoreError") \
	T(FileParsingError, "FileParsingError")   \
	T(ExecutionError, "ExecutionError")

enum class StampError {
#define __ERROR_TYPES(t, s) \
t,
	ENUMERATE_ERROR_TYPES(__ERROR_TYPES)
#undef __ERROR_TYPES
};

inline void terminating_error(StampError error, std::string message) {
#define __PRINT_ERROR(t, s) \
	case StampError::t: std::cerr << s; break;

	switch (error) {
		ENUMERATE_ERROR_TYPES(__PRINT_ERROR)
	}

	std::cerr << ": " << message << "\n";
	exit(1);
#undef __PRINT_ERROR
}