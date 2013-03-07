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

#include "common.h"
#include "textpool.h"

textpool::textpool() {
	RegisterNewText("textpool/keynotfound", "textpool: No such key: ");
}

void textpool::RegisterNewText(const std::string &key, const std::string &text) {
	textmap[key] = text;
}

std::string textpool::GetTextByName(const std::string &key) const {
	auto text = textmap.find(key);
	if(text != textmap.end()) return text->second;
	else return textmap.at("textpool/keynotfound") + key;
}

void textpool::Deserialise(const deserialiser_input &di, error_collection &ec) {
	//code goes here
}


defaultusermessagepool::defaultusermessagepool() {
	RegisterNewText("track_ops/pointsunmovable", "Points $points not movable: $reason");
	RegisterNewText("points/locked", "Locked");
	RegisterNewText("points/reminderset", "Reminder set");
	RegisterNewText("track/reserved", "Reserved");
}