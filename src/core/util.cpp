//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
//
//  NOTE: This software is licensed under the GPL. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  Jonathan Rennison (or anybody else) is in no way responsible, or liable
//  for this program or its use in relation to users, 3rd parties or to any
//  persons in any way whatsoever.
//
//  You  should have  received a  copy of  the GNU  General Public
//  License along  with this program; if  not, write to  the Free Software
//  Foundation, Inc.,  59 Temple Place,  Suite 330, Boston,  MA 02111-1307
//  USA
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "util.h"
#include <cstring>
#include <cstdarg>
#include <cstdio>
#ifdef _WIN32
#include <Windows.h>
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
	#ifdef _WIN32		//%z is broken in MSVCRT, use a replacement
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

size_t GetLineNumberOfStringOffset(const std::string &input, size_t offset, size_t *linestart, size_t *lineend) {
	size_t lineno = 1;
	size_t i;
	for(i = 0; i < input.size() && i <= offset; i++) {
		if(input[i] == '\r') {
			lineno++;
			if(input[i+1] == '\n') i++;
			if(linestart) *linestart = i+1;
		}
		else if(input[i] == '\n') {
			lineno++;
			if(linestart) *linestart = i+1;
		}
	}
	if(lineend) {
		*lineend = input.size()-1;
		for(; i < input.size(); i++) {
			if(input[i] == '\r' || input[i] == '\n') {
				*lineend = i;
				break;
			}
		}
	}
	return lineno;
}
