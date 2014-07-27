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

#ifndef INC_COMMON_ALREADY
#define INC_COMMON_ALREADY

typedef unsigned int world_time;

#ifdef __MINGW32__

#include <string>
#include <sstream>

//fix for MinGW, from http://pastebin.com/7rhvv92A
namespace std
{
    template <typename T>
    string to_string(const T & value)
    {
        stringstream stream;
        stream << value;
        return stream.str();// string_stream;
    }
}
#endif

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

#endif
