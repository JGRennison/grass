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

#include "error.h"
#include "util.h"

void error_collection::RegisterError(std::unique_ptr<error_obj> &&err) {
	errors.emplace_back(std::move(err));
}

void error_collection::Reset() {
	errors.clear();
}

unsigned int error_collection::GetErrorCount() const {
	return errors.size();
}

void error_collection::StreamOut(std::ostream& os) const {
	os << "Errors: " << errors.size() << "\n";
	for(auto it=errors.begin(); it!=errors.end(); ++it) {
		const error_obj& obj = **it;
		os << obj;
	}
}

void error_obj::StreamOut(std::ostream& os) const {
	os << msg.str() << "\n";
}

error_obj::error_obj() {
	millitimestamp = GetMilliTime();
	timestamp = time(0);
	msg << gr_strftime(string_format("[%%F %%T.%03d %%z] Error: ", millitimestamp), localtime(&timestamp), timestamp, true);
}

std::ostream& operator<<(std::ostream& os, const error_obj& obj) {
	obj.StreamOut(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, const error_collection& obj) {
	obj.StreamOut(os);
	return os;
}