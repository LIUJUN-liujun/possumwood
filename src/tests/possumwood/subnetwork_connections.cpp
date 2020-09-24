#include <actions/actions.h>
#include <actions/app.h>
#include <dependency_graph/graph.h>
#include <dependency_graph/metadata_register.h>

#include <boost/test/unit_test.hpp>
#include <dependency_graph/attr.inl>
#include <dependency_graph/metadata.inl>
#include <dependency_graph/nodes_iterator.inl>
#include <dependency_graph/port.inl>
#include <dependency_graph/values.inl>

#include "common.h"

using namespace dependency_graph;

namespace std {
static std::ostream& operator<<(std::ostream& out, const std::type_info& t) {
	out << dependency_graph::unmangledName(t.name());

	return out;
}
}  // namespace std

BOOST_AUTO_TEST_CASE(simple_subnet) {
	// make the app "singleton"
	possumwood::AppCore app;

	auto networkFactoryIterator = MetadataRegister::singleton().find("network");
	BOOST_REQUIRE(networkFactoryIterator != MetadataRegister::singleton().end());

	// create a subnetwork in the graph, via actions
	UniqueId netId;
	BOOST_REQUIRE_NO_THROW(possumwood::actions::createNode(app.graph(), *networkFactoryIterator, "network",
	                                                       possumwood::NodeData(), netId));

	auto netIt = app.graph().nodes().find(netId);
	BOOST_REQUIRE(netIt != app.graph().nodes().end());
	BOOST_REQUIRE(netIt->is<dependency_graph::Network>());

	dependency_graph::Network& network = netIt->as<dependency_graph::Network>();

	// initial state check
	BOOST_CHECK(network.nodes().empty());
	BOOST_CHECK(network.connections().empty());
	BOOST_CHECK_EQUAL(network.metadata()->attributeCount(), 0u);

	auto inputFactoryIterator = MetadataRegister::singleton().find("input");
	BOOST_REQUIRE(inputFactoryIterator != MetadataRegister::singleton().end());

	// create a tiny subnetwork
	UniqueId inId;
	BOOST_REQUIRE_NO_THROW(
	    possumwood::actions::createNode(network, *inputFactoryIterator, "network_input", possumwood::NodeData(), inId));

	auto inIt = network.nodes().find(inId);
	BOOST_REQUIRE(inIt != network.nodes().end());
	dependency_graph::NodeBase& input = *inIt;

	auto outputFactoryIterator = MetadataRegister::singleton().find("output");
	BOOST_REQUIRE(outputFactoryIterator != MetadataRegister::singleton().end());

	UniqueId outId;
	BOOST_REQUIRE_NO_THROW(possumwood::actions::createNode(network, *outputFactoryIterator, "network_output",
	                                                       possumwood::NodeData(), outId));

	auto outIt = network.nodes().find(outId);
	BOOST_REQUIRE(outIt != network.nodes().end());
	dependency_graph::NodeBase& output = *outIt;

	UniqueId midId;
	BOOST_REQUIRE_NO_THROW(
	    possumwood::actions::createNode(network, passThroughNode(), "passthru", possumwood::NodeData(), midId));

	auto midIt = network.nodes().find(midId);
	BOOST_REQUIRE(midIt != network.nodes().end());
	dependency_graph::NodeBase& middle = *midIt;

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 4u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// before connecting the input and output, the network should have no ports
	BOOST_CHECK_EQUAL(network.portCount(), 0u);

	/////

	// connect the input
	BOOST_REQUIRE_NO_THROW(possumwood::actions::connect(input.port(0), middle.port(0)));

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 5u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// this should mean that the network has an increased portcount
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 1u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 1u);

	// the new port should be of "float" type
	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	// undo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().undo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 4u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 1u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 0u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 0u);

	// redo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().redo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 5u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 1u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 1u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	/////

	// connect the output
	BOOST_REQUIRE_NO_THROW(possumwood::actions::connect(middle.port(1), output.port(0)));

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 6u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// this should mean that the network has an increased portcount
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 2u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 2u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(1).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(1).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(1).category(), Attr::kOutput);

	// undo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().undo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 5u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 1u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 1u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 1u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	// redo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().redo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 6u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 2u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 2u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(1).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(1).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(1).category(), Attr::kOutput);

	//////

	// disconnect the output
	BOOST_REQUIRE_NO_THROW(possumwood::actions::disconnect(middle.port(1), output.port(0)));

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 7u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// this should mean that the network has an decreased portcount
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 1u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 1u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	// undo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().undo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 6u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 1u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 2u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 2u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(1).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(1).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(1).category(), Attr::kOutput);

	// redo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().redo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 7u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 1u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 1u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	//////

	// disconnect the input
	BOOST_REQUIRE_NO_THROW(possumwood::actions::disconnect(input.port(0), middle.port(0)));

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 8u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// this should mean that the network has an increased portcount
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 0u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 0u);

	// undo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().undo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 7u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 1u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 1u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 1u);

	BOOST_CHECK_EQUAL(input.network().metadata()->attr(0).type(), typeid(float));
	BOOST_CHECK_EQUAL(dependency_graph::unmangledTypeId(input.network().port(0).type()),
	                  dependency_graph::unmangledTypeId<float>());
	BOOST_CHECK_EQUAL(input.network().port(0).category(), Attr::kInput);

	// redo it
	BOOST_REQUIRE_NO_THROW(app.undoStack().redo());

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 8u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// ports back to the original state
	BOOST_CHECK_EQUAL(input.network().metadata()->attributeCount(), 0u);
	BOOST_CHECK_EQUAL(input.network().portCount(), 0u);
}

BOOST_AUTO_TEST_CASE(input_output_root) {
	// make the app "singleton"
	possumwood::AppCore app;

	auto inputFactoryIterator = MetadataRegister::singleton().find("input");
	BOOST_REQUIRE(inputFactoryIterator != MetadataRegister::singleton().end());

	auto outputFactoryIterator = MetadataRegister::singleton().find("output");
	BOOST_REQUIRE(outputFactoryIterator != MetadataRegister::singleton().end());

	// initial state check
	BOOST_CHECK(app.graph().nodes().empty());
	BOOST_CHECK(app.graph().connections().empty());
	BOOST_CHECK_EQUAL(app.graph().metadata()->attributeCount(), 0u);

	// create a tiny subnetwork
	UniqueId inId;
	BOOST_REQUIRE_NO_THROW(possumwood::actions::createNode(app.graph(), *inputFactoryIterator, "network_input",
	                                                       possumwood::NodeData(), inId));

	auto inIt = app.graph().nodes().find(inId);
	BOOST_REQUIRE(inIt != app.graph().nodes().end());
	dependency_graph::NodeBase& input = *inIt;

	UniqueId outId;
	BOOST_REQUIRE_NO_THROW(possumwood::actions::createNode(app.graph(), *outputFactoryIterator, "network_output",
	                                                       possumwood::NodeData(), outId));

	auto outIt = app.graph().nodes().find(outId);
	BOOST_REQUIRE(outIt != app.graph().nodes().end());
	dependency_graph::NodeBase& output = *outIt;

	UniqueId midId;
	BOOST_REQUIRE_NO_THROW(
	    possumwood::actions::createNode(app.graph(), passThroughNode(), "passthru", possumwood::NodeData(), midId));

	auto midIt = app.graph().nodes().find(midId);
	BOOST_REQUIRE(midIt != app.graph().nodes().end());
	dependency_graph::NodeBase& middle = *midIt;

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 3u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// before connecting the input and output, the network should have no ports
	BOOST_CHECK_EQUAL(app.graph().portCount(), 0u);

	// connect
	BOOST_REQUIRE_NO_THROW(possumwood::actions::connect(input.port(0), middle.port(0)));
	BOOST_REQUIRE_NO_THROW(possumwood::actions::connect(middle.port(1), output.port(0)));

	// check the state of the undo stack
	BOOST_CHECK_EQUAL(app.undoStack().undoActionCount(), 5u);
	BOOST_CHECK_EQUAL(app.undoStack().redoActionCount(), 0u);

	// after connecting, the graph should have an input and an output
	BOOST_CHECK_EQUAL(app.graph().portCount(), 2u);
}
