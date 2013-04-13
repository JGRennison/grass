//  grass - Generic Rail And Signalling Simulator
//
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

#ifndef INC_DESERIALISATION_SCALARCONV_ALREADY
#define INC_DESERIALISATION_SCALARCONV_ALREADY

#include <string>

class error_collection;

namespace dsconv {
	bool Length(const std::string &in, uint64_t &out, error_collection &ec);
	bool Speed(const std::string &in, uint64_t &out, error_collection &ec);
	bool Time(const std::string &in, uint64_t &out, error_collection &ec);
	bool Force(const std::string &in, uint64_t &out, error_collection &ec);
	bool Mass(const std::string &in, uint64_t &out, error_collection &ec);
	bool Power(const std::string &in, uint64_t &out, error_collection &ec);
	bool ForcePerSpeedCoeff(const std::string &in, uint64_t &out, error_collection &ec);
	bool ForcePerSpeedSqCoeff(const std::string &in, uint64_t &out, error_collection &ec);
}

#endif
