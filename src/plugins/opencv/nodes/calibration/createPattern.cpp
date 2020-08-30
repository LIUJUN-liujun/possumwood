#include <actions/traits.h>
#include <maths/io/vec3.h>
#include <possumwood_sdk/datatypes/enum.h>
#include <possumwood_sdk/datatypes/filename.h>
#include <possumwood_sdk/node_implementation.h>

#include <opencv2/opencv.hpp>

#include "calibration_pattern.h"
#include "frame.h"

namespace {

dependency_graph::InAttr<possumwood::Enum> a_type;
dependency_graph::InAttr<unsigned> a_sizeWidth, a_sizeHeight;
dependency_graph::InAttr<float> a_squareSize;
dependency_graph::OutAttr<possumwood::opencv::CalibrationPattern> a_pattern;

possumwood::opencv::CalibrationPattern::Type typeToEnum(const std::string& flags) {
	if(flags == "Symmetric circles grid")
		return possumwood::opencv::CalibrationPattern::kSymmetricCirclesGrid;
	else if(flags == "Asymmetric circles grid")
		return possumwood::opencv::CalibrationPattern::kAsymmetricCirclesGrid;
	else if(flags == "Chessboard")
		return possumwood::opencv::CalibrationPattern::kChessboard;

	throw std::runtime_error("Unknown value " + flags);
}

dependency_graph::State compute(dependency_graph::Values& data) {
	const possumwood::opencv::CalibrationPattern::Type type = typeToEnum(data.get(a_type).value());

	cv::Mat pattern(data.get(a_sizeHeight) * data.get(a_sizeWidth), 1, CV_32FC2);

	const float squareSize = data.get(a_squareSize);

	// asymmetric
	if(type == possumwood::opencv::CalibrationPattern::kAsymmetricCirclesGrid) {
		for(unsigned r = 0; r < data.get(a_sizeHeight); ++r)
			for(unsigned c = 0; c < data.get(a_sizeWidth); ++c) {
				float* ptr = pattern.ptr<float>(r * data.get(a_sizeWidth) + c, 0);

				*(ptr) = (2 * c + r % 2) * squareSize;
				*(ptr + 1) = r * squareSize;
			}
	}

	// symmetric
	else {
		for(unsigned r = 0; r < data.get(a_sizeHeight); ++r)
			for(unsigned c = 0; c < data.get(a_sizeWidth); ++c) {
				float* ptr = pattern.ptr<float>(r * data.get(a_sizeWidth) + c, 0);

				*(ptr) = c * squareSize;
				*(ptr + 1) = r * squareSize;
			}
	}

	data.set(a_pattern, possumwood::opencv::CalibrationPattern(
	                        pattern, cv::Size(data.get(a_sizeWidth), data.get(a_sizeHeight)), false, type));

	return dependency_graph::State();
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_type, "type",
	                  possumwood::Enum({"Symmetric circles grid", "Asymmetric circles grid", "Chessboard"}));
	meta.addAttribute(a_sizeWidth, "size/width", 4u);
	meta.addAttribute(a_sizeHeight, "size/height", 11u);
	meta.addAttribute(a_squareSize, "square_size", 30.0f);
	meta.addAttribute(a_pattern, "pattern", possumwood::opencv::CalibrationPattern(), possumwood::AttrFlags::kVertical);

	meta.addInfluence(a_type, a_pattern);
	meta.addInfluence(a_sizeWidth, a_pattern);
	meta.addInfluence(a_sizeHeight, a_pattern);
	meta.addInfluence(a_squareSize, a_pattern);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("opencv/calibration/create_pattern", init);

}  // namespace
