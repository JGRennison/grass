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

#include "catch.hpp"
#include "track_ops.h"
#include "world-test.h"
#include "track.h"
#include "textpool.h"
#include "var.h"

struct test_fixture_world_ops_1 {
	world_test w;
	points p1;
	test_fixture_world_ops_1() : p1(w) {
		p1.SetName("P1");
	}
};

TEST_CASE( "track/ops/points/movement", "Test basic points movement future" ) {
	test_fixture_world_ops_1 env;

	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, genericpoints::PTF_REV, genericpoints::PTF_REV));

	REQUIRE(env.p1.GetPointFlags(0) == 0);
	env.w.GameStep(500);
	REQUIRE(env.p1.GetPointFlags(0) == (genericpoints::PTF_REV | genericpoints::PTF_OOC));
	env.w.GameStep(4500);
	REQUIRE(env.p1.GetPointFlags(0) == genericpoints::PTF_REV);

	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, genericpoints::PTF_LOCKED, genericpoints::PTF_LOCKED));
	env.w.GameStep(50);
	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, genericpoints::PTF_REV, genericpoints::PTF_REV));
	env.w.GameStep(50);
	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, 0, genericpoints::PTF_REV));
	env.w.GameStep(4900);
	REQUIRE(env.p1.GetPointFlags(0) == (genericpoints::PTF_REV | genericpoints::PTF_LOCKED));
	REQUIRE(env.w.GetLogText() == "Points P1 not movable: Locked\n");
}
