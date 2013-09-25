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

#include "common.h"
#include "layout/layout.h"
#include "core/world_serialisation.h"
#include "core/signal.h"
#include "core/points.h"
#include "core/serialisable_impl.h"

namespace guilayout {

	error_layout::error_layout(const layout_obj &obj, const std::string &errmsg) {
		msg << "GUI Layout error for: '" << obj.GetFriendlyName() << "'\n" << errmsg;
	}

	std::array<std::pair<guilayout::LAYOUT_DIR, std::string>, 13> layout_direction_names { {
		{ guilayout::LAYOUT_DIR::NULLDIR, "null" },
		{ guilayout::LAYOUT_DIR::U, "u" },
		{ guilayout::LAYOUT_DIR::UR, "ur" },
		{ guilayout::LAYOUT_DIR::RU, "ru" },
		{ guilayout::LAYOUT_DIR::R, "r" },
		{ guilayout::LAYOUT_DIR::RD, "rd" },
		{ guilayout::LAYOUT_DIR::DR, "dr" },
		{ guilayout::LAYOUT_DIR::D, "d" },
		{ guilayout::LAYOUT_DIR::DL, "dl" },
		{ guilayout::LAYOUT_DIR::LD, "ld" },
		{ guilayout::LAYOUT_DIR::L, "l" },
		{ guilayout::LAYOUT_DIR::LU, "lu" },
		{ guilayout::LAYOUT_DIR::UL, "ul" },
	} };

	static bool DeserialiseLayoutDirectionName(guilayout::LAYOUT_DIR &dir, std::string str) {
		for(auto &it : layout_direction_names) {
			if(it.second == str) {
				dir = it.first;
				return true;
			}
		}
		return false;
	}
};

template<typename C>
struct flagtyperemover<C, typename std::enable_if<std::is_convertible<C, guilayout::LAYOUT_DIR>::value>::type> {
	typedef C type;
};

template <> inline bool IsType<guilayout::LAYOUT_DIR>(const rapidjson::Value& val) {
	if(val.IsString()) {
		guilayout::LAYOUT_DIR dir = guilayout::LAYOUT_DIR::NULLDIR;
		return guilayout::DeserialiseLayoutDirectionName(dir, val.GetString());
	}
	else return false;
}
template <> inline guilayout::LAYOUT_DIR GetType<guilayout::LAYOUT_DIR>(const rapidjson::Value& val) {
	guilayout::LAYOUT_DIR dir = guilayout::LAYOUT_DIR::NULLDIR;
	guilayout::DeserialiseLayoutDirectionName(dir, val.GetString());
	return dir;
}
template <> inline void SetType<guilayout::LAYOUT_DIR>(Handler &out, guilayout::LAYOUT_DIR val) {
	for(auto &it : guilayout::layout_direction_names) {
		if(it.first == val) {
			out.String(it.second);
			return;
		}
	}
	out.String("");
}
template <> inline const char *GetTypeFriendlyName<guilayout::LAYOUT_DIR>() { return "layout direction"; }

void guilayout::layouttrack_obj::RelativeFixup(const layout_obj &src, std::function<void(layouttrack_obj &obj, error_collection &ec)> f, error_collection &ec) {
	if(setmembers & LOSM_X && setmembers & LOSM_Y) f(*this, ec);
	else fixups.push_back(f);
}

std::string guilayout::layouttrack_obj::GetFriendlyName() const {
	return "Track: " + gt->GetFriendlyName();
};

std::string guilayout::layoutberth_obj::GetFriendlyName() const {
	return "Track Berth for: " + gt->GetFriendlyName();
};

std::string guilayout::layoutgui_obj::GetFriendlyName() const {
	return "Layout GUI Object";
};

void guilayout::layout_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	if(CheckTransJsonValue(x, di, "x", ec)) setmembers |= LOSM_X;
	if(CheckTransJsonValue(y, di, "y", ec)) setmembers |= LOSM_Y;
	if(CheckTransJsonValue(rx, di, "rx", ec)) setmembers |= LOSM_RX;
	if(CheckTransJsonValue(ry, di, "ry", ec)) setmembers |= LOSM_RY;
}

void guilayout::layouttrack_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	guilayout::layout_obj::Deserialise(di, ec);
	if(CheckTransJsonValue(length, di, "length", ec)) setmembers |= LTOSM_LENGTH;
	if(CheckTransJsonValue(leftside, di, "leftside", ec)) setmembers |= LTOSM_LEFTSIDE;
	if(CheckTransJsonValue(rightside, di, "rightside", ec)) setmembers |= LTOSM_RIGHTSIDE;
	if(CheckTransJsonValue(layoutdirection, di, "direction", ec)) setmembers |= LTOSM_LAYOUTDIR;
	if(CheckTransJsonValue(prev, di, "prev", ec)) setmembers |= LTOSM_PREV;
	if(CheckTransJsonValue(prev_edge, di, "prevedge", ec)) setmembers |= LTOSM_PREVEDGE;
	if(CheckTransJsonValue(src_branch, di, "branch", ec)) dest_branch = src_branch;
	else {
		CheckTransJsonValue(src_branch, di, "srcbranch", ec);
		CheckTransJsonValue(dest_branch, di, "destbranch", ec);
	}
}

void guilayout::layoutberth_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	guilayout::layout_obj::Deserialise(di, ec);
	if(CheckTransJsonValue(length, di, "length", ec)) setmembers |= LBOSM_LENGTH;
}

void guilayout::layoutgui_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	guilayout::layout_obj::Deserialise(di, ec);
	if(CheckTransJsonValue(dx, di, "dx", ec)) setmembers |= LGOSM_DX;
	if(CheckTransJsonValue(dy, di, "dy", ec)) setmembers |= LGOSM_DY;
	if(CheckTransJsonValue(graphics, di, "graphics", ec)) setmembers |= LGOSM_GRAPHICS;
}

void guilayout::layouttrack_obj::Process(world_layout &wl, error_collection &ec) {
	std::shared_ptr<layouttrack_obj> prevtrackobj;
	if(setmembers & LTOSM_PREV) {
		generictrack *targgt = wl.GetWorld().FindTrackByName(prev);
		if(!targgt) {
			ec.RegisterNewError<error_layout>(*this, "No such track piece: " + prev);
			return;
		}
		prevtrackobj = wl.GetTrackLayoutObj(*this, targgt, ec);
	}
	else {
		prevtrackobj = wl.GetLayoutBranchRef(src_branch);
	}

	bool x_relative = !(setmembers & LOSM_X);
	bool y_relative = !(setmembers & LOSM_Y);

	//work out which edge prevtrackobj/the given coords should be connected to
	EDGETYPE startedge = prev_edge;
	if(startedge == EDGETYPE::EDGE_NULL) {
		std::vector<generictrack::edgelistitem> edges;
		gt->GetListOfEdges(edges);

		unsigned int free_edges = edges.size();
		for(auto &it : edges) {
			if(it.target->track == gt->GetPreviousTrackPiece()) {	//if we are connected to the previous (in the layout) track piece, use that connection/edge as the reference
				startedge = it.edge;
				break;;
			}
			if(it.target->track == gt->GetNextTrackPiece()) {	//if we are connected to the next (in the layout) track piece,
									//don't use that connection/edge, use the remaining edge as the reference
									//if there are multiple remaining edges, report an error
				free_edges--;
				it.edge = EDGETYPE::EDGE_NULL;
			}
		}

		if(startedge == EDGETYPE::EDGE_NULL) {	//check that there is one free edge in the edgelist and use that
			if(free_edges != 1) {
				ec.RegisterNewError<error_layout>(*this, string_format("Ambiguous relative layout declaration: %d possible edges to choose from, try using the 'prevedge' parameter", free_edges));
				return;
			}
			for(auto &it : edges) {		//find the free edge
				if(it.edge != EDGETYPE::EDGE_NULL) {
					startedge = it.edge;
					break;
				}
			}
		}
	}

	if(!prevtrackobj && (x_relative || y_relative)) {
		ec.RegisterNewError<error_layout>(*this, "Cannot layout track piece relative to non-existent track piece");
		return;
	}

	const genericsignal *gs = FastSignalCast(gt);
	if(gs) {

	}
	const genericpoints *gp = dynamic_cast<const genericpoints*>(gt);
	if(gp) {

	}

	auto finalise = [this](error_collection &ec) {
		for(auto &it : fixups) it(*this, ec);
		fixups.clear();
	};

	auto layoutfixup = [&wl, prevtrackobj, this, finalise](layouttrack_obj &targobj, error_collection &ec) {
		finalise(ec);
	};
	if(x_relative || y_relative) prevtrackobj->RelativeFixup(*this, layoutfixup, ec);
	else finalise(ec);
}

void guilayout::layoutberth_obj::Process(world_layout &wl, error_collection &ec) {

}

void guilayout::layoutgui_obj::Process(world_layout &wl, error_collection &ec) {

}

guilayout::layoutoffsetdirectionresult guilayout::LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length) {
	if(ld == LAYOUT_DIR::NULLDIR || !length) {
		return layoutoffsetdirectionresult{ startx, starty, startx, starty, ld };
	}

	LAYOUT_DIR step1 = ld;
	LAYOUT_DIR step2 = ld;
	LAYOUT_DIR stepend1 = ld;
	LAYOUT_DIR stepend2 = ld;

	struct layout_dir_mapping {
		LAYOUT_DIR dir;
		LAYOUT_DIR altstep;
		LAYOUT_DIR step1;
		LAYOUT_DIR step2;
	};
	std::array<layout_dir_mapping, 4> layout_table { {
		{ LAYOUT_DIR::UR, LAYOUT_DIR::RU, LAYOUT_DIR::U, LAYOUT_DIR::R },
		{ LAYOUT_DIR::UL, LAYOUT_DIR::LU, LAYOUT_DIR::U, LAYOUT_DIR::L },
		{ LAYOUT_DIR::DR, LAYOUT_DIR::RD, LAYOUT_DIR::D, LAYOUT_DIR::R },
		{ LAYOUT_DIR::DL, LAYOUT_DIR::LD, LAYOUT_DIR::D, LAYOUT_DIR::L },
	} };

	for(auto &it : layout_table) {
		if(it.dir == ld) {
			step1 = it.step1;
			step2 = it.step2;
			stepend1 = it.altstep;
			break;
		}
		else if(it.altstep == ld) {
			step1 = it.step2;
			step2 = it.step1;
			stepend1 = it.dir;
			break;
		}
	}

	int end_x = startx;
	int end_y = starty;
	int next_x = startx;
	int next_y = starty;
	LAYOUT_DIR outld = ld;

	auto stepcarddir = [&](LAYOUT_DIR ldcard) {
		switch(ldcard) {
			case LAYOUT_DIR::U:
				next_y--; break;
			case LAYOUT_DIR::D:
				next_y++; break;
			case LAYOUT_DIR::L:
				next_x--; break;
			case LAYOUT_DIR::R:
				next_x++; break;
			default:
				break;
		}

		if(length == 2) {
			end_x = next_x;
			end_y = next_y;
		}
	};

	while(true) {
		stepcarddir(step1);
		outld = stepend1;
		length--;
		if(!length) break;

		stepcarddir(step2);
		outld = stepend2;
		length--;
		if(!length) break;
	}

	return layoutoffsetdirectionresult { end_x, end_y, next_x, next_y, outld };
}

void guilayout::world_layout::SetWorldSerialisationLayout(world_serialisation &ws) {
	std::weak_ptr<world_layout> wl_weak = shared_from_this();
	ws.gui_layout_generictrack = [wl_weak](const generictrack *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;

		std::shared_ptr<layouttrack_obj> obj = std::make_shared<layouttrack_obj>(gt);
		obj->Deserialise(di, ec);

		wl->AddLayoutObj(std::static_pointer_cast<layout_obj>(obj));
	};
	ws.gui_layout_trackberth = [wl_weak](const trackberth *b, const generictrack *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;

		std::shared_ptr<layoutberth_obj> obj = std::make_shared<layoutberth_obj>(gt, b);
		obj->Deserialise(di, ec);

		wl->AddLayoutObj(std::static_pointer_cast<layout_obj>(obj));
	};
	ws.gui_layout_guiobject = [wl_weak](const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;

		std::shared_ptr<layoutgui_obj> obj = std::make_shared<layoutgui_obj>();
		obj->Deserialise(di, ec);

		wl->AddLayoutObj(std::static_pointer_cast<layout_obj>(obj));
	};

}

void guilayout::world_layout::AddLayoutObj(const std::shared_ptr<layout_obj> &obj) {
	objs.push_back(obj);
}

void guilayout::world_layout::ProcessLayoutObjSet(error_collection &ec) {
	for(auto &it : objs) {
		it->Process(*this, ec);
	}
	tracktolayoutmap.clear();
	layout_branches.clear();
}

std::shared_ptr<guilayout::layouttrack_obj> guilayout::world_layout::GetTrackLayoutObj(const layout_obj &src, const generictrack *targetgt, error_collection &ec) {
	auto it = tracktolayoutmap.find(targetgt);
	if(it == tracktolayoutmap.end()) {
		ec.RegisterNewError<error_layout>(src, "Cannot layout object relative to track piece: not in layout: " + targetgt->GetFriendlyName());
		return std::shared_ptr<layouttrack_obj>();
	}
	return it->second;
}

void guilayout::world_layout::LayoutTrackRelativeFixup(const layout_obj &src, const generictrack *targetgt, std::function<void(layouttrack_obj &obj, error_collection &ec)> f, error_collection &ec) {
	std::shared_ptr<guilayout::layouttrack_obj> targ = GetTrackLayoutObj(src, targetgt, ec);
	if(targ) {
		targ->RelativeFixup(src, f, ec);
	}
}
