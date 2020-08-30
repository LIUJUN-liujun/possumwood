#pragma once

#include <actions/node_data.h>
#include <dependency_graph/node.h>
#include <dependency_graph/unique_id.h>
#include <possumwood_sdk/drawable.h>
#include <qt_node_editor/node.h>

#include <boost/noncopyable.hpp>

namespace possumwood {

/// an indexing structure, allowing to seamlessly switch between the UI classes,
/// and connected data model classes. Data model uses UniqueId instances for
/// indexing, while UI classes just use simple raw pointers to Qt elements.
class Index {
  public:
	struct Item : public boost::noncopyable {
		Item(dependency_graph::NodeBase* gr, node_editor::Node* en) : graphNode(gr), editorNode(en) {
		}

		Item(Item&& i) : graphNode(i.graphNode), editorNode(i.editorNode) {
		}

		dependency_graph::NodeBase* graphNode;
		node_editor::Node* editorNode;
	};

	void add(Item&& item);

	void remove(const dependency_graph::UniqueId& id);

	Item& operator[](const dependency_graph::UniqueId& id);
	const Item& operator[](const dependency_graph::UniqueId& id) const;

	Item& operator[](node_editor::Node* id);
	const Item& operator[](const node_editor::Node* id) const;

	Item& operator[](dependency_graph::NodeBase* id);
	const Item& operator[](const dependency_graph::NodeBase* id) const;

	typedef std::map<dependency_graph::UniqueId, Item>::iterator iterator;
	iterator begin();
	iterator end();
	iterator find(const dependency_graph::UniqueId& id);

	typedef std::map<dependency_graph::UniqueId, Item>::const_iterator const_iterator;
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator find(const dependency_graph::UniqueId& id) const;

	bool empty() const;
	void clear();

  private:
	/// this should be implemented using multiindex, but the move semantics in multiindex
	/// library seem a bit iffy in current version of boost.
	std::map<dependency_graph::UniqueId, Item> m_data;

	std::map<const node_editor::Node*, dependency_graph::UniqueId> m_uiIndex;
	std::map<const dependency_graph::NodeBase*, dependency_graph::UniqueId> m_nodeIndex;
};

}  // namespace possumwood
