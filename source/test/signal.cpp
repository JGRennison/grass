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
#include "track.h"
#include "signal.h"
#include "traverse.h"
#include "deserialisation-test.h"
#include "world-test.h"

std::string track_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S1", "routeshuntsignal" : true }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true, "connect" : { "fromdirection" : "back", "to" : "DS1", "todirection" : "leftfront" } }, )"
	R"({ "type" : "doubleslip", "name" : "DS1", "notrack_fl_bl" : true }, )"
	R"({ "type" : "startofline", "name" : "C" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S3", "shuntsignal" : true, "connect" : { "fromdirection" : "back", "to" : "DS1", "todirection" : "leftback" } }, )"
	R"({ "type" : "startofline", "name" : "D" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S6", "routeshuntsignal" : true }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true, "routethrough_rev" : false, "connect" : { "fromdirection" : "back", "to" : "DS1", "todirection" : "rightback" } }, )"
	R"({ "type" : "startofline", "name" : "B" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S2", "routeshuntsignal" : true, "routerestrictions" : [ { "targets" : "C", "denyroute" : true } ] }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "points", "name" : "P1", "connect" : { "fromdirection" : "reverse", "to" : "DS1", "todirection" : "rightfront" } }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend_rev" : true, "through" : false }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S4", "reverseautoconnection" : true, "routeshuntsignal" : true, "through_rev" : false, "overlapend" : true}, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "autosignal", "name" : "S5", "reverseautoconnection" : true }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "endofline", "name" : "E" } )"
"] }";

TEST_CASE( "signal/deserialisation/general", "Test basic signal and routing deserialisation" ) {

	test_fixture_world env(track_test_str_1);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	startofline *a = dynamic_cast<startofline *>(env.w.FindTrackByName("A"));
	REQUIRE(a != 0);
	REQUIRE(a->GetAvailableRouteTypes(EDGE_FRONT) == (routingpoint::RPRT_SHUNTEND | routingpoint::RPRT_ROUTEEND));
	REQUIRE(a->GetAvailableRouteTypes(EDGE_BACK) == 0);
	REQUIRE(a->GetSetRouteTypes(EDGE_FRONT) == 0);
	REQUIRE(a->GetSetRouteTypes(EDGE_BACK) == 0);

	routesignal *s1 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S1"));
	REQUIRE(s1 != 0);
	REQUIRE(s1->GetAvailableRouteTypes(EDGE_FRONT) == (routingpoint::RPRT_SHUNTEND | routingpoint::RPRT_ROUTEEND | routingpoint::RPRT_SHUNTSTART | routingpoint::RPRT_ROUTESTART));
	REQUIRE(s1->GetAvailableRouteTypes(EDGE_BACK) == (routingpoint::RPRT_SHUNTTRANS | routingpoint::RPRT_ROUTETRANS));
	REQUIRE(s1->GetSetRouteTypes(EDGE_FRONT) == 0);
	REQUIRE(s1->GetSetRouteTypes(EDGE_BACK) == 0);

	routingmarker *rm = dynamic_cast<routingmarker *>(env.w.FindTrackByName("#4"));
	REQUIRE(rm != 0);
	REQUIRE(rm->GetAvailableRouteTypes(EDGE_FRONT) == (routingpoint::RPRT_MASK_TRANS | routingpoint::RPRT_OVERLAPEND));
	REQUIRE(rm->GetAvailableRouteTypes(EDGE_BACK) == (routingpoint::RPRT_MASK_TRANS));
	REQUIRE(rm->GetSetRouteTypes(EDGE_FRONT) == 0);
	REQUIRE(rm->GetSetRouteTypes(EDGE_BACK) == 0);

	routesignal *s3 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S3"));
	REQUIRE(s3 != 0);
	REQUIRE(s3->GetAvailableRouteTypes(EDGE_FRONT) == (routingpoint::RPRT_SHUNTEND | routingpoint::RPRT_SHUNTSTART));
	REQUIRE(s3->GetAvailableRouteTypes(EDGE_BACK) == (routingpoint::RPRT_SHUNTTRANS | routingpoint::RPRT_ROUTETRANS));
	REQUIRE(s3->GetSetRouteTypes(EDGE_FRONT) == 0);
	REQUIRE(s3->GetSetRouteTypes(EDGE_BACK) == 0);

	routingmarker *rm2 = dynamic_cast<routingmarker *>(env.w.FindTrackByName("#13"));
	REQUIRE(rm2 != 0);
	REQUIRE(rm2->GetAvailableRouteTypes(EDGE_FRONT) == (routingpoint::RPRT_MASK_TRANS | routingpoint::RPRT_OVERLAPEND));
	REQUIRE(rm2->GetAvailableRouteTypes(EDGE_BACK) == (routingpoint::RPRT_MASK_TRANS & ~routingpoint::RPRT_ROUTETRANS));
	REQUIRE(rm2->GetSetRouteTypes(EDGE_FRONT) == 0);
	REQUIRE(rm2->GetSetRouteTypes(EDGE_BACK) == 0);

	routesignal *s2 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S2"));
	REQUIRE(s2 != 0);
	REQUIRE(s2->GetRouteRestrictions().GetRestrictionCount() == 1);
}

TEST_CASE( "signal/routing/general", "Test basic signal and routing connectivity and route creation" ) {
	test_fixture_world env(track_test_str_1);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w.LayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w.PostLayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	routesignal *s1 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S1"));
	REQUIRE(s1 != 0);
	routesignal *s2 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S2"));
	REQUIRE(s2 != 0);
	routesignal *s3 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S3"));
	REQUIRE(s3 != 0);
	routesignal *s4 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S4"));
	REQUIRE(s4 != 0);
	autosignal *s5 = dynamic_cast<autosignal *>(env.w.FindTrackByName("S5"));
	REQUIRE(s5 != 0);
	routesignal *s6 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S6"));
	REQUIRE(s6 != 0);

	startofline *a = dynamic_cast<startofline *>(env.w.FindTrackByName("A"));
	REQUIRE(a != 0);
	startofline *b = dynamic_cast<startofline *>(env.w.FindTrackByName("B"));
	REQUIRE(b != 0);
	startofline *c = dynamic_cast<startofline *>(env.w.FindTrackByName("C"));
	REQUIRE(c != 0);
	startofline *d = dynamic_cast<startofline *>(env.w.FindTrackByName("D"));
	REQUIRE(d != 0);
	startofline *e = dynamic_cast<startofline *>(env.w.FindTrackByName("E"));
	REQUIRE(e != 0);

	auto s5check = [&](unsigned int index, ROUTE_CLASS type) {
		REQUIRE(s5->GetRouteByIndex(index) != 0);
		REQUIRE(s5->GetRouteByIndex(index)->type == type);
		REQUIRE(s5->GetRouteByIndex(index)->end == vartrack_target_ptr<routingpoint>(s4, EDGE_FRONT));
		REQUIRE(s5->GetRouteByIndex(index)->pieces.size() == 1);
	};
	s5check(0, RTC_ROUTE);
	s5check(1, RTC_OVERLAP);

	std::vector<const route *> routeset;
	REQUIRE(s1->GetMatchingRoutes(routeset, a, via_list()) == 0);
	REQUIRE(s1->GetMatchingRoutes(routeset, b, via_list()) == 0);
	REQUIRE(s1->GetMatchingRoutes(routeset, c, via_list()) == 0);
	REQUIRE(s1->GetMatchingRoutes(routeset, d, via_list()) == 1);
	REQUIRE(s1->GetMatchingRoutes(routeset, e, via_list()) == 0);

	REQUIRE(s2->GetMatchingRoutes(routeset, a, via_list()) == 0);
	REQUIRE(s2->GetMatchingRoutes(routeset, b, via_list()) == 0);
	REQUIRE(s2->GetMatchingRoutes(routeset, c, via_list()) == 1);
	REQUIRE(s2->GetMatchingRoutes(routeset, d, via_list()) == 1);
	REQUIRE(s2->GetMatchingRoutes(routeset, e, via_list()) == 0);

	REQUIRE(s3->GetMatchingRoutes(routeset, a, via_list()) == 0);
	REQUIRE(s3->GetMatchingRoutes(routeset, b, via_list()) == 1);
	REQUIRE(s3->GetMatchingRoutes(routeset, c, via_list()) == 0);
	REQUIRE(s3->GetMatchingRoutes(routeset, d, via_list()) == 0);
	REQUIRE(s3->GetMatchingRoutes(routeset, e, via_list()) == 0);

	REQUIRE(s4->GetMatchingRoutes(routeset, a, via_list()) == 0);
	REQUIRE(s4->GetMatchingRoutes(routeset, b, via_list()) == 2);
	REQUIRE(s4->GetMatchingRoutes(routeset, c, via_list()) == 0);
	REQUIRE(s4->GetMatchingRoutes(routeset, d, via_list()) == 0);
	REQUIRE(s4->GetMatchingRoutes(routeset, e, via_list()) == 0);

	REQUIRE(s5->GetMatchingRoutes(routeset, a, via_list()) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, b, via_list()) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, c, via_list()) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, d, via_list()) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, e, via_list()) == 0);

	REQUIRE(s6->GetMatchingRoutes(routeset, a, via_list()) == 2);
	REQUIRE(s6->GetMatchingRoutes(routeset, b, via_list()) == 2);
	REQUIRE(s6->GetMatchingRoutes(routeset, c, via_list()) == 0);
	REQUIRE(s6->GetMatchingRoutes(routeset, d, via_list()) == 0);
	REQUIRE(s6->GetMatchingRoutes(routeset, e, via_list()) == 0);

	auto overlapcheck = [&](genericsignal *s, const std::string &target) {
		routingpoint *rp = dynamic_cast<routingpoint *>(env.w.FindTrackByName(target));
		REQUIRE(s->GetMatchingRoutes(routeset, rp, via_list()) == 1);
		REQUIRE(routeset[0]->end.track == rp);
		REQUIRE(routeset[0]->type == RTC_OVERLAP);

		unsigned int overlapcount = 0;
		auto callback = [&](const route *r) {
			if(r->type == RTC_OVERLAP) overlapcount++;
		};
		s->EnumerateRoutes(callback);
		REQUIRE(overlapcount == 1);
	};
	overlapcheck(s1, "#4");
	overlapcheck(s2, "#18");
	overlapcheck(s4, "#21");
	overlapcheck(s6, "#13");
}