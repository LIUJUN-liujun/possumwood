#pragma once

#include <actions/traits.h>

#include <boost/range/iterator_range.hpp>
#include <cassert>
#include <limits>
#include <map>
#include <vector>

#include "constraints/channel.h"

namespace anim {

class Animation;

/// TODO: Interface of this class is just a read-only draft, and needs to be finalized.
class Constraints {
  public:
	Constraints() = default;
	Constraints(const anim::Animation& a);

	Constraints(const Constraints& c);
	const Constraints& operator=(const Constraints& c);

	typedef std::map<std::string, constraints::Channel>::const_iterator const_iterator;
	const_iterator begin() const;
	const_iterator end() const;

	/// Adds a new constraint to the set, for a joint named jointName.
	/// Frames are pre-filled with world-space transformations of the selected joint.
	/// If this joint is processed for the first time, all values in constraints are 0.
	/// Otherwise, the initial value is the value of the previous constraint (and can be changed).
	/// The constraint value is assumed to be normalized - threshold is hardwired to be 1.0f.
	void process(const std::string& jointName, std::function<void(constraints::Frames& fr)> fn);

	bool empty() const;
	std::size_t size() const;

	bool operator==(const Constraints& c) const;
	bool operator!=(const Constraints& c) const;

	/// returns a reference to the source animation
	const anim::Animation& anim() const;

  private:
	// source animation - shared ptr, because it doesn't need to be copied or changed.
	// Should not be exposed in the public interface - implementation might change.
	std::shared_ptr<const anim::Animation> m_anim;

	// joint -> channel (set of non-overlapping constraints)
	std::map<std::string, constraints::Channel> m_channels;
};

std::ostream& operator<<(std::ostream& out, const Constraints& c);

}  // namespace anim

namespace possumwood {

template <>
struct Traits<anim::Constraints> {
	static constexpr std::array<float, 3> colour() {
		return std::array<float, 3>{{1.0, 1.0, 0}};
	}
};

}  // namespace possumwood
