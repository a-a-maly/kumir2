#include "drawview.h"
#include "drawmodule.h"

#include <QMutex>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QLabel>
#include <QDebug>


namespace ActorDraw {

static const qreal MAX_ZOOM = 1000000;

DrawView::DrawView(DrawModule *draw, QMutex *_mutex, QWidget *parent) : QGraphicsView(parent)
{
	c_scale = 1;
	pressed = false;
	press_pos = QPoint();
	firstResize = true;
	net = true;

	DRAW = draw;
	dr_mutex = _mutex;

	smallNetLabel = new QLabel(this);
	smallNetLabel->hide();
	smallNetLabel->setText(trUtf8("Слишком мелкая сетка"));
}

DrawView::~DrawView()
{
	delete smallNetLabel;
	smallNetLabel = 0;
}


void DrawView::forceRedraw()
{
	qDebug() << "ForceREDDR";
	QGraphicsView::resetCachedContent();
	QGraphicsView::update();
	QGraphicsView::repaint();
}


void DrawView::resizeEvent(QResizeEvent *event)
{
	if (viewport()->size().isEmpty()) {
		return;
	}
	if (firstResize) {
		qDebug() << "FirstresizeEvent";
		firstResize = false;
		DRAW->showInitialView();
	}
	qDebug() << "resizeEvent";
	// setViewportUpdateMode (QGraphicsView::NoViewportUpdate);
	if (!pressed) {
		DRAW->drawNet();
	}
	update();
	// setViewportUpdateMode (QGraphicsView::SmartViewportUpdate);
}

void DrawView::mousePressEvent(QMouseEvent *event)
{
	pressed = true;
	press_pos = event->pos();
	qDebug() << "Mouse press" << mapToScene(press_pos);

}

void DrawView::mouseReleaseEvent(QMouseEvent *event)
{
	pressed = false;
	DRAW->drawNet();
}

void DrawView::mouseMoveEvent(QMouseEvent *event)
{
	if (pressed) {
		DRAW->setAutoFit(false);
		dr_mutex->lock();
		setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
		QPointF delta = mapToScene(press_pos) - mapToScene(event->pos());

		QPointF center = mapToScene(viewport()->rect().center());
		//QPointF center = sceneRect().center();
		qDebug() << "CenterOn" << center;

		verticalScrollBar()->setValue(verticalScrollBar()->value() + (press_pos.y() - event->pos().y()));
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + (press_pos.x() - event->pos().x()));
		// centerOn(center);

		qDebug() << "CenterOn" << center << " realCenter" << mapToScene(viewport()->rect().center());
		qDebug() << "SCENERECTCENTER" << sceneRect().center().x();
		qDebug() << "DELTA" << delta << " xd" << press_pos.x() - event->pos().x() << " yd" << mapToScene(press_pos).y() - mapToScene(event->pos()).y();
		press_pos = event->pos();
		qDebug() << "Ppos" << press_pos;
		update();
		setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
		dr_mutex->unlock();
	}
}

void DrawView::setZoom(double zoom)
{
	if (zoom > MAX_ZOOM) {
		return;
	}
	if (zoom < 1 / MAX_ZOOM) {
		return;
	}
	this->scale(zoom / c_scale, zoom / c_scale);

	c_scale = zoom;
};

void DrawView::setNet()
{
	if (DRAW->isAutoNet()) {
		net = true;
		smallNetLabel->hide();
		double pixel_per_cell = DRAW->NetStepX() / (1 / c_scale);
		qreal stepX = DRAW->NetStepX();
		qreal stepY = DRAW->NetStepY();


		if (stepX > 5) {
			stepX = (int)(stepX - 0.5);
		}
		if (stepY > 5) {
			stepY = (int)(stepY - 0.5);
		}

		if (pixel_per_cell > 100) {
			stepX = DRAW->NetStepX() * 0.5;
			stepY = DRAW->NetStepY() * 0.5;
			if (stepX > 5) {
				stepX = (int)(stepX - 0.5);
			}
			if (stepY > 5) {
				stepY = (int)(stepY - 0.5);
			}
			DRAW->setNetStepX(stepX);
			DRAW->setNetStepY(stepY);

			// DRAW->scalePen(0.5);

		}
		if (pixel_per_cell < 15) {
			while (pixel_per_cell < 15) {
				pixel_per_cell = stepX / (1 / c_scale);

				stepX = stepX * 1.5;
				stepY = stepY * 1.5;
			}

			DRAW->setNetStepX(stepX);
			DRAW->setNetStepY(stepY);
			DRAW->drawNet();
		}
		DRAW->setNetStepX(stepX);
		DRAW->setNetStepY(stepY);
		qDebug() << "c_scale" << c_scale << "NetStep" << DRAW->NetStepX() << "PPC" << pixel_per_cell;
		update();
	} else {
		double pixel_per_cell = DRAW->NetStepX() / (1 / c_scale);
		if (pixel_per_cell < 15) { //Net step too short
			net = false;
			smallNetLabel->show();
		} else {
			if (pixel_per_cell > 15 && !net && pixel_per_cell < this->width() * 2) {
				net = true;
				smallNetLabel->hide();
			}
			if (pixel_per_cell > this->width() * 2) {
				net = false;
			}
		}
	}
}

void    DrawView::wheelEvent(QWheelEvent *event)
{
	DRAW->setAutoFit(false);
	float numDegrees = event->delta() / 8;
	qDebug() << "whell:" << numDegrees;
	//        c_scale=c_scale*0.8;
	qDebug() << "Scale" << c_scale;


	//setRenderHint(QPainter::Antialiasing);
	if (numDegrees > 0) {


		setZoom(zoom() * 1.189207);
		setNet();
		DRAW->scalePen(DRAW->Pen()->scale() * (1 / 1.189207));
		DRAW->drawNet();
	} else {
		if (c_scale < 3e-05) {
			return;
		}
		setZoom(zoom() * (1 / 1.189207));
		setNet();
		DRAW->scalePen(DRAW->Pen()->scale() * ((1.189207)));
		DRAW->drawNet();

	}

}

} // namespace ActorDraw
