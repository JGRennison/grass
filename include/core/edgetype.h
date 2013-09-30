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

#ifndef INC_EDGETYPE_ALREADY
#define INC_EDGETYPE_ALREADY

#include <ostream>

typedef enum {
	EDGE_NULL = 0,
	EDGE_FRONT,         //front edge/forward direction on track
	EDGE_BACK,          //back edge/reverse direction on track

	EDGE_PTS_FACE,      //points: facing direction
	EDGE_PTS_NORMAL,    //points: normal trailing direction
	EDGE_PTS_REVERSE,   //points: reverse trailing direction

	EDGE_X_LEFT,        //crossover: Left edge (seen from front)
	EDGE_X_RIGHT,       //crossover: Right edge (seen from front)

	EDGE_DS_FL,         //double-slip: front edge, left track
	EDGE_DS_FR,         //double-slip: front edge, right track
	EDGE_DS_BL,         //double-slip: back edge, left track (as seen from front)
	EDGE_DS_BR,         //double-slip: back edge, right track (as seen from front)
} EDGETYPE;

std::ostream& operator<<(std::ostream& os, const EDGETYPE& obj);
const char * SerialiseDirectionName(const EDGETYPE& obj);
bool DeserialiseDirectionName(EDGETYPE& obj, const char *dirname);

#endif