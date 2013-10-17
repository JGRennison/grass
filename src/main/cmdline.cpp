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
#include "main/wxcommon.h"
#include "main/SimpleOpt.h"
#include "main/cmdline.h"
#include <wx/string.h>
#include <wx/log.h>

typedef CSimpleOptTempl<wxChar> CSO;

enum { OPT_TESTMODE };

CSO::SOption g_rgOptions[] =
{
	{ OPT_TESTMODE,  wxT("--test"),   SO_NONE  },

	SO_END_OF_OPTIONS
};

static const wxChar* cmdlineargerrorstr(ESOError err) {
	switch (err) {
	case SO_OPT_INVALID:
		return wxT("Unrecognized option");
	case SO_OPT_MULTIPLE:
		return wxT("Option matched multiple strings");
	case SO_ARG_INVALID:
		return wxT("Option does not accept argument");
	case SO_ARG_INVALID_TYPE:
		return wxT("Invalid argument format");
	case SO_ARG_MISSING:
		return wxT("Required argument is missing");
	case SO_ARG_INVALID_DATA:
		return wxT("Option argument appears to be another option");
	default:
		return wxT("Unknown Error");
	}
}

bool cmdlineproc(wxChar ** argv, int argc) {
	CSO args(argc, argv, g_rgOptions, SO_O_CLUMP|SO_O_EXACT|SO_O_SHORTARG|SO_O_FILEARG|SO_O_CLUMP_ARGD|SO_O_NOSLASH);
	while (args.Next()) {
		if (args.LastError() != SO_SUCCESS) {
			wxLogError(wxT("Command line processing error: %s, arg: %s"), cmdlineargerrorstr(args.LastError()), args.OptionText());
			return false;
		}
		switch(args.OptionId()) {
			/*case OPT_TESTMODE: {
				Catch::Config config;
				std::ostringstream oss;
				config.setStreamBuf( oss.rdbuf() );
				//Catch::Main( argc - args.m_nNextOption, argv + args.m_nNextOption, config );
				Catch::Main(config);
				wxLogError(wxT("Test results:\n%s"), wxString(oss.str().c_str(), wxConvLocal).c_str());
				return false;
			}*/
		}
	}
	return true;
}
