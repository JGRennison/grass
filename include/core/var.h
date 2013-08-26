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

#ifndef INC_VAR_ALREADY
#define INC_VAR_ALREADY

#include <string>
#include <functional>
#include <cctype>
#include <map>

struct message_formatter {
	typedef std::function<std::string(std::string)> mf_lambda;
	std::map<std::string, mf_lambda> registered_variables;

	public:
	std::string FormatMessage(const std::string &input) const;
	void RegisterVariable(const std::string &name, mf_lambda func);

	private:
	std::string ExpandVariable(std::string::const_iterator &begin, std::string::const_iterator end) const;

	inline bool IsVarChar(char c) const {
		return isalpha(c) || (c == '-');
	}
};

#endif
