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
