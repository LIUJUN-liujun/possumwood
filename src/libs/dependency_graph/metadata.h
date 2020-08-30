#pragma once

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "attr.h"
#include "data.h"
#include "state.h"
#include "unique_id.h"

namespace dependency_graph {

template <typename T>
class InAttr;

template <typename T>
class OutAttr;

class Values;
class Attr;

class Node;
class NodeBase;
class Port;
class Network;

class MetadataHandle;

namespace detail {
struct MetadataAccess;
}

class Metadata : public boost::noncopyable, public std::enable_shared_from_this<Metadata> {
  public:
	Metadata(const std::string& nodeType);
	virtual ~Metadata();

	/// returns the type of the node this metadata object describes
	const std::string& type() const;

	/// returns true if this metadata instance has no attributes or no compute
	virtual bool isValid() const;

	/// registers an input attribute.
	/// Each attribute instance should be held statically in the
	/// implementation of the "node" concept of the target application.
	/// This call does *not* take ownership of the attribute, and assumes
	/// that it will be available throughout the application run.
	template <typename T>
	void addAttribute(InAttr<T>& in, const std::string& name, T&& defaultValue = T(), unsigned flags = 0);

	void addAttribute(InAttr<void>& in, const std::string& name, unsigned flags = 0);

	/// registers an output attribute.
	/// Each attribute instance should be held statically in the
	/// implementation of the "node" concept of the target application.
	/// This call does *not* take ownership of the attribute, and assumes
	/// that it will be available throughout the application run.
	template <typename T>
	void addAttribute(OutAttr<T>& out, const std::string& name, T&& defaultValue = T(), unsigned flags = 0);

	void addAttribute(OutAttr<void>& in, const std::string& name, unsigned flags = 0);

	/// compute method of this node
	void setCompute(std::function<State(Values&)> compute);

	/// returns the number of attributes currently present
	size_t attributeCount() const;

	/// returns an attribute reference
	const Attr& attr(size_t index) const;

	/// adds attribute influence - inputs required to compute outputs
	template <typename T, typename U>
	void addInfluence(const InAttr<T>& in, const OutAttr<U>& out);

	template <typename T>
	std::vector<std::reference_wrapper<const Attr>> influences(const InAttr<T>& in) const;

	template <typename T>
	std::vector<std::reference_wrapper<const Attr>> influencedBy(const OutAttr<T>& out) const;

	virtual std::unique_ptr<NodeBase> createNode(const std::string& name, Network& parent,
	                                             const UniqueId& id = UniqueId()) const;

  protected:
	unsigned doAddAttribute(const std::string& name, Attr::Category cat, const Data& data, unsigned flags);
	virtual void doAddAttribute(Attr& attr);

  private:
	std::vector<std::size_t> influences(size_t index) const;
	std::vector<std::size_t> influencedBy(size_t index) const;

	std::string m_type;
	std::vector<Attr> m_attrs;
	std::function<State(Values&)> m_compute;

	boost::bimap<boost::bimaps::multiset_of<unsigned>, boost::bimaps::multiset_of<unsigned>> m_influences;

	friend class Node;
	friend class NodeBase;
	friend class Port;

	/// allow actions to access untemplated doAddAttribute
	friend struct detail::MetadataAccess;
};

/// create a new metadata instance - if an instance of MetadataFactory is present in user code, use that
std::unique_ptr<Metadata> instantiateMetadata(const std::string& type);

/// A simple base class, to be statically instantiated in client code to provide a new metadata implementation
struct MetadataFactory : public boost::noncopyable {
	MetadataFactory();
	virtual ~MetadataFactory() = 0;

	virtual std::unique_ptr<Metadata> instantiate(const std::string& type) = 0;
};

/// Just a wrapper over an std::shared_ptr, which might eventually implement
/// a variant of copy-on-write paradigm.
class MetadataHandle {
  public:
	MetadataHandle(std::unique_ptr<Metadata> m);
	MetadataHandle(const Metadata& meta);
	~MetadataHandle();

	const Metadata& metadata() const;
	operator const Metadata&() const;
	const Metadata* operator->() const;

	bool operator==(const MetadataHandle& h) const;
	bool operator!=(const MetadataHandle& h) const;

  private:
	std::shared_ptr<const Metadata> m_meta;
};

}  // namespace dependency_graph
