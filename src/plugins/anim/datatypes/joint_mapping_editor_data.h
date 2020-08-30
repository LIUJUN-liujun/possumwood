#pragma once

#include "skeleton.h"

namespace anim {

/// A simple data container for frame editor node - holds individual transformations for each bone.
class JointMappingEditorData {
  public:
	void setSourceSkeleton(const Skeleton& s);
	const Skeleton& sourceSkeleton() const;

	void setTargetSkeleton(const Skeleton& s);
	const Skeleton& targetSkeleton() const;

	typedef std::vector<std::pair<int, int>>::const_iterator const_iterator;
	typedef std::vector<std::pair<int, int>>::iterator iterator;

	void add(int fromJoint, int toJoint);
	void erase(iterator i);
	void clear();

	std::pair<int, int>& operator[](std::size_t index);
	const std::pair<int, int>& operator[](std::size_t index) const;

	const_iterator begin() const;
	const_iterator end() const;
	const_iterator findSource(int index) const;
	const_iterator findTarget(int index) const;

	iterator begin();
	iterator end();

	bool empty() const;
	std::size_t size() const;

	bool operator==(const JointMappingEditorData& d) const;
	bool operator!=(const JointMappingEditorData& d) const;

  protected:
  private:
	// this should be only a list of bones!
	Skeleton m_sourceSkeleton, m_targetSkeleton;
	std::vector<std::pair<int, int>> m_mapping;
};

std::ostream& operator<<(std::ostream& out, const JointMappingEditorData& d);

}  // namespace anim

namespace possumwood {

template <>
struct Traits<anim::JointMappingEditorData> {
	static IO<anim::JointMappingEditorData> io;

	static constexpr std::array<float, 3> colour() {
		return std::array<float, 3>{{0, 0.2, 0}};
	}
};

}  // namespace possumwood
