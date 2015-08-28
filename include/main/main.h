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

#ifndef INC_MAIN_MAIN_ALREADY
#define INC_MAIN_MAIN_ALREADY

#include <wx/app.h>
#include <wx/timer.h>
#include <memory>
#include <forward_list>

class error_collection;
class world;
class grass_app;
namespace gui_layout {
	class world_layout;
}
namespace draw {
	class wx_draw_engine;
	class draw_options;
	class draw_module;
}
namespace main_gui {
	class gr_view_win;
	struct gr_view_winlist;
}

class game_timer : public wxTimer {
	grass_app *app;

	public:
	game_timer(grass_app *app_) : app(app_) { }
	virtual void Notify() override;
};

class grass_app: public wxApp {
	friend game_timer;

	virtual bool OnInit();
	virtual int OnExit();

	std::shared_ptr<world> w;
	std::shared_ptr<gui_layout::world_layout> layout;
	std::shared_ptr<draw::wx_draw_engine> eng;
	std::shared_ptr<draw::draw_options> opts;
	unsigned int timestepms = 50;
	unsigned int speedfactor = 256;
	bool ispaused = false;
	std::unique_ptr<game_timer> timer;
	std::shared_ptr<main_gui::gr_view_winlist> panelset;

	public:
	grass_app();
	virtual ~grass_app();
	bool CmdLineProc(wxChar ** argv, int argc);
	bool LoadGame(const wxString &base, const wxString &save);
	std::pair<int, int> GetSpriteSizes() const;
	std::shared_ptr<draw::draw_module> GetCurrentDrawModule();
	std::shared_ptr<draw::draw_options> GetDrawOptions();
	void DisplayErrors(error_collection &ec);
	void MakeNewViewWin();

	protected:
	void RunGameTimer();
	void InitialDrawAll();
};

#endif
