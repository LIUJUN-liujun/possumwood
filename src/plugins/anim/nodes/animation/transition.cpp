#include <possumwood_sdk/app.h>
#include <possumwood_sdk/node_implementation.h>

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QLayout>
#include <QMouseEvent>
#include <utility>

#include "datatypes/animation.h"
#include "datatypes/filter.h"
#include "datatypes/skeleton.h"
#include "ui/motion_map.h"

namespace {

dependency_graph::InAttr<anim::Animation> a_animA, a_animB;
dependency_graph::InAttr<unsigned> a_transitionLength, a_trA, a_trB;

dependency_graph::OutAttr<anim::Animation> a_outAnim;

class Editor : public possumwood::Editor {
  public:
	Editor() {
		m_widget = new anim::ui::MotionMap(this);
		layout()->addWidget(m_widget);

		m_lineX = new QGraphicsLineItem();
		m_widget->scene()->addItem(m_lineX);
		m_lineX->setPen(QPen(QColor(255, 128, 0)));

		m_lineY = new QGraphicsLineItem();
		m_widget->scene()->addItem(m_lineY);
		m_lineY->setPen(QPen(QColor(255, 128, 0)));

		m_lineTr = new QGraphicsLineItem();
		m_widget->scene()->addItem(m_lineTr);
		m_lineTr->setPen(QPen(QColor(0, 128, 255)));

		m_timeConnection = possumwood::App::instance().onTimeChanged([this](float t) { timeChanged(t); });

		QObject::connect(m_widget, &anim::ui::MotionMap::mousePress, [this](QMouseEvent* event) {
			if(event->button() == Qt::LeftButton) {
				const QPointF scenePos = m_widget->mapToScene(event->pos());
				if(scenePos.x() >= 0.0f && scenePos.x() < (float)m_widget->width() && scenePos.y() >= 0.0f &&
				   scenePos.y() < (float)m_widget->height()) {
					values().set(a_trA, (unsigned)scenePos.x());
					values().set(a_trB, (unsigned)scenePos.y());
				}
			}
		});

		QObject::connect(m_widget, &anim::ui::MotionMap::mouseMove, [this](QMouseEvent* event) {
			if(event->buttons() & Qt::LeftButton) {
				const QPointF scenePos = m_widget->mapToScene(event->pos());
				if(scenePos.x() >= 0.0f && scenePos.x() < (float)m_widget->width() && scenePos.y() >= 0.0f &&
				   scenePos.y() < (float)m_widget->height()) {
					values().set(a_trA, (unsigned)scenePos.x());
					values().set(a_trB, (unsigned)scenePos.y());
				}
			}
		});
	}

	virtual ~Editor() {
		m_timeConnection.disconnect();
	}

  protected:
	void timeChanged(float t) {
		const unsigned tr_a = values().get(a_trA);
		const unsigned tr_b = values().get(a_trB);
		const unsigned len_a = m_widget->width();
		const unsigned len_b = m_widget->height();

		const float x_pos = t * m_fps;
		if(x_pos > tr_a || x_pos > len_a)
			m_lineX->hide();
		else {
			m_lineX->show();
			m_lineX->setLine(x_pos, 0, x_pos, m_widget->height());
		}

		const float y_pos = t * m_fps - (float)tr_a + (float)tr_b;
		if(y_pos < tr_b || y_pos > len_b)
			m_lineY->hide();
		else {
			m_lineY->show();
			m_lineY->setLine(0, y_pos, m_widget->width(), y_pos);
		}
	}

	virtual void valueChanged(const dependency_graph::Attr& attr) override {
		const anim::Animation& animA = values().get(a_animA);
		const anim::Animation& animB = values().get(a_animB);

		if(attr == a_animA || attr == a_animB) {
			QPixmap pixmap;
			m_fps = 0.0f;

			if(!animA.empty() && !animB.empty()) {
				::anim::MotionMap mmap(animA, animB, ::anim::metric::LocalAngle());

				::anim::filter::LinearTransition filter(values().get(a_transitionLength));
				mmap.filter(filter);

				m_widget->init(mmap);

				m_fps = animA.fps();
			}

			timeChanged(possumwood::App::instance().time());
		}

		if(attr == a_animA || attr == a_animB || attr == a_transitionLength || attr == a_trA || attr == a_trB) {
			const unsigned a = values().get(a_trA);
			const unsigned b = values().get(a_trB);
			const unsigned len = values().get(a_transitionLength);

			m_lineTr->setLine(a >= len / 2 ? a - len / 2 : 0, b >= len / 2 ? b - len / 2 : 0,
			                  a + len / 2 < animA.size() ? a + len / 2 : animA.size(),
			                  b + len / 2 < animB.size() ? b + len / 2 : animB.size());

			timeChanged(possumwood::App::instance().time());
		}
	}

  private:
	anim::ui::MotionMap* m_widget;

	QGraphicsLineItem *m_lineX, *m_lineY, *m_lineTr;

	boost::signals2::connection m_timeConnection;
	float m_fps;
};

dependency_graph::State compute(dependency_graph::Values& values) {
	// the inputs
	auto& anim_a = values.get(a_animA);
	auto& anim_b = values.get(a_animB);

	// if anything goes wrong, just reset the output
	values.set(a_outAnim, anim::Animation());

	if(!anim_a.empty() && !anim_b.empty()) {
		if(not anim_a.frame(0).isCompatibleWith(anim_b.frame(0)))
			throw(std::runtime_error("Animation skeletons don't seem to be compatible."));
		if((anim_a.frame(0).size() == 0) || (anim_b.frame(0).size() == 0))
			throw(std::runtime_error("Empty animations cannot be blended."));

		// make a new animation instance
		anim::Animation out(anim_a.fps());

		const unsigned tr_a = values.get(a_trA);
		const unsigned tr_b = values.get(a_trB);
		const unsigned tr_len = values.get(a_transitionLength);

		// frame counts for the first and second part of the transition
		const unsigned tr_start = tr_len / 2;
		const unsigned tr_end = tr_len - tr_len / 2;

		if(tr_a > tr_start && anim_a.size() > tr_a + tr_end) {
			// "before" transition - just copy animation frames from anim A
			for(unsigned fi = 0; fi < tr_a - tr_start; ++fi)
				out.addFrame(anim_a.frame(fi));

			// transition itself
			if(tr_b > tr_start && anim_b.size() > tr_b + tr_end) {
				for(unsigned fi = 0; fi < tr_len; ++fi) {
					// weight from 0..1 (excluding 0 and 1)
					const float weight = (float)(fi + 1) / (float)(tr_len + 1);

					// get the two frames to be blended
					unsigned fa_index = tr_a - tr_start + fi;
					assert(fa_index > 0);
					unsigned fb_index = tr_b - tr_start + fi;
					assert(fb_index > 0);

					anim::Skeleton f1 = anim_a.frame(fa_index);
					anim::Skeleton f2 = anim_b.frame(fb_index);

					// make them into "delta" frames by making their root relative to previous frame
					f1[0].tr() = anim_a.frame(fa_index - 1)[0].tr().inverse() * f1[0].tr();
					f2[0].tr() = anim_b.frame(fb_index - 1)[0].tr().inverse() * f2[0].tr();

					// blend them
					for(unsigned bi = 0; bi < f1.size(); ++bi) {
						Imath::V3f t = f1[bi].tr().translation * (1.0f - weight) + f2[bi].tr().translation * (weight);

						Imath::Quatf q;
						{
							const Imath::Quatf q1 = f1[bi].tr().rotation;
							const Imath::Quatf q2 = f2[bi].tr().rotation;

							if((q1 ^ q2) > 0.0f)
								q = q1 * (1.0f - weight) + q2 * (weight);
							else
								q = q1 * (1.0f - weight) + -q2 * (weight);
						}

						f1[bi].tr().translation = t;
						f1[bi].tr().rotation = q;
					}

					// add the root of previous frame (if any)
					if(!out.empty())
						f1[0].tr() = out.back()[0].tr() * f1[0].tr();

					// and push the frame to the output
					out.addFrame(f1);
				}

				// frames after transition
				for(unsigned fi = tr_b + tr_end; fi < anim_b.size(); ++fi) {
					// get the frame
					anim::Skeleton f = anim_b.frame(fi);

					// make into "differential" frame
					assert(fi > 0);
					f[0].tr() = anim_b.frame(fi - 1)[0].tr().inverse() * f[0].tr();

					// "add" it to the end of the output animation
					assert(!out.empty());
					f[0].tr() = out.back()[0].tr() * f[0].tr();

					// and add this to the end of the animation
					out.addFrame(f);
				}
			}
		}

		values.set(a_outAnim, out);
	}

	return dependency_graph::State();
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_animA, "anim_a", anim::Animation(), possumwood::AttrFlags::kVertical);
	meta.addAttribute(a_animB, "anim_b", anim::Animation(), possumwood::AttrFlags::kVertical);
	meta.addAttribute(a_transitionLength, "transition_length", 11u);
	meta.addAttribute(a_trA, "transition_a");
	meta.addAttribute(a_trB, "transition_b");
	meta.addAttribute(a_outAnim, "out_anim", anim::Animation(), possumwood::AttrFlags::kVertical);

	meta.addInfluence(a_animA, a_outAnim);
	meta.addInfluence(a_animB, a_outAnim);
	meta.addInfluence(a_transitionLength, a_outAnim);
	meta.addInfluence(a_trA, a_outAnim);
	meta.addInfluence(a_trB, a_outAnim);

	meta.setEditor<Editor>();
	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("anim/animation/transition", init);

}  // namespace
