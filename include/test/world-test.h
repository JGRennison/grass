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

#ifndef INC_WORLD_TEST_ALREADY
#define INC_WORLD_TEST_ALREADY

#include <sstream>
#include "core/world.h"

class world_test : public world {
	std::stringstream logtext;
	LOG_CATEGORY last_logcat = LOG_NULL;

	public:
	bool round_trip_actions = false;

	virtual void LogUserMessageLocal(LOG_CATEGORY lc, const std::string &message) override {
		last_logcat = lc;
		logtext << message << "\n";
		//world::LogUserMessageLocal(lc, message);
	}

	std::string GetLogText() const { return logtext.str(); }
	LOG_CATEGORY GetLastLogCategory() const { return last_logcat; }
	void ResetLogText() { logtext.str(""); }

	virtual void SubmitAction(const action &request) override;
};

#endif
