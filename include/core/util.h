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

#ifndef INC_UTIL_ALREADY
#define INC_UTIL_ALREADY

#include <ctime>
#include <string>
#include <sstream>
#include <iterator>
#include <vector>
#include <utility>

class error_collection;

//adapted from http://forums.devshed.com/showpost.php?p=2678621&postcount=9
//Newton-Raphson
template <typename I> I fast_isqrt( I n ){

	I p = 1;
	while (p <= n/p) p <<= 1;

	I r = p; // initial guess: isqrt(n) < r

	do {
		p = r;
		r = (r + n/r) / 2;
	} while(r < p);

	return p;
}

std::string string_format(const std::string &fmt, ...);
std::string gr_strftime(const std::string &format, const struct tm *tm, time_t timestamp, bool localtime);
unsigned int GetMilliTime();
size_t GetLineNumberOfStringOffset(const std::string &input, size_t offset, size_t *linestart = 0, size_t *lineend = 0);

template <typename I> I swap_single_bits(I in, I bit1, I bit2) {
	bool rev = !(in & bit1) != !(in & bit2);
	if(rev) return in ^ (bit1 | bit2);
	else return in;
}

template <typename I> I SetOrClearBits(I in, I bits, bool set) {
	if(set) return in | bits;
	else in &= ~bits;
	return in;
}

template <typename I> void SetOrClearBitsRef(I &in, I bits, bool set) {
	in = SetOrClearBits(in, bits, set);
}

template <typename C, typename UP> unsigned int container_unordered_remove_if(C &container, UP predicate) {
	unsigned int removecount = 0;
	for(auto it = container.begin(); it != container.end();) {
		if(predicate(*it)) {
			removecount++;
			if(std::next(it) != container.end()) {
				*it = std::move(container.back());
				container.pop_back();
			}
			else {
				container.pop_back();
				break;
			}
		}
		else ++it;
	}
	return removecount;
}

template <typename C, typename V> unsigned int container_unordered_remove(C &container, const V &value) {
	return container_unordered_remove_if(container, [&](const V &current) {
		return current == value;
	});
}

void SplitString(const char *in, size_t len, char delim, std::vector<std::pair<const char*, size_t>> &result);
std::vector<std::string> TokenizeString(const std::string &str, const std::string &delimiters);

bool slurp_file(const std::string &filename, std::string &out, error_collection &ec);

template <typename T> std::string stringify(const T &in) {
	std::stringstream msg;
	msg << in;
	return msg.str();
}

#endif
