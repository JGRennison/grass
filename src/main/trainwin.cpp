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

#include "common.h"
#include "core/util.h"
#include "core/world.h"
#include "core/train.h"
#include "main/wxcommon.h"
#include "main/wxutil.h"
#include "main/trainwin.h"
#include "main/maingui.h"

maingui::trainwin::trainwin(std::shared_ptr<const world> w_, std::shared_ptr<grviewwinlist> winlist_, std::unique_ptr<trainwin_columnset> columns_)
		: wxFrame(nullptr, wxID_ANY, wxT("Train list")), w(std::move(w_)), winlist(std::move(winlist_)), columns(std::move(columns_)) {
	winlist->trainwins.emplace_back(this);

	grid = new wxGrid(this, wxID_ANY, columns->columns.size(), 5, 5);
	grid->CreateGrid(0, 0);
	grid->EnableGridLines(true);
	grid->EnableEditing(false);

	grid->AppendCols(columns->columns.size());

	int colid = 0;
	for(auto &it : columns->columns) {
		grid->SetColLabelValue(colid++, wxstrstd(it.name));
	}

	Redraw();

	timer = statictimer.lock();
	if(!timer) {
		timer = std::make_shared<twtimer>(winlist);
		statictimer = timer;
	}
}

maingui::trainwin::~trainwin() {
	container_unordered_remove(winlist->trainwins, this);
}

void maingui::trainwin::Redraw() {
	Freeze();
	grid->BeginBatch();
	if(grid->GetNumberRows()) {
		grid->DeleteRows(0, grid->GetNumberRows());
	}

	w->EnumerateTrains([&](const train &t) {
		grid->AppendRows(1);
		int rowid = grid->GetNumberRows() - 1;
		grid->SetRowLabelValue(rowid, wxT(""));
		int colid = 0;
		for(auto &it : columns->columns) {
			grid->SetCellValue(rowid, colid++, wxstrstd(it.func(t)));
		}
	});

	grid->EndBatch();
	Thaw();
}

maingui::trainwin::twtimer::twtimer(std::shared_ptr<grviewwinlist> winlist_)
		: winlist(winlist_) {
	Start(500, wxTIMER_CONTINUOUS);
}

void maingui::trainwin::twtimer::Notify() {
	for(auto &it : winlist->trainwins) {
		it->Redraw();
	}
}

std::weak_ptr<maingui::trainwin::twtimer> maingui::trainwin::statictimer;

std::unique_ptr<maingui::trainwin_columnset> maingui::GetTrainwinColumnSet() {
	std::unique_ptr<maingui::trainwin_columnset> cols(new maingui::trainwin_columnset);

	cols->columns.push_back({ "ID", [&](const train &t) { return t.GetName(); } });
	cols->columns.push_back({ "Speed", [&](const train &t) { return std::to_string(t.GetTrainMotionState().current_speed); } });
	cols->columns.push_back({ "Max Speed", [&](const train &t) { return std::to_string(t.GetTrainDynamics().veh_max_speed); } });
	cols->columns.push_back({ "Length", [&](const train &t) { return std::to_string(t.GetTrainDynamics().total_length); } });
	cols->columns.push_back({ "Tractive Force", [&](const train &t) { return std::to_string(t.GetTrainDynamics().total_tractive_force); } });
	cols->columns.push_back({ "Tractive Power", [&](const train &t) { return std::to_string(t.GetTrainDynamics().total_tractive_power); } });
	cols->columns.push_back({ "Braking Force", [&](const train &t) { return std::to_string(t.GetTrainDynamics().total_braking_force); } });
	cols->columns.push_back({ "Head Position", [&](const train &t) { return stringify(t.GetTrainMotionState().head_pos); } });
	cols->columns.push_back({ "Tail Position", [&](const train &t) { return stringify(t.GetTrainMotionState().tail_pos); } });

	return std::move(cols);
}
