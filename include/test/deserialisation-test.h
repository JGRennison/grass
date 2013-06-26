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

#ifndef INC_DESERIALISATION_TEST_ALREADY
#define INC_DESERIALISATION_TEST_ALREADY

#include "catch.hpp"
#include "error.h"
#include "world.h"
#include "world-test.h"
#include "world_serialisation.h"

struct test_fixture_world {
	world_test w;
	world_serialisation ws;
	error_collection ec;

	test_fixture_world(std::string input) : ws(w) {
		ws.ParseInputString(input, ec);
	}
};

struct test_fixture_world_init_checked : public test_fixture_world {
	test_fixture_world_init_checked(std::string input, bool pli = true, bool gs = false, bool li = true) : test_fixture_world(input) {
		if(li) w.LayoutInit(ec);
		if(pli) w.PostLayoutInit(ec);
		if(gs) ws.DeserialiseGameState(ec);

		if(ec.GetErrorCount()) {
			FAIL("Error Collection: " << ec);
		}
	}
};

#endif
