/*
 * SRT - Secure, Reliable, Transport
 * Copyright (c) 2018 Haivision Systems Inc.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * 
 */

#ifndef INC_SRT_LOGSUPPORT_HPP
#define INC_SRT_LOGSUPPORT_HPP

#include "../srtcore/srt.h"
#include "../srtcore/logging_api.h"

srt_logging::LogLevel::type SrtParseLogLevel(std::string level);
std::set<srt_logging::LogFA> SrtParseLogFA(std::string fa, std::set<std::string>* punknown = nullptr);

SRT_API extern std::map<std::string, int> srt_level_names;


#endif
