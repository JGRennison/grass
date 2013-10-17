//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
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

#ifndef INC_ERROR_ALREADY
#define INC_ERROR_ALREADY

#include <memory>
#include <list>
#include <string>
#include <sstream>

class error_obj {
	protected:
	std::stringstream msg;
	time_t timestamp;
	unsigned int millitimestamp;

	public:
	error_obj();
	virtual ~error_obj() { }
	void StreamOut(std::ostream& os) const;
};

class generic_error_obj : public error_obj {
	public:
	generic_error_obj(const std::string &str) {
		msg << str;
	}
};

class error_collection {
	std::list<std::unique_ptr<error_obj> > errors;

	public:
	void RegisterError(std::unique_ptr<error_obj> &&err);
	template <typename C, typename... Args> void RegisterNewError(Args&& ...msg) {
		RegisterError(std::unique_ptr<error_obj>(new C(msg...)));
	}
	void Reset();
	unsigned int GetErrorCount() const;
	void StreamOut(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const error_obj& obj);
std::ostream& operator<<(std::ostream& os, const error_collection& obj);

#endif
