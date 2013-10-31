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

#ifndef INC_LAYOUT_ALREADY
#define INC_LAYOUT_ALREADY

#include <memory>
#include <utility>
#include <deque>
#include <string>
#include <map>
#include <tuple>
#include <vector>
#include <functional>
#include "core/edgetype.h"
#include "core/error.h"
#include "draw/drawtypes.h"

class world_serialisation;
class generictrack;
class trackberth;
class error_collection;
class deserialiser_input;
class world;

namespace guilayout {
	struct layout_obj;
	class world_layout;

	class error_layout : public error_obj {
		public:
		error_layout(const layout_obj &obj, const std::string &msg);
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

	class layout_obj : public std::enable_shared_from_this<layout_obj> {
		protected:
		int x = 0;
		int y = 0;
		int rx = 0;
		int ry = 0;

		enum {
			LOSM_X      = 1<<0,
			LOSM_Y      = 1<<1,
			LOSM_RX     = 1<<2,
			LOSM_RY     = 1<<3,
		};
		unsigned int setmembers = 0;

		public:
		virtual ~layout_obj() { }
		std::pair<unsigned int, unsigned int> GetDimensions() const;
		std::pair<int, int> GetPosition() const;
		virtual std::string GetFriendlyName() const = 0;
		virtual void Process(world_layout &wl, error_collection &ec) = 0;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
		draw::draw_func_type drawfunction;
		int GetX() const { return x; }
		int GetY() const { return y; }
	};

	class layouttrack_obj : public layout_obj {
		protected:
		const generictrack *gt;
		int length = 0;
		bool leftside = false;
		bool rightside = false;
		LAYOUT_DIR layoutdirection = LAYOUT_DIR::NULLDIR;
		std::string src_branch = "default";
		std::string dest_branch = "default";
		std::string connectto;
		EDGETYPE connectto_edge = EDGETYPE::EDGE_NULL;
		EDGETYPE this_edge = EDGETYPE::EDGE_NULL;

		enum {
			LTOSM_LENGTH       = 1<<16,
			LTOSM_LEFTSIDE     = 1<<17,
			LTOSM_RIGHTSIDE    = 1<<18,
			LTOSM_LAYOUTDIR    = 1<<19,
			LTOSM_CONNECT      = 1<<20,
			LTOSM_CONNECTEDGE  = 1<<21,
			LTOSM_THISEDGE     = 1<<21,
		};

		EDGETYPE most_probable_incoming_edge = EDGETYPE::EDGE_NULL;

		struct edge_def {
			EDGETYPE edge;
			int x;                        //these refer to where the next track piece should start from
			int y;                        //"
			LAYOUT_DIR outgoingdirection; //this refers to the direction of the first section of the joining track piece
		};
		std::vector<edge_def> edges;

		std::vector<std::function<void(layouttrack_obj &obj, error_collection &ec)> > fixups;

		public:
		layouttrack_obj(const generictrack *gt_) : gt(gt_) { }
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
		void RelativeFixup(const layout_obj &src, std::function<void(layouttrack_obj &obj, error_collection &ec)> f, error_collection &ec);
		const edge_def *GetEdgeDef(EDGETYPE e) const;
	};

	class layoutberth_obj : public layout_obj {
		protected:
		const generictrack *gt = 0;
		const trackberth *b = 0;
		int length = 0;

		enum {
			LBOSM_LENGTH      = 1<<16,
		};

		public:
		layoutberth_obj(const generictrack *gt_, const trackberth *b_) : gt(gt_), b(b_) { }
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	};

	class layoutgui_obj : public layout_obj {
		protected:
		int dx = 0;
		int dy = 0;
		std::string graphics;

		enum {
			LGOSM_DX          = 1<<16,
			LGOSM_DY          = 1<<17,
			LGOSM_GRAPHICS    = 1<<18,
		};

		public:
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	};

	struct layoutoffsetdirectionresult {
		int endx;
		int endy;
		int nextx;
		int nexty;
		LAYOUT_DIR nextdir;
	};

	layoutoffsetdirectionresult LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length);

	struct layout_branch {
		std::shared_ptr<layouttrack_obj> track_obj;
		EDGETYPE edge;
	};

	class world_layout : public std::enable_shared_from_this<world_layout> {
		std::deque<std::shared_ptr<layout_obj> > objs;
		std::map<const generictrack *, std::shared_ptr<layouttrack_obj> > tracktolayoutmap;
		std::map<std::string, layout_branch> layout_branches;
		const world &w;
		std::shared_ptr<draw::draw_module> eng;

		public:
		world_layout(const world &w_, std::shared_ptr<draw::draw_module> eng_ = std::shared_ptr<draw::draw_module>()) : w(w_), eng(std::move(eng_)) { }
		virtual ~world_layout() { }
		const world & GetWorld() const { return w; }
		layout_branch & GetLayoutBranchRef(std::string name) { return layout_branches[name]; }
		void AddLayoutObj(const std::shared_ptr<layout_obj> &obj);
		void SetWorldSerialisationLayout(world_serialisation &ws);
		void ProcessLayoutObjSet(error_collection &ec);
		std::shared_ptr<layouttrack_obj> GetTrackLayoutObj(const layout_obj &src, const generictrack *targetgt, error_collection &ec);
		void LayoutTrackRelativeFixup(const layout_obj &src, const generictrack *targetgt, std::function<void(layouttrack_obj &obj, error_collection &ec)> f, error_collection &ec);
		inline std::shared_ptr<draw::draw_module> GetDrawEngine() const { return eng; }
	};

};

#endif
