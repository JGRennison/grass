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
//  2014 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#ifndef INC_MAIN_TRAIN_WIN_ALREADY
#define INC_MAIN_TRAIN_WIN_ALREADY

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <wx/grid.h>
#include <wx/frame.h>
#include <wx/timer.h>

class world;
class train;

namespace main_gui {

	class gr_view_winlist;

	struct trainwin_columnset {
		struct trainwin_column {
			std::string name;
			std::function<std::string(const train &t)> func;
		};
		std::vector<trainwin_column> columns;
	};

	class trainwin : public wxFrame {
		std::shared_ptr<const world> w;
		std::shared_ptr<gr_view_winlist> winlist;
		std::unique_ptr<trainwin_columnset> columns;

		wxGrid *grid = nullptr;

		struct twtimer : public wxTimer {
			std::shared_ptr<gr_view_winlist> winlist;

			public:
			twtimer(std::shared_ptr<gr_view_winlist> winlist_);
			virtual void Notify() override;
		};
		static std::weak_ptr<twtimer> statictimer;
		std::shared_ptr<twtimer> timer;

		public:
		trainwin(std::shared_ptr<const world> w_, std::shared_ptr<gr_view_winlist> winlist_, std::unique_ptr<trainwin_columnset> columns_);
		~trainwin();
		void Redraw();
	};

	std::unique_ptr<trainwin_columnset> GetTrainwinColumnSet();

};

#endif
