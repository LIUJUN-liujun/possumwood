#include <actions/traits.h>
#include <maths/io/vec3.h>
#include <possumwood_sdk/datatypes/filename.h>
#include <possumwood_sdk/node_implementation.h>

#include <opencv2/opencv.hpp>

#include "calibration_pattern.h"
#include "frame.h"
#include "tools.h"

namespace {

dependency_graph::InAttr<possumwood::opencv::Frame> a_inFrame;
dependency_graph::InAttr<possumwood::opencv::CalibrationPattern> a_calibrationPattern;
dependency_graph::OutAttr<possumwood::opencv::Frame> a_outFrame;

dependency_graph::State compute(dependency_graph::Values& data) {
	cv::Mat frame = (*data.get(a_inFrame)).clone();

	const possumwood::opencv::CalibrationPattern& grid = data.get(a_calibrationPattern);

	cv::drawChessboardCorners(frame, grid.size(), *grid, grid.wasFound());

	data.set(a_outFrame, possumwood::opencv::Frame(frame));

	return dependency_graph::State();
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_inFrame, "in_frame", possumwood::opencv::Frame(), possumwood::AttrFlags::kVertical);
	meta.addAttribute(a_calibrationPattern, "pattern", possumwood::opencv::CalibrationPattern(),
	                  possumwood::AttrFlags::kVertical);
	meta.addAttribute(a_outFrame, "out_frame", possumwood::opencv::Frame(), possumwood::AttrFlags::kVertical);

	meta.addInfluence(a_inFrame, a_outFrame);
	meta.addInfluence(a_calibrationPattern, a_outFrame);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("opencv/calibration/draw_pattern", init);

}  // namespace
