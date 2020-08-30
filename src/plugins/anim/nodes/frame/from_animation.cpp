#include <GL/glut.h>
#include <possumwood_sdk/app.h>
#include <possumwood_sdk/node_implementation.h>

#include "datatypes/animation.h"
#include "datatypes/skeleton.h"

namespace {

dependency_graph::InAttr<anim::Animation> a_anim;
dependency_graph::InAttr<float> a_time;
dependency_graph::OutAttr<anim::Skeleton> a_frame;

dependency_graph::State compute(dependency_graph::Values& data) {
	dependency_graph::State out;

	const float time = data.get(a_time);
	anim::Animation anim = data.get(a_anim);

	if(!anim.empty()) {
		const int frameId = time * anim.fps();
		if(frameId < 0)
			data.set(a_frame, anim.front());
		else if(frameId >= (int)anim.size())
			data.set(a_frame, anim.back());
		else
			data.set(a_frame, anim.frame(frameId));
	}
	else
		data.set(a_frame, anim::Skeleton());

	return out;
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_anim, "animation", anim::Animation(), possumwood::AttrFlags::kVertical);
	meta.addAttribute(a_time, "time");
	meta.addAttribute(a_frame, "frame", anim::Skeleton(), possumwood::AttrFlags::kVertical);

	meta.addInfluence(a_anim, a_frame);
	meta.addInfluence(a_time, a_frame);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("anim/frame/from_animation", init);

}  // namespace
