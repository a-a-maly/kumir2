/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

#ifndef DRAWMODULE_H
#define DRAWMODULE_H

// Base class include
#include "drawmodulebase.h"
#include <QColor>
#include <QPointF>
class QTimer;
class QToolButton;
class QGraphicsPolygonItem;

class DrawNavigator;
namespace ActorDraw
{

class DrawScene;
class DrawView;

class DrawModule : public DrawModuleBase
{
	Q_OBJECT
public /* methods */:
	DrawModule(ExtensionSystem::KPlugin *parent);

	void terminateEvaluation() {}
	static QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters();

	QWidget *mainWidget() const;
	QWidget *pultWidget() const;

	void handleGuiReady();
	void showInitialView();

	void setAutoFit(bool state)
	{
		autoFit = state;
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

	QGraphicsPolygonItem *Pen() const
	{
		return mPen;
	}

	double zoom() const;

	void scalePen(double factor);


	QColor axisColor() const
	{
		return QColor(DrawSettings()->value("AxisColor", "#999900").toString());
	}

	void redrawPicture();

	QString initialize(const QStringList &configurationParameters, const ExtensionSystem::CommandLine &runtimeParameters);

public slots:
	void changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current);
	void loadActorData(QIODevice *source);
	void reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList &keys);
	void reset();
	void setAnimationEnabled(bool enabled);
	void runSetupPen();
	void runReleasePen();
	void runSetPenColor(const Color &color);
	void runMoveTo(const qreal x, const qreal y);
	void runMoveBy(const qreal dX, const qreal dY);
	void runAddCaption(const qreal width, const QString &text);
	bool runIsLineAtCircle(const qreal x, const qreal y, const qreal radius);
	void zoomFullDraw();


	void drawNet();
	void autoNetChange(bool value);
	void netStepChange(double value);
	void zoomIn();
	void zoomOut();
	void zoomNorm();

	void showNavigator(bool state);
	void openFile();
	void saveFile();
	void redraw();
	void updateDraw();

private:
	static ExtensionSystem::SettingsPtr DrawSettings();
	void createGui();
	void CreatePen(void);
	void fitView(const QRectF &rect, const QSizeF &marginPixels);

private:
	DrawScene *CurScene;
	DrawView *CurView;
	QGraphicsPolygonItem *mPen;
	QMutex mutex;
	double netStepX, netStepY;
	QColor netColor;
	bool autoNet;
	bool autoFit;
	bool fitPending;
	bool penIsDrawing;
	bool firstShow;
	QColor penColor;
	ExtensionSystem::GlobalState currentState;
	DrawNavigator *navigator;
	QToolButton *showToolsBut;
	QDir curDir;
	bool animate;
	QTimer *redrawTimer;
	QPointF curPos;
};


} // namespace ActorDraw

#endif // DRAWMODULE_H
