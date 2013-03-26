//  grass - Generic Rail And Signalling Simulator
//
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

#ifndef INC_UTIL_ALREADY
#define INC_UTIL_ALREADY

#include <ctime>
#include <string>

//adapted from http://forums.devshed.com/showpost.php?p=2678621&postcount=9
//Newton-Raphson
template <typename I> I fast_isqrt( I n ){

	I p = 1;
	while (p <= n/p) p <<= 1;

	I r = p; // initial guess: isqrt(n) < r

	do {
		p = r;
		r = (r + n/r) / 2;
	} while(r < p);

	return p;
}

std::string string_format(const std::string &fmt, ...);
std::string gr_strftime(const std::string &format, const struct tm *tm, time_t timestamp, bool localtime);
unsigned int GetMilliTime();

#endif
