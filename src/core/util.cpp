//  grass - Generic Rail And Signalling Simulator
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "core/util.h"
#include "core/error.h"
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

//from http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf#2342176
std::string string_format(const std::string &fmt, ...) {
    int size = 100;
    std::string str;
    va_list ap;
    while (1) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.c_str(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            return str;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }
    return str;
}

std::string gr_strftime(const std::string &format, const struct tm *tm, time_t timestamp, bool localtime) {
	#ifdef _WIN32
		//%z is broken in MSVCRT, use a replacement
		//also add %F, %R, %T, %s
		//this is adapted from retcon tpanel.cpp adapted from npipe var.cpp

	std::string newfmt;
	std::string &real_format=newfmt;
	const char *ch=format.c_str();
	const char *cur=ch;
	while(*ch) {
		if(ch[0]=='%') {
			std::string insert;
			if(ch[1]=='z') {
				int hh;
				int mm;
				if(localtime) {
					TIME_ZONE_INFORMATION info;
					DWORD res = GetTimeZoneInformation(&info);
					int bias = - info.Bias;
					if(res==TIME_ZONE_ID_DAYLIGHT) bias-=info.DaylightBias;
					hh = bias / 60;
					if(bias<0) bias=-bias;
					mm = bias % 60;
				}
				else {
					hh=mm=0;
				}
				insert=string_format("%+03d%02d", hh, mm);
			}
			else if(ch[1]=='F') {
				insert="%Y-%m-%d";
			}
			else if(ch[1]=='R') {
				insert="%H:%M";
			}
			else if(ch[1]=='T') {
				insert="%H:%M:%S";
			}
			else if(ch[1]=='s') {
				insert=std::to_string(timestamp);
			}
			else if(ch[1]) {
				ch++;
			}
			if(insert.size()) {
				real_format += std::string(cur, ch-cur);
				real_format += insert;
				cur=ch+2;
			}
		}
		ch++;
	}
	real_format += cur;
	#else
	const std::string &real_format=format;
	#endif

	char timestr[256];
	strftime(timestr, sizeof(timestr), real_format.c_str(), tm);
	return std::string(timestr);
}

unsigned int GetMilliTime() {
	unsigned int ms;
	#ifdef _WIN32
	SYSTEMTIME time;
	GetSystemTime(&time);
	ms=time.wMilliseconds;
	#else
	timeval time;
	gettimeofday(&time, NULL);
	ms=(time.tv_usec / 1000);
	#endif
	return ms;
}

//linestart is inclusive limit, lineen is exclusive limit
size_t GetLineNumberOfStringOffset(const std::string &input, size_t offset, size_t *linestart, size_t *lineend) {
	size_t lineno = 1;
	size_t i;
	for(i = 0; i < input.size() && i <= offset; i++) {
		if(input[i] == '\r') {
			lineno++;
			if(input[i+1] == '\n') i++;
			if(linestart) *linestart = i + 1;
		}
		else if(input[i] == '\n') {
			lineno++;
			if(linestart) *linestart = i + 1;
		}
	}
	if(lineend) {
		*lineend = input.size();
		for(; i < input.size(); i++) {
			if(input[i] == '\r' || input[i] == '\n') {
				*lineend = i;
				break;
			}
		}
	}
	return lineno;
}

void SplitString(const char *in, size_t len, char delim, std::vector<std::pair<const char*, size_t>> &result) {
	result.clear();
	size_t pos = 0;
	for(size_t i = 0; i < len; i++) {
		if(in[i] == delim) {
			result.emplace_back(in + pos, i - pos);
			pos = i + 1;
		}
	}
	result.emplace_back(in + pos, len - pos);
}

bool slurp_file(const std::string &filename, std::string &out, error_collection &ec) {
	out.clear();
	std::ifstream ifs(filename, std::ios_base::binary);
	if(!ifs.is_open()) {
		ec.RegisterNewError<generic_error_obj>(string_format("Error reading file: '%s'", filename.c_str()));
		return false;
	}
	ifs.seekg(0, std::ios::end);
	out.reserve(ifs.tellg());
	ifs.seekg(0, std::ios::beg);
	out.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	out.push_back(0);
	return true;
}
