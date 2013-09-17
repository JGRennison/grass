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

#ifndef INC_LAYOUT_ALREADY
#define INC_LAYOUT_ALREADY

#include <memory>
#include <utility>
#include <deque>
#include <string>
#include <map>
#include <tuple>
#include "core/edgetype.h"

class world_serialisation;
class generictrack;
class trackberth;
class error_collection;

namespace guilayout {

	class layout_obj : public std::enable_shared_from_this<layout_obj> {
		std::pair<unsigned int, unsigned int> dimensions;
		std::pair<int, int> position;

		public:
		virtual ~layout_obj() { }
		std::pair<unsigned int, unsigned int> GetDimensions() const;
		std::pair<int, int> GetPosition() const;
	};

	enum class LAYOUT_DIR {
		NULLDIR = 0,
		U,
		UR,
		RU,
		R,
		RD,
		DR,
		D,
		DL,
		LD,
		L,
		LU,
		UL,
	};

	struct layoutoffsetdirectionresult {
		int endx;
		int endy;
		int nextx;
		int nexty;
		LAYOUT_DIR nextdir;
	};

	layoutoffsetdirectionresult LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length);

	struct layout_descriptor {
		std::string type;
		generictrack *gt = 0;
		const trackberth *b = 0;
		int x = 0;
		int y = 0;
		int rx = 0;
		int ry = 0;
		int dx = 0;
		int dy = 0;
		int length = 0;
		bool leftside = false;
		bool rightside = false;
		LAYOUT_DIR layoutdirection = LAYOUT_DIR::NULLDIR;
		std::string prev;
		EDGETYPE prev_edge = EDGETYPE::EDGE_NULL;
		std::string graphics;

		std::string pos_class = "default";

		enum {
			LDSM_X		= 1<<0,
			LDSM_Y		= 1<<1,
			LDSM_RX		= 1<<2,
			LDSM_RY		= 1<<3,
			LDSM_DX		= 1<<4,
			LDSM_DY		= 1<<5,
			LDSM_LENGTH	= 1<<6,
			LDSM_LEFTSIDE	= 1<<7,
			LDSM_RIGHTSIDE	= 1<<8,
			LDSM_LAYOUTDIR	= 1<<9,
			LDSM_PREV	= 1<<10,
			LDSM_PREVEDGE	= 1<<11,
			LDSM_GRAPHICS	= 1<<12,
		};

		unsigned int setmembers = 0;
	};

	class world_layout : public std::enable_shared_from_this<world_layout> {
		std::deque<std::shared_ptr<layout_obj> > objs;
		std::deque<std::unique_ptr<layout_descriptor> > descriptors;

		struct layout_pos {
			int x = 0;
			int y = 0;
			layout_obj *prev_obj = 0;
			generictrack *prev_gt = 0;
			EDGETYPE prev_edge = EDGETYPE::EDGE_NULL;
			LAYOUT_DIR layoutdirection = LAYOUT_DIR::NULLDIR;
		};
		std::map<std::string, layout_pos> layout_positions;

		public:
		void AddLayoutObj(const std::shared_ptr<layout_obj> &obj);
		void AddLayoutDescriptor(std::unique_ptr<layout_descriptor> &&desc);
		void SetWorldSerialisationLayout(world_serialisation &ws);
		void ProcessLayoutDescriptor(const layout_descriptor &desc, error_collection &ec);
		void ProcessLayoutDescriptorSet(error_collection &ec);
		virtual ~world_layout();
	};

	class generictrack_obj : public layout_obj {

	};

};

#endif
