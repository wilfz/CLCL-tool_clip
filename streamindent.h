#pragma once

#include <ios>
#include <iomanip>
#include <string>

namespace wilfz {

	long& indent_of(std::ios_base& b) {
		static int const index = std::ios_base::xalloc();
		return b.iword(index);
	}

	class indent_level {
	public:
		explicit constexpr indent_level(long n) : _level{ n } {}
	private:
		long const _level;

		friend std::ostream& operator<<(std::ostream& o, indent_level const& i) {
			indent_of(o) = i._level >= 0 ? i._level : 0;
			return o;
		}
	};

	class change_indent {
	public:
		explicit constexpr change_indent(int n) : _delta{ n } {}
	private:
		int const _delta;

		friend std::ostream& operator<<(std::ostream& o, change_indent const& i) {
			auto& o_indent = indent_of(o);
			if (o_indent + i._delta >= 0)  // ensure indent stays non-negative
				o_indent += i._delta;
			return o;
		}
	};

	long& scale_of(std::ios_base& b) {
		static int const index = std::ios_base::xalloc();
		return b.iword(index);
	}

	class indent_scale {
	public:
		explicit constexpr indent_scale(unsigned short n) : _scale{ n } {}
	private:
		int const _scale;

		friend std::ostream& operator<<(std::ostream& o, indent_scale const& i) {
			scale_of(o) = i._scale;
			return o;
		}
	};


	inline std::ostream& indent(std::ostream& o) {
		o << std::setw(static_cast<int>(indent_of(o)) * 2) << "";
		return o;
	}

	inline std::ostream& inc_indent(std::ostream& o) {
		++indent_of(o);
		return o;
	}

	inline std::ostream& dec_indent(std::ostream& o) {
		auto& o_indent = indent_of(o);
		if (o_indent > 0)  // ensure indent stays non-negative
			--o_indent;
		return o;
	}

}