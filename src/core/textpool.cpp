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

#include "common.h"
#include "core/text_pool.h"

text_pool::text_pool() {
	RegisterNewText("text_pool/keynotfound", "text_pool: No such key: ");
}

void text_pool::RegisterNewText(const std::string &key, const std::string &text) {
	textmap[key] = text;
}

std::string text_pool::GetTextByName(const std::string &key) const {
	auto text = textmap.find(key);
	if (text != textmap.end()) {
		return text->second;
	} else {
		return textmap.at("text_pool/keynotfound") + key;
	}
}

void text_pool::Deserialise(const deserialiser_input &di, error_collection &ec) {
	//code goes here
}


default_user_message_pool::default_user_message_pool() {
	RegisterNewText("track_ops/pointsunmovable", "Points $target not movable: $reason");
	RegisterNewText("points/locked", "Locked");
	RegisterNewText("points/reminderset", "Reminder set");
	RegisterNewText("track/reserved", "Reserved");
	RegisterNewText("track/notreserved", "Not reserved");
	RegisterNewText("track/reservation/fail", "Cannot reserve route: $reason");
	RegisterNewText("track/reservation/alreadyset", "Route already set from this signal");
	RegisterNewText("track/reservation/overlap/noneavailable", "No overlap available");
	RegisterNewText("track/reservation/conflict", "Conflicts with existing route");
	RegisterNewText("track/reservation/notsignal", "Not a signal");
	RegisterNewText("track/reservation/notroutingpoint", "Not a routing point");
	RegisterNewText("track/reservation/noroute", "No route between selected signals");
	RegisterNewText("track/reservation/routesetfromexitsignal", "Route set from exit signal");
	RegisterNewText("track/reservation/routesettothissignal", "Route set to entry signal");
	RegisterNewText("track/unreservation/fail", "Cannot unreserve route: $reason");
	RegisterNewText("track/unreservation/autosignal", "Automatic signal");
	RegisterNewText("track/reservation/overlapcantswing/trainapproaching", "Can't swing overlap: train on approach");
	RegisterNewText("track/reservation/overlapcantswing/notpermitted", "Can't swing overlap: not permitted");
	RegisterNewText("track/notpoints", "Not points");
	RegisterNewText("generic/failurereason", "Failed");
	RegisterNewText("generic/invalidcommand", "Invalid command: '$input'");
	RegisterNewText("generic/cannotuse", "Cannot use '$item': $reason");
}
