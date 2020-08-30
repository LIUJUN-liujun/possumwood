#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#include "index.h"
#include "n_links.h"
#include "t_links.h"

namespace lightfields {

/// An abstraction of a grid graph - interconnected layers, with horizontal N links and vertical T links
class Graph {
  public:
	Graph(const V2i& size, int n_link_value, std::size_t layer_count);

	// sets the value of all T links in a "stack"
	void setValue(const V2i& pos, const std::vector<int>& values, float power = 1.0f);
	// sets the values of all N links in a "layer"
	void setLayer(const cv::Mat& vals, const std::size_t& index, float power = 1.0f);

	std::size_t layerCount() const;
	V2i size() const;

	Link::Direction& edge(const Index& source, const Index& target);
	const Link::Direction& edge(const Index& source, const Index& target) const;

	cv::Mat minCut() const;
	std::vector<cv::Mat> debug() const;

  private:
	std::size_t v2i(const V2i& v) const;

	cv::Mat t_flow(const TLinks& t) const;
	cv::Mat n_flow(const NLinks& t) const;

	V2i m_size;
	int m_nLinkValue;
	std::vector<TLinks> m_tLinks;
	std::vector<NLinks> m_nLinks;
};

}  // namespace lightfields
