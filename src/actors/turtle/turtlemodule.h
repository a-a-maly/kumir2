/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

#ifndef TURTLEMODULE_H
#define TURTLEMODULE_H

// Base class include
#include "turtlemodulebase.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QLabel>
#include <QDebug>

class QMouseEvent;
class QWheelEvent;
class QResizeEvent;
class QToolButton;


class DrawNavigator;
class TurtlePult;

namespace ActorTurtle
{


class TurtleModule;

class TurtlePen: public QGraphicsItem
{

public:
	TurtlePen(QGraphicsItem *parent, QString svgFileName);

	void setTailColor(QColor  color)
	{
		tailColor = color;
	}

	void tailUp()
	{
		tailup = true;
	}

	void tailDown()
	{
		tailup = false;
	}

	QRectF boundingRect() const
	{
		return QRectF(-15, -30, mX, mY);
	}

	bool isTailUp() const
	{
		return tailup;
	}

	void setTurtleVis(bool vis)
	{
		turtle->setVisible(vis);
		turtle->update();

	}

	void paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *option,
		QWidget *widget
	);

	void resetPen()
	{
		mPosX = 0;
		mPosY = 0;
	}

	void movePen(qreal dx, qreal dy)
	{
		mPosX += dx;
		mPosY += dy;
	}

	QPointF penPos() const
	{
		return QPointF(mPosX, mPosY);
	}

private:
	qreal mX, mY;
	QGraphicsSvgItem *turtle;
	QPainterPath tail;
	QColor tailColor;
	QImage *turtleImage;
	bool tailup;
	qreal mPosX, mPosY;
};

class TurtleView : public QGraphicsView
{
	Q_OBJECT
public:
	TurtleView(QWidget *parent = 0)
	{
		c_scale = 1;
		pressed = false;
		press_pos = QPoint();
		firstResize = true;
		net = true;
		smallNetLabel = new QLabel(this);
		smallNetLabel->hide();
		smallNetLabel->setText(trUtf8("Слишком мелкая сетка"));
	};
	void setDraw(TurtleModule *draw, QMutex *mutex)
	{
		DRAW = draw;
		dr_mutex = mutex;
	};
	double zoom()const
	{
		return c_scale;
	};
	void setZoom(double zoom);
	void setNet();//RESIZE NET

	bool isNet() const
	{
		return net;

	}
	void forceRedraw()
	{
		// horizontalScrollBar()->setValue(horizontalScrollBar()->value() +10);
		// horizontalScrollBar()->setValue(horizontalScrollBar()->value()-10);

		// verticalScrollBar()->setValue(horizontalScrollBar()->value() +1);
		//  verticalScrollBar()->setValue(horizontalScrollBar()->value()-1);

	}

protected:
	// void scrollContentsBy ( int dx, int dy );
	void resizeEvent(QResizeEvent *event);
	void wheelEvent(QWheelEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);

private:
	TurtleModule *DRAW;
	double c_scale;
	bool pressed;
	bool net;
	QPoint press_pos;
	bool firstResize;
	double lastStep;
	QMutex *dr_mutex;
	QLabel *smallNetLabel;

};


class TurtleScene : public QGraphicsScene
{
	Q_OBJECT
public:
	TurtleScene(QObject *parent = 0) {};
	void drawNet(double startx, double endx, double starty, double endy, QColor color, const double step, const double stepY, bool net, qreal nw, qreal aw);
	void setDraw(TurtleModule *draw, QMutex *mutex)
	{
		DRAW = draw;
		dr_mutex = mutex;
	}

	void addDrawLine(QLineF lineF, QColor color, qreal width);
	void reset()
	{
		for (int i = 0; i < lines.count(); i++) {
			removeItem(lines.at(i));
		}
		lines.clear();
		for (int i = 0; i < texts.count(); i++) {
			removeItem(texts.at(i));
		}
		texts.clear();

	}
	void fromBufferToScene()
	{
		QGraphicsItemGroup *buff = createItemGroup(itemsBuffer);
		buff->setZValue(90);
		addItem(buff);
		clearBuffer();
	}
	void clearBuffer()
	{
		itemsBuffer.clear();
	}
	int buffSize()
	{
		return itemsBuffer.count();
	}
	void DestroyNet();
	void drawOnlyAxis(double startx, double endx, double starty, double endy, qreal aw);
	bool netIsShown()
	{
		return Netlines.count() > 0;
	}
	bool isLineAt(const QPointF &pos, qreal radius);
	qreal drawText(const QString &Text, qreal widthChar, QPointF from, QColor color); //Returns offset of pen.
	QRectF getRect();
	int saveToFile(const QString &p_FileName);
	int loadFromFile(const QString &p_FileName);
protected:
	// void resizeEvent ( QResizeEvent * event );
private:
	bool isUserLine(QGraphicsItem *); //Return true if item is user item;
	QList<QGraphicsLineItem *> lines;
	QList<QGraphicsLineItem *> Netlines;
	QList<QGraphicsLineItem *> linesDubl; //Базовый чертеж
	QList<QGraphicsSimpleTextItem *> texts;
	TurtleModule *DRAW;
	QMutex *dr_mutex;
	QList<QGraphicsItem *> itemsBuffer;
};


class TurtleModule : public TurtleModuleBase
{
	Q_OBJECT
public /* methods */:
	TurtleModule(ExtensionSystem::KPlugin *parent);
	static QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters();
	QWidget *mainWidget() const;
	QWidget *pultWidget() const;
	void handleGuiReady();
	QString initialize(const QStringList &configurationParameters, const ExtensionSystem::CommandLine &runtimeParameters);
	void scalePen(double factor)
	{
		// mutex.lock();
		mPen->setScale(factor);
		//  mTurt->setScale(factor);
		qDebug() << "PenScale" << factor << "mPen->scale" << mPen->scale();
		//  mutex.unlock();
	}
	void setAutoFit(bool state)
	{
		autoFit = state;
		if (!state) {
			fitPending = false;
		}
	}
	bool isFitPending() const
	{
		return fitPending;
	}
	bool isAutoNet() const
	{
		return autoNet;
	}
	void setAutoNet(bool state)
	{
		autoNet = state;
	}
	double NetStepX() const
	{
		return netStepX;
	}
	void setNetStepX(double step)
	{
		netStepX = step;
	}
	double NetStepY() const
	{
		return netStepY;
	}
	void setNetStepY(double step)
	{
		netStepY = step;
	}
	double zoom() const
	{
		return CurView->zoom();
	}

	TurtlePen *Pen() const
	{
		return mPen;
	}
	static ExtensionSystem::SettingsPtr TurtleSettings();
	QColor axisColor() const
	{
		return QColor(TurtleSettings()->value("AxisColor", "#996600").toString());
	}
	TurtleView *getCurView() const
	{
		return CurView;
	}
	void updateTurtle();
public slots:
	void changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current);
	void loadActorData(QIODevice *source);
	void reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList &keys);
	void reset();
	inline void terminateEvaluation() {}
	void setAnimationEnabled(bool enabled);
	void runTailUp();
	void runTailDown();
	void runForward(const qreal dist);
	void runBack(const qreal dist);
	void runLeft(const qreal angle);
	void runRight(const qreal angle);
	void runSetPenColor(const Color &color);

	void drawNet();
	void autoNetChange(bool value);
	void netStepChange(double value);
	void zoomIn();
	void zoomOut();
	void zoomNorm();
	void zoomFullDraw();
	void fitDrawing();

	void showNavigator(bool state);

	void showTurtleSlt(bool state);
	void redraw();

private:
	void createGui();
	void CreatePen(void);

	bool animation;
	QMutex mutex;

	TurtleScene *CurScene;
	TurtleView *CurView;

	TurtlePen *mPen;
	QGraphicsSvgItem *mTurt;
	double netStepX, netStepY;
	QColor netColor;
	bool autoNet;
	bool autoFit;
	bool fitPending;
	bool penIsDrawing;
	Color penColor;
	ExtensionSystem::GlobalState currentState;
	DrawNavigator *navigator;
	QToolButton *showToolsBut, *showTurtleBut;
	QDir curDir;
	bool animate;
	qreal curAngle;
	qreal AncX, AncY;
	QTimer *redrawTimer;
	TurtlePult *pult;
};


} // namespace ActorTurtle

#endif // TURTLEMODULE_H
