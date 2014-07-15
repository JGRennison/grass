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

#ifndef INC_MAIN_MAIN_ALREADY
#define INC_MAIN_MAIN_ALREADY

#include <wx/app.h>
#include <wx/timer.h>
#include <memory>
#include <forward_list>

class error_collection;
class world;
class grassapp;
namespace guilayout {
	class world_layout;
}
namespace draw {
	class wx_draw_engine;
	class draw_options;
	class draw_module;
}
namespace maingui {
	class grviewwin;
	struct grviewwinlist;
}

class gametimer : public wxTimer {
	grassapp *app;

	public:
	gametimer(grassapp *app_) : app(app_) { }
	virtual void Notify() override;
};

class grassapp: public wxApp {
	friend gametimer;

	virtual bool OnInit();
	virtual int OnExit();

	std::shared_ptr<world> w;
	std::shared_ptr<guilayout::world_layout> layout;
	std::shared_ptr<draw::wx_draw_engine> eng;
	std::shared_ptr<draw::draw_options> opts;
	unsigned int timestepms = 50;
	unsigned int speedfactor = 256;
	bool ispaused = false;
	std::unique_ptr<gametimer> timer;
	std::shared_ptr<maingui::grviewwinlist> panelset;

	public:
	grassapp();
	virtual ~grassapp();
	bool cmdlineproc(wxChar ** argv, int argc);
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
