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

guilayout::layoutoffsetdirectionresult guilayout::LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length) {
	if(ld == LAYOUT_DIR::NULLDIR || !length) {
		return layoutoffsetdirectionresult { startx, starty, startx, starty, ld };
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

	auto common = [](layout_descriptor &desc, const deserialiser_input &di, error_collection &ec) {
		if(CheckTransJsonValue(desc.x, di, "x", ec)) desc.setmembers |= layout_descriptor::LDSM_X;
		if(CheckTransJsonValue(desc.y, di, "y", ec)) desc.setmembers |= layout_descriptor::LDSM_Y;
		if(CheckTransJsonValue(desc.rx, di, "rx", ec)) desc.setmembers |= layout_descriptor::LDSM_RX;
		if(CheckTransJsonValue(desc.ry, di, "ry", ec)) desc.setmembers |= layout_descriptor::LDSM_RY;
		if(CheckTransJsonValue(desc.leftside, di, "leftside", ec)) desc.setmembers |= layout_descriptor::LDSM_LEFTSIDE;
		if(CheckTransJsonValue(desc.rightside, di, "rightside", ec)) desc.setmembers |= layout_descriptor::LDSM_RIGHTSIDE;
		if(CheckTransJsonValue(desc.prev, di, "prev", ec)) desc.setmembers |= layout_descriptor::LDSM_PREV;
		if(CheckTransJsonValue(desc.prev_edge, di, "prevedge", ec)) desc.setmembers |= layout_descriptor::LDSM_PREVEDGE;
		if(CheckTransJsonValue(desc.graphics, di, "graphics", ec)) desc.setmembers |= layout_descriptor::LDSM_GRAPHICS;
		CheckTransJsonValue(desc.pos_class, di, "posclass", ec);
	};

	std::weak_ptr<world_layout> wl_weak = shared_from_this();
	ws.gui_layout_generictrack = [wl_weak, common](const generictrack *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;

		std::unique_ptr<layout_descriptor> desc(new layout_descriptor);
		desc->type = "track";
		if(CheckTransJsonValue(desc->layoutdirection, di, "direction", ec)) desc->setmembers |= layout_descriptor::LDSM_LAYOUTDIR;
		if(CheckTransJsonValue(desc->length, di, "length", ec)) desc->setmembers |= layout_descriptor::LDSM_LENGTH;
		common(*desc, di, ec);

		wl->AddLayoutDescriptor(std::move(desc));
	};
	ws.gui_layout_trackberth = [wl_weak, common](const trackberth *b, const generictrack *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;

		std::unique_ptr<layout_descriptor> desc(new layout_descriptor);
		desc->type = "berth";
		if(CheckTransJsonValue(desc->length, di, "length", ec)) desc->setmembers |= layout_descriptor::LDSM_LENGTH;
		common(*desc, di, ec);

		wl->AddLayoutDescriptor(std::move(desc));
	};
	ws.gui_layout_guiobject = [wl_weak, common](const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;

		std::unique_ptr<layout_descriptor> desc(new layout_descriptor);
		desc->type = "guiobj";
		if(CheckTransJsonValue(desc->dx, di, "dx", ec)) desc->setmembers |= layout_descriptor::LDSM_DX;
		if(CheckTransJsonValue(desc->dy, di, "dy", ec)) desc->setmembers |= layout_descriptor::LDSM_DY;
		common(*desc, di, ec);

		wl->AddLayoutDescriptor(std::move(desc));
	};

}

void guilayout::world_layout::AddLayoutObj(const std::shared_ptr<layout_obj> &obj) {
	objs.push_back(obj);
}

void guilayout::world_layout::AddLayoutDescriptor(std::unique_ptr<layout_descriptor> &&desc) {
	descriptors.emplace_back(std::move(desc));
}

void guilayout::world_layout::ProcessLayoutDescriptor(const layout_descriptor &desc, error_collection &ec) {
	std::shared_ptr<generictrack_obj> obj = std::make_shared<generictrack_obj>();
	AddLayoutObj(std::static_pointer_cast<layout_obj>(obj));

	layout_pos &lp = layout_positions[desc.pos_class];


	generictrack *gt = desc.gt;
	if(gt) {
		genericsignal *gs = FastSignalCast(gt);
		if(gs) {

		}
		genericpoints *gp = dynamic_cast<genericpoints*>(gt);
		if(gp) {

		}
	}
}

void guilayout::world_layout::ProcessLayoutDescriptorSet(error_collection &ec) {
	for(auto &it : descriptors) {
		ProcessLayoutDescriptor(*it, ec);
	}
	descriptors.clear();
}
