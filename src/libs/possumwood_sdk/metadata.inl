#pragma once

#include "actions/traits.h"
#include "colours.h"
#include "editor.h"
#include "metadata.h"

namespace possumwood {

template <typename DRAWABLE>
void Metadata::setDrawable() {
	m_drawableFactory = [](dependency_graph::Values&& vals) {
		return std::unique_ptr<possumwood::Drawable>(new DRAWABLE(std::move(vals)));
	};
}

template <typename EDITOR>
void Metadata::setEditor() {
	m_editorFactory = []() {
		std::unique_ptr<possumwood::Editor> result(new EDITOR());
		return result;
	};
}

template <typename T>
void Metadata::addAttribute(dependency_graph::InAttr<T>& in, const std::string& name, T defaultValue, AttrFlags flags) {
	Colours::registerColour(typeid(T), Traits<T>::colour());

	dependency_graph::Metadata::addAttribute(in, name, std::move(defaultValue), static_cast<unsigned>(flags));
}

template <typename T>
void Metadata::addAttribute(dependency_graph::OutAttr<T>& out, const std::string& name, T defaultValue,
                            AttrFlags flags) {
	Colours::registerColour(typeid(T), Traits<T>::colour());

	dependency_graph::Metadata::addAttribute(out, name, std::move(defaultValue), static_cast<unsigned>(flags));
}

}  // namespace possumwood
