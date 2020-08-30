#include "index.h"

#include <dependency_graph/node_base.inl>

namespace possumwood {

void Index::add(Item&& item) {
	const dependency_graph::UniqueId id = item.graphNode->index();

	assert(m_data.find(id) == m_data.end());
	auto it = m_data.insert(std::make_pair(id, std::move(item))).first;

	// ui index is optional
	if(it->second.editorNode != nullptr) {
		assert(m_uiIndex.find(it->second.editorNode) == m_uiIndex.end());
		m_uiIndex.insert(std::make_pair(it->second.editorNode, id));
	}

	assert(m_nodeIndex.find(it->second.graphNode) == m_nodeIndex.end());
	m_nodeIndex.insert(std::make_pair(it->second.graphNode, id));
}

void Index::remove(const dependency_graph::UniqueId& id) {
	auto it = m_data.find(id);
	assert(it != m_data.end());

	if(it->second.editorNode != nullptr) {
		auto uiIt = m_uiIndex.find(it->second.editorNode);
		assert(uiIt != m_uiIndex.end());
		m_uiIndex.erase(uiIt);
	}

	auto nodeIt = m_nodeIndex.find(it->second.graphNode);
	assert(nodeIt != m_nodeIndex.end());
	m_nodeIndex.erase(nodeIt);

	m_data.erase(it);
}

Index::Item& Index::operator[](const dependency_graph::UniqueId& id) {
	auto it = m_data.find(id);
	assert(it != m_data.end());
	return it->second;
}

const Index::Item& Index::operator[](const dependency_graph::UniqueId& id) const {
	auto it = m_data.find(id);
	assert(it != m_data.end());
	return it->second;
}

Index::Item& Index::operator[](node_editor::Node* ptr) {
	auto uiIt = m_uiIndex.find(ptr);
	assert(uiIt != m_uiIndex.end());

	auto it = m_data.find(uiIt->second);
	assert(it != m_data.end());
	return it->second;
}

const Index::Item& Index::operator[](const node_editor::Node* ptr) const {
	auto uiIt = m_uiIndex.find(ptr);
	assert(uiIt != m_uiIndex.end());

	auto it = m_data.find(uiIt->second);
	assert(it != m_data.end());
	return it->second;
}

Index::Item& Index::operator[](dependency_graph::NodeBase* id) {
	auto nodeIt = m_nodeIndex.find(id);
	assert(nodeIt != m_nodeIndex.end());

	auto it = m_data.find(nodeIt->second);
	assert(it != m_data.end());
	return it->second;
}

const Index::Item& Index::operator[](const dependency_graph::NodeBase* id) const {
	auto nodeIt = m_nodeIndex.find(id);
	assert(nodeIt != m_nodeIndex.end());

	auto it = m_data.find(nodeIt->second);
	assert(it != m_data.end());
	return it->second;
}

Index::const_iterator Index::begin() const {
	return m_data.begin();
}

Index::const_iterator Index::end() const {
	return m_data.end();
}

Index::const_iterator Index::find(const dependency_graph::UniqueId& id) const {
	return m_data.find(id);
}

Index::iterator Index::begin() {
	return m_data.begin();
}

Index::iterator Index::end() {
	return m_data.end();
}

Index::iterator Index::find(const dependency_graph::UniqueId& id) {
	return m_data.find(id);
}

bool Index::empty() const {
	return m_data.empty();
}

void Index::clear() {
	m_data.clear();
	m_uiIndex.clear();
	m_nodeIndex.clear();
}

}  // namespace possumwood
