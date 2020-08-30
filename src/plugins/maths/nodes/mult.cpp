#include <possumwood_sdk/node_implementation.h>

namespace {

dependency_graph::InAttr<float> input1, input2;
dependency_graph::OutAttr<float> output;

dependency_graph::State compute(dependency_graph::Values& data) {
	const float a = data.get(input1);
	const float b = data.get(input2);

	data.set(output, a * b);

	return dependency_graph::State();
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(input1, "a");
	meta.addAttribute(input2, "b");
	meta.addAttribute(output, "out");

	meta.addInfluence(input1, output);
	meta.addInfluence(input2, output);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("maths/mult", init);

}  // namespace
