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

#ifndef INC_FLAGS_ALREADY
#define INC_FLAGS_ALREADY

#include <type_traits>
#include <ostream>
#include <ios>

template< typename enum_type >
struct enum_traits {
	static constexpr bool flags = false;
};

template <typename C> class flagwrapper {
	C flags;

	public:
	flagwrapper(C f) : flags(f) { }
	operator bool() { return flags != static_cast<C>(0); }
	operator C&() { return flags; }
	operator C() const { return flags; }
	flagwrapper &operator&=(C r) { flags &= r; return *this; }
	flagwrapper &operator^=(C r) { flags ^= r; return *this; }
	flagwrapper &operator|=(C r) { flags |= r; return *this; }
	flagwrapper &operator&=(const flagwrapper<C> &r) { flags &= r; return *this; }
	flagwrapper &operator^=(const flagwrapper<C> &r) { flags ^= r; return *this; }
	flagwrapper &operator|=(const flagwrapper<C> &r) { flags |= r; return *this; }
	flagwrapper &operator=(C r) { flags = r; return *this; }
	bool operator!=(C r) { return flags != r; }
	bool operator==(C r) { return flags == r; }
	C &get() { return flags; }
	C get() const { return flags; }
};
template <typename C> flagwrapper<C> operator|(const flagwrapper<C> &l, C r) { return flagwrapper<C>(l.get() | r); }
template <typename C> flagwrapper<C> operator&(const flagwrapper<C> &l, C r) { return flagwrapper<C>(l.get() & r); }
template <typename C> flagwrapper<C> operator^(const flagwrapper<C> &l, C r) { return flagwrapper<C>(l.get() ^ r); }
template <typename C> flagwrapper<C> operator|(const flagwrapper<C> &l, const flagwrapper<C> &r) { return flagwrapper<C>(l.get() | r.get()); }
template <typename C> flagwrapper<C> operator&(const flagwrapper<C> &l, const flagwrapper<C> &r) { return flagwrapper<C>(l.get() & r.get()); }
template <typename C> flagwrapper<C> operator^(const flagwrapper<C> &l, const flagwrapper<C> &r) { return flagwrapper<C>(l.get() ^ r.get()); }
template <typename C> flagwrapper<C> operator~(const flagwrapper<C> &l) { return flagwrapper<C>(~(l.get())); }

//enum_traits<C>::flags
//std::is_enum<C>::value
template <typename C> typename std::enable_if<enum_traits<C>::flags, flagwrapper<C>>::type operator|(C l, C r) { return flagwrapper<C>(static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) | static_cast<typename std::underlying_type<C>::type >(r))); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, flagwrapper<C>>::type operator&(C l, C r) { return flagwrapper<C>(static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) & static_cast<typename std::underlying_type<C>::type >(r))); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, flagwrapper<C>>::type operator^(C l, C r) { return flagwrapper<C>(static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) ^ static_cast<typename std::underlying_type<C>::type >(r))); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, flagwrapper<C>>::type operator|(C l, const flagwrapper<C> &r) { return flagwrapper<C>(static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) | static_cast<typename std::underlying_type<C>::type >(r.get()))); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, flagwrapper<C>>::type operator&(C l, const flagwrapper<C> &r) { return flagwrapper<C>(static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) & static_cast<typename std::underlying_type<C>::type >(r.get()))); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, flagwrapper<C>>::type operator^(C l, const flagwrapper<C> &r) { return flagwrapper<C>(static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) ^ static_cast<typename std::underlying_type<C>::type >(r.get()))); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, C&>::type operator|=(C &l, C r) { l = static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) | static_cast<typename std::underlying_type<C>::type >(r)); return l; }
template <typename C> typename std::enable_if<enum_traits<C>::flags, C&>::type operator&=(C &l, C r) { l = static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) & static_cast<typename std::underlying_type<C>::type >(r)); return l; }
template <typename C> typename std::enable_if<enum_traits<C>::flags, C&>::type operator^=(C &l, C r) { l = static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) ^ static_cast<typename std::underlying_type<C>::type >(r)); return l; }
template <typename C> typename std::enable_if<enum_traits<C>::flags, C&>::type operator|=(C &l, const flagwrapper<C> &r) { l = static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) | static_cast<typename std::underlying_type<C>::type >(r.get())); return l; }
template <typename C> typename std::enable_if<enum_traits<C>::flags, C&>::type operator&=(C &l, const flagwrapper<C> &r) { l = static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) & static_cast<typename std::underlying_type<C>::type >(r.get())); return l; }
template <typename C> typename std::enable_if<enum_traits<C>::flags, C&>::type operator^=(C &l, const flagwrapper<C> &r) { l = static_cast<C>(static_cast<typename std::underlying_type<C>::type >(l) ^ static_cast<typename std::underlying_type<C>::type >(r.get())); return l; }
template <typename C> typename std::enable_if<enum_traits<C>::flags, flagwrapper<C>>::type operator~(C l) { return flagwrapper<C>(static_cast<C>(~static_cast<typename std::underlying_type<C>::type >(l))); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, bool>::type operator!(C l) { return !static_cast<typename std::underlying_type<C>::type >(l); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, bool>::type operator||(C l, C r) { return static_cast<typename std::underlying_type<C>::type >(l) || static_cast<typename std::underlying_type<C>::type >(r); }
template <typename C> typename std::enable_if<enum_traits<C>::flags, bool>::type operator&&(C l, C r) { return static_cast<typename std::underlying_type<C>::type >(l) && static_cast<typename std::underlying_type<C>::type >(r); }

template <typename C> typename std::enable_if<enum_traits<C>::flags, std::ostream&>::type
operator<<(std::ostream& os, C val) {
	auto prev = os.flags();
	os << "0x" << std::hex << std::uppercase << static_cast<typename std::underlying_type<C>::type>(val);
	os.flags(prev);
	return os;
}

template <typename C> std::ostream&
operator<<(std::ostream& os, flagwrapper<C> val) {
	os << val.get();
	return os;
}

#endif

