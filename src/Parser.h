/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "AST.h"

ASTNode *parse(std::string&, std::vector<std::string>&, Generator *gen);
std::string error_msg(std::string error);