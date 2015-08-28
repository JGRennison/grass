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

#ifndef INC_EDGE_TYPE_ALREADY
#define INC_EDGE_TYPE_ALREADY

#include <ostream>

enum class EDGE {
	INVALID = 0,
	FRONT,         //front edge/forward direction on track
	BACK,          //back edge/reverse direction on track

	PTS_FACE,      //points: facing direction
	PTS_NORMAL,    //points: normal trailing direction
	PTS_REVERSE,   //points: reverse trailing direction

	X_LEFT,        //crossover: Left edge (seen from front)
	X_RIGHT,       //crossover: Right edge (seen from front)

	DS_FL,         //double-slip: front edge, left track
	DS_FR,         //double-slip: front edge, right track
	DS_BL,         //double-slip: back edge, left track (as seen from front)
	DS_BR,         //double-slip: back edge, right track (as seen from front)
};

std::ostream& operator<<(std::ostream& os, EDGE obj);
const char * SerialiseDirectionName(EDGE obj);
bool DeserialiseDirectionName(EDGE& obj, const char *dirname);

#endif
