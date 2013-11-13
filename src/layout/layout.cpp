//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
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

#include "common.h"
#include "layout/layout.h"
#include "core/world_serialisation.h"
#include "core/signal.h"
#include "core/points.h"
#include "core/serialisable_impl.h"
#include "draw/drawmodule.h"

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
}

void guilayout::layouttrack_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	guilayout::layout_obj::Deserialise(di, ec);
	if(CheckTransJsonValue(length, di, "length", ec)) setmembers |= LTOSM_LENGTH;
	if(CheckTransJsonValue(layoutdirection, di, "direction", ec)) setmembers |= LTOSM_LAYOUTDIR;
	if(CheckTransJsonValue(layoutdirection, di, "tracktype", ec)) setmembers |= LTOSM_TRACKTYPE;
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
	if(!(setmembers & LOSM_X) || !(setmembers & LOSM_Y)) {
		ec.RegisterNewError<error_layout>(*this, "x and/or y coordinate missing: layout track pieces must be positioned absolutely");
		return;
	}

	if(layoutdirection == LAYOUT_DIR::NULLDIR) {
		ec.RegisterNewError<error_layout>(*this, "No layout direction given: direction cannot be otherwise inferred.");
		return;
	}

	wl.AddTrackLayoutObj(gt, std::static_pointer_cast<layouttrack_obj>(shared_from_this()));

	std::shared_ptr<draw::draw_module> dmod = wl.GetDrawEngine();
	if(dmod) drawfunction = std::move(dmod->GetDrawTrack(*this, ec));
}

void guilayout::layoutberth_obj::Process(world_layout &wl, error_collection &ec) {
	if(!(setmembers & LOSM_X) || !(setmembers & LOSM_Y)) {
		ec.RegisterNewError<error_layout>(*this, "x and/or y coordinate missing: layout berths must be positioned absolutely");
		return;
	}

	std::shared_ptr<draw::draw_module> dmod = wl.GetDrawEngine();
	if(dmod) drawfunction = std::move(dmod->GetDrawBerth(*this, ec));
}

void guilayout::layoutgui_obj::Process(world_layout &wl, error_collection &ec) {
	if(!(setmembers & LOSM_X) || !(setmembers & LOSM_Y)) {
		ec.RegisterNewError<error_layout>(*this, "x and/or y coordinate missing: gui layout objects must be positioned absolutely");
		return;
	}
	std::shared_ptr<draw::draw_module> dmod = wl.GetDrawEngine();
	if(dmod) drawfunction = std::move(dmod->GetDrawObj(*this, ec));
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
}

void guilayout::world_layout::GetTrackLayoutObjs(const layout_obj &src, const generictrack *targetgt, error_collection &ec, std::vector<std::shared_ptr<guilayout::layouttrack_obj> > &output) {
	decltype(tracktolayoutmap)::iterator start, end;
	std::tie(start, end) = tracktolayoutmap.equal_range(targetgt);
	for(auto it = start; it != end; ++it) {
		output.emplace_back(it->second);
	}
}
