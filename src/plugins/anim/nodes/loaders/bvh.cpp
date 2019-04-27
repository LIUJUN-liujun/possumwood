#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <possumwood_sdk/node_implementation.h>
#include <possumwood_sdk/datatypes/filename.h>
#include <actions/io.h>

#include <ImathMatrixAlgo.h>

#include "datatypes/animation.h"
#include "tokenizer.h"

namespace {
	class BvhTokenizer : public anim::Tokenizer {
		private:
			anim::Tokenizer::State start;

		public:
			BvhTokenizer(std::istream& in) : anim::Tokenizer(in), start(this) {
				start = [this](char c) {
					// don't read any space-like characters, but each is considered an end of a token
					if(std::isspace(c)) {
						emit();
						reject();
					}

					// { and } are tokens on their own
					else if((c == '{') || (c == '}')) {
						emit();
						accept(c);
						emit();
					}

					// anything else is part of a token
					else
						accept(c);
				};

				// set start as the active state
				start.setActive();

				// read the first keyword into current
				next();
			}

		protected:
		private:
	};

	struct Joint {
		std::string name;
		Imath::V3f offset;
		int parent;
		std::vector<std::string> channels;

		unsigned targetId;
	};

	Imath::V3f readOffset(anim::Tokenizer& tokenizer) {
		// has to be written on separate lines to prevent the compiler from reordering the next() calls
		const float x = boost::lexical_cast<float>(tokenizer.next().value);
		const float y = boost::lexical_cast<float>(tokenizer.next().value);
		const float z = boost::lexical_cast<float>(tokenizer.next().value);

		tokenizer.next();

		return Imath::V3f(x, y, z);
	}

	std::vector<std::string> readChannels(anim::Tokenizer& tokenizer) {
		std::vector<std::string> result;

		unsigned count = boost::lexical_cast<unsigned>(tokenizer.next().value);
		for(unsigned a=0;a<count;++a)
			result.push_back(tokenizer.next().value);

		tokenizer.next();

		return result;
	}

	void readJointData(anim::Tokenizer& tokenizer, std::vector<Joint>& joints, int parent, const std::string& namePrefix = "") {
		const int current = joints.size();

		joints.push_back(Joint());

		joints.back().parent = parent;
		joints.back().name = namePrefix;

		// the name might be multiple words, but all on the same line
		const unsigned lineNo = tokenizer.current().line;
		while(tokenizer.current().line == lineNo && tokenizer.current().value != "{") {
			if(!joints.back().name.empty())
				joints.back().name += ' ';
			joints.back().name += tokenizer.current().value;

			tokenizer.next();
		}

		if(tokenizer.current().value != "{")
			throw std::runtime_error("joint " + joints.back().name + " should start with an opening bracket!");
		tokenizer.next();

		while(1) {
			if(tokenizer.current().value == "OFFSET")
				joints.back().offset = readOffset(tokenizer);
			else if(tokenizer.current().value == "CHANNELS")
				joints.back().channels = readChannels(tokenizer);
			else if(tokenizer.current().value == "JOINT") {
				tokenizer.next();
				readJointData(tokenizer, joints, current);
			}
			else if(tokenizer.current().value == "End") {
				if(tokenizer.next().value != "Site")
					throw std::runtime_error("End without Site found (" + tokenizer.current().value + " instead)");
				tokenizer.next();

				readJointData(tokenizer, joints, current, joints.back().name + " End");
			}
			else if(tokenizer.current().value == "}")
				break;
			else
				throw std::runtime_error("unknown joint tag " + tokenizer.current().value);
		}

		tokenizer.next();
	}

	void readHierarchy(anim::Tokenizer& tokenizer, std::vector<Joint>& joints) {
		if(tokenizer.next().value != "ROOT")
			throw std::runtime_error("HIERARCHY should start with ROOT, not with " + tokenizer.current().value);
		tokenizer.next();

		readJointData(tokenizer, joints, -1);
	}

	anim::Transform makeTransform(float val, const std::string& key) {
		if(key == "Xposition")
			return anim::Transform(Imath::V3f(val, 0, 0));
		else if(key == "Yposition")
			return anim::Transform(Imath::V3f(0, val, 0));
		else if(key == "Zposition")
			return anim::Transform(Imath::V3f(0, 0, val));

		else if(key == "Xrotation") {
			Imath::Quatf q;
			q.setAxisAngle(Imath::V3f(1,0,0), val / 180.0f * M_PI);
			return anim::Transform(q);
		}
		else if(key == "Yrotation") {
			Imath::Quatf q;
			q.setAxisAngle(Imath::V3f(0,1,0), val / 180.0f * M_PI);
			return anim::Transform(q);
		}
		else if(key == "Zrotation") {
			Imath::Quatf q;
			q.setAxisAngle(Imath::V3f(0,0,1), val / 180.0f * M_PI);
			return anim::Transform(q);
		}
		else
			throw std::runtime_error("unknown channel type " + key);
	}

	void readMotion(anim::Tokenizer& tokenizer, anim::Animation& anim, const std::vector<Joint>& joints, const anim::Skeleton& skeleton) {
		assert(skeleton.size() == joints.size());

		// number of frames
		if(tokenizer.next().value != "Frames:")
			throw std::runtime_error("MOTION section should start with Frames: keyword, " + tokenizer.current().value + " found instead.");
		const unsigned frameCount = boost::lexical_cast<unsigned>(tokenizer.next().value);

		// frame time
		if(tokenizer.next().value != "Frame")
			throw std::runtime_error("MOTION section should contain with Frame Time: keyword, " + tokenizer.current().value + " found instead.");
		if(tokenizer.next().value != "Time:")
			throw std::runtime_error("MOTION section should contain with Frame Time: keyword, " + tokenizer.current().value + " found instead.");
		anim.setFps(1.0f / boost::lexical_cast<float>(tokenizer.next().value));

		// read the frame data
		for(unsigned f=0;f<frameCount;++f) {
			// make a new frame
			anim::Skeleton frame = skeleton;

			// reset it to identity
			for(auto& j : frame)
				j.tr() = anim::Transform();

			// and process all joints
			unsigned ji = 0;
			for(auto& joint : joints) {
				anim::Transform tr;

				for(auto& ch : joint.channels)
					tr *= makeTransform(boost::lexical_cast<float>(tokenizer.next().value), ch);

				const unsigned targetId = joints[ji].targetId;
				frame[targetId].tr() = skeleton[targetId].tr() * tr;

				++ji;
			}

			// and add it to the animation
			anim.addFrame(frame);
		}

		tokenizer.next();
	}

	anim::Skeleton convertHierarchy(std::vector<Joint>& joints) {
		anim::Skeleton result;

		if(!joints.empty()) {
			result.addRoot(joints[0].name, joints[0].offset);

			for(unsigned a=1;a<joints.size();++a) {
				// find the parent
				int p = -1;
				for(unsigned x=0;x<result.size();++x)
					if(result[x].name() == joints[joints[a].parent].name)
						p = x;
				assert(p >= 0);

				result.addChild(result[p], joints[a].offset, joints[a].name);
			}

			// and set the target IDs for each joint, assuming unique naming
			for(unsigned a=0;a<joints.size();++a)
				for(unsigned b=0;b<result.size();++b)
					if(joints[a].name == result[b].name())
						joints[a].targetId = b;
		}

		return result;
	}

/////

dependency_graph::InAttr<possumwood::Filename> a_filename;
dependency_graph::OutAttr<anim::Skeleton> a_skel;
dependency_graph::OutAttr<std::shared_ptr<const anim::Animation>> a_anim;

dependency_graph::State compute(dependency_graph::Values& data) {
	dependency_graph::State out;

	const possumwood::Filename filename = data.get(a_filename);

	if(!filename.filename().empty() && boost::filesystem::exists(filename.filename())) {
		std::ifstream in(filename.filename().string());

		std::vector<Joint> joints;
		std::shared_ptr<anim::Animation> result(new anim::Animation());
		anim::Skeleton skeleton;

		BvhTokenizer tokenizer(in);

		while(!tokenizer.eof()) {
			if(tokenizer.current().value == "HIERARCHY") {
				readHierarchy(tokenizer, joints);
				skeleton = convertHierarchy(joints);
			}
			else if(tokenizer.current().value == "MOTION")
				readMotion(tokenizer, *result, joints, skeleton);
			else
				throw std::runtime_error("unknown keyword " + tokenizer.current().value);
		}

		data.set(a_skel, skeleton);
		data.set(a_anim, std::shared_ptr<const anim::Animation>(result));
	}
	else {
		data.set(a_anim, std::shared_ptr<const anim::Animation>());
		out.addError("Cannot load filename " + filename.filename().string());
	}

	return out;
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_filename, "filename", possumwood::Filename({
		"BVH files (*.bvh)",
	}));
	meta.addAttribute(a_skel, "skeleton", anim::Skeleton(), possumwood::Metadata::Flags::kVertical);
	meta.addAttribute(a_anim, "anim", std::shared_ptr<const anim::Animation>(new anim::Animation(24.0f)), possumwood::Metadata::Flags::kVertical);

	meta.addInfluence(a_filename, a_anim);
	meta.addInfluence(a_filename, a_skel);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("anim/loaders/bvh", init);

}
