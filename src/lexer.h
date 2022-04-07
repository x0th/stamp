/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>

#include "token.h"

static const int MAX_TOKEN_LEN = 128;

Token scan(std::string &raw_string, long unsigned int *position);