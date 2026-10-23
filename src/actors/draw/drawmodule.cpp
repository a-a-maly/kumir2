/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

// Self include
#include "drawmodule.h"
#include "drawscene.h"
#include "drawview.h"
#include <kumir2-libs/widgets/draw_turtle_field.h>

// Kumir includes
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolButton>
#include <QIcon>
#include <QAction>
#include <QFileDialog>
#include <QVector2D>
#include <QTimer>
#include <QGraphicsItem>
#include <QDebug>

namespace ActorDraw
{
const int maxBuff = 1500;



ExtensionSystem::SettingsPtr DrawModule::DrawSettings()
{
	ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
	ExtensionSystem::KPlugin *plugin = pluginManager->loadedPlugins("ActorDraw")[0];
	return plugin->mySettings();
//        return pluginManager->settingsByObject(plugin);
}//Get  settings



DrawModule::DrawModule(ExtensionSystem::KPlugin *parent)
	: DrawModuleBase(parent)
{
	CurView = 0;
	firstShow = true;
	autoFit = true;
	fitPending = false;
	curPos = QPointF(0, 0);
	penIsDrawing = false;
	penColor = QColor(0, 0, 0, 255);
}


double DrawModule::zoom() const
{
	return CurView->zoom();
}

void DrawModule::scalePen(double factor)
{
	mutex.lock();
	mPen->setScale(factor);
	qDebug() << "PenScale" << factor << "mPen->scale" << mPen->scale();
	mutex.unlock();
}

void DrawModule::redrawPicture()
{
	CurScene->upd();

	CurView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	CurView->forceRedraw();

	CurScene->update(CurScene->sceneRect());

	CurView->viewport()->update();
	CurView->setZoom(CurView->zoom() * 2);
	CurView->setZoom(CurView->zoom() * 0.5);
}


void DrawModule::handleGuiReady()
{
	zoomFullDraw();
}

void DrawModule::createGui()
{
	netStepX = 1;
	netStepY = 1;
	autoNet = true;
	netColor = QColor("#669966");
	penIsDrawing = false;
	penColor = QColor(0, 0, 0, 255);

	CurView = new DrawView(this, &mutex);
	CurScene = new DrawScene(this, &mutex, CurView);
	navigator = new DrawNavigator(CurView);

	showToolsBut = new QToolButton(CurView);
	showToolsBut->move(20, 20);
	showToolsBut->setCheckable(true);

	connect(showToolsBut, SIGNAL(toggled(bool)), this, SLOT(showNavigator(bool)));

	showToolsBut->setIcon(QIcon(myResourcesDir().absoluteFilePath("menu-24x24-black.png")));
	connect(m_actionDrawSaveDrawing, SIGNAL(triggered()), this, SLOT(saveFile()));
	connect(m_actionDrawLoadDrawing, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(navigator, SIGNAL(redrawNet()), this, SLOT(drawNet()));
	connect(navigator, SIGNAL(autoNetChange(bool)), this, SLOT(autoNetChange(bool)));
	connect(navigator, SIGNAL(netStepChange(double)), this, SLOT(netStepChange(double)));

	connect(navigator->zoomUp, SIGNAL(pressed()), this, SLOT(zoomIn()));
	connect(navigator->zoomDown, SIGNAL(pressed()), this, SLOT(zoomOut()));
	connect(navigator->zoomNormal, SIGNAL(pressed()), this, SLOT(zoomNorm()));
	connect(navigator->zoomFullDraw, SIGNAL(pressed()), this, SLOT(zoomFullDraw()));

	navigator->setParent(CurView);
	navigator->setFixedSize(QSize(120, 175));

	navigator->move(20, showToolsBut->pos().y() + showToolsBut->height());
	navigator->hide();

	CurView->setScene(CurScene);
	CurView->centerOn(5, -5);
	// CurView->setViewportUpdateMode (QGraphicsView::NoViewportUpdate);//For better perfomance; Manual Update;
	drawNet();
	CreatePen();
	CurView->setZoom(50);
	CurView->setNet();
	netStepX = 1;
	netStepY = 1;

	CurView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	CurView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QBrush curBackground = QBrush(QColor(DrawSettings()->value("Draw/BackColor", "#99FF99").toString()));

	CurScene->setBackgroundBrush(curBackground);
	mPen->setBrush(QBrush(QColor("white")));
	//zoomFullDraw();
}

QString DrawModule::initialize(const QStringList &configurationParameters, const ExtensionSystem::CommandLine &)
{
	if (!configurationParameters.contains("tablesOnly")) {
		createGui();
		animate = false;
		redrawTimer = new QTimer(this);
		connect(redrawTimer, SIGNAL(timeout()), this, SLOT(redraw()));
		redrawTimer->start(250);
	}
	return "";
}

void  DrawModule::openFile()
{
	QString File = QFileDialog::getOpenFileName(mainWidget(), QString::fromUtf8("Открыть файл"), curDir.path(), "(*.ps)");

	if (File.isEmpty()) {
		return;
	}

	if (CurScene->loadFromFile(File) != 0) { //Get troubles when loading env.
		QMessageBox::information(CurView, "", QString::fromUtf8("Ошибка открытия файла! ") + File, 0, 0, 0);
		return;
	}
}

void  DrawModule::saveFile()
{
	QString File = QFileDialog::getSaveFileName(CurView, QString::fromUtf8("Сохранить файл"), curDir.path(), "(*.ps)");
	if (File.contains("*") || File.contains("?")) {
		QMessageBox::information(0, "", QString::fromUtf8("Недопустимый символ в имени файла!"), 0, 0, 0);
		return;
	}

	if (File.right(3) != ".ps") {
		File += ".ps";
	}

	QFileInfo info(File);
	QDir dir = info.absoluteDir();
	curDir = dir.path();
	CurScene->saveToFile(File);
}

void DrawModule::showNavigator(bool state)
{
	navigator->setVisible(state);
}


/* public static */ QList<ExtensionSystem::CommandLineParameter> DrawModule::acceptableCommandLineParameters()
{
	// See "src/shared/extensionsystem/commandlineparameter.h" for constructor details
	return QList<ExtensionSystem::CommandLineParameter>();
}

/* public slot */ void DrawModule::changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current)
{
	// Called when changed kumir state. The states are defined as enum ExtensionSystem::GlobalState:
	/*
	namespace ExtensionSystem {
	    enum GlobalState {
	        GS_Unlocked, // Edit mode
	        GS_Observe, // Observe mode
	        GS_Running, // Running mode
	        GS_Input,  // User input required
	        GS_Pause  // Running paused
	    };
	}
	*/
	// TODO implement me
	using namespace ExtensionSystem;  // not to write "ExtensionSystem::" each time in this method scope
	updateDraw();
	redraw();
	currentState = current;
	CurView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	const bool programFinished = old == GlobalState::GS_Running
		&& (current == GlobalState::GS_Observe || current == GlobalState::GS_Unlocked);
	if (programFinished && autoFit) {
		if (CurView->viewport()->size().isEmpty()) {
			fitPending = true;
		} else {
			zoomFullDraw();
		}
	}
	// if(current==GlobalState::GS_Running)
	// {
	//  redrawTimer->start(500);
	// }else
	//    redrawTimer->stop();
}

/* public slot */ void DrawModule::loadActorData(QIODevice *source)
{
	// Set actor specific data (like environment)
	// The source should be ready-to-read QIODevice like QBuffer or QFile
	Q_UNUSED(source);  // By default do nothing

}

/* public */ QWidget *DrawModule::mainWidget() const
{

	return  CurView;

}

/* public */ QWidget *DrawModule::pultWidget() const
{
//No pult for Draw module.
	return nullptr;
}

/* public slot */ void DrawModule::reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList &keys)
{
	// Updates setting on module load, workspace change or appliyng settings dialog.
	// If @param keys is empty -- should reload all settings, otherwise load only setting specified by @param keys
	// TODO implement me
	QBrush curBackground = QBrush(QColor(settings->value("BackColor", "#99FF99").toString()));

	CurScene->setBackgroundBrush(curBackground);
	netColor = QColor(settings->value("LineColor", "#669966").toString());
	drawNet();
	Q_UNUSED(keys);
}

/* public slot */ void DrawModule::reset()
{
	// Resets module to initial state before program execution
	// TODO implement me
	mPen->setBrush(QBrush(QColor("white")));
	penIsDrawing = false;
	mPen->setPos(0, 0);
	CurScene->reset();

	penColor = QColor(0, 0, 0, 255);
	QList<QRectF> rect;
	rect.append(CurView->sceneRect());
	CurView->updateScene(rect);
	CurView->show();
	CurView->forceRedraw();
	CurView->setZoom(CurView->zoom() * 2);
	CurView->setZoom(CurView->zoom() * 0.5);
	curPos = QPointF(0, 0);
}

/* public slot */ void DrawModule::setAnimationEnabled(bool enabled)
{
	// Sets GUI animation flag on run
	// NOTE this method just setups a flag and might be called anytime, even module not needed
	// TODO implement me
	// Q_UNUSED(enabled);  // Remove this line on implementation
	animate = enabled;
}

/* public slot */ void DrawModule::runSetupPen()
{
	/* алг опустить перо */
	// TODO implement me
	mutex.lock();
	mPen->setBrush(QBrush(penColor));
	penIsDrawing = true;
	mutex.unlock();

}

/* public slot */ void DrawModule::runReleasePen()
{
	/* алг поднять перо */
	// TODO implement me

	mutex.lock();
	mPen->setBrush(QBrush(QColor("white")));
	penIsDrawing = false;
	mutex.unlock();
}

/* public slot */ void DrawModule::runSetPenColor(const Color &color)
{
	/* алг установить цвет(цвет color) */

	penColor = QColor(color.r, color.g, color.b, color.a);
	mPen->setBrush(QBrush(penColor));
	qDebug()
			<< "DrawModule::runSetPenColor( { cssValue = \""
				<< penColor.name()
				<< "\" } )" << color.a;
}

/* public slot */ void DrawModule::runMoveTo(const qreal x, const qreal y)
{
	int curBuffSize = 0;
	mutex.lock();
	QPointF start = mPen->pos();
	mPen->setPos(x, -y);
	if (penIsDrawing) {
		CurScene->addDrawLine(QLineF(start, mPen->pos()), penColor, mySettings()->value("LineWidth", 4).toFloat());
	}
	curBuffSize = CurScene->buffSize();
	mutex.unlock();
	if (curBuffSize > maxBuff - 1) {
		msleep(3);
		mutex.lock();
		curBuffSize = CurScene->buffSize();
		mutex.unlock();

	}
}

/* public slot */ void DrawModule::runMoveBy(const qreal dX, const qreal dY)
{
	/* алг сместиться на вектор(вещ dX, вещ dY) */
	// TODO implement me
	int curBuffSize = 0;
	mutex.lock();
	QPointF start = mPen->pos();
	mPen->moveBy(dX, -dY);
	if (penIsDrawing) {
		CurScene->addDrawLine(QLineF(start, mPen->pos()), penColor, mySettings()->value("LineWidth", 4).toFloat());
	}
	//CurView->update();
	//if(animate)redrawPicture();
	curBuffSize = CurScene->buffSize();
	mutex.unlock();
	if (curBuffSize > maxBuff - 1) {
		msleep(3);
		mutex.lock();
		curBuffSize = CurScene->buffSize();
		mutex.unlock();
	}
}

/* public slot */ void DrawModule::runAddCaption(const qreal width, const QString &text)
{
	/* алг надпись(вещ width, лит text) */

	if (width <= 0) {
		setError(trUtf8("Отрицательная или нулевая ширина надписи!"));
		return;
	}
	qreal offset = CurScene->drawText(text, width, mPen->pos(), penColor);
	mPen->moveBy(offset, 0);
	qDebug() << "TExt ofset" << offset;
}


bool DrawModule::runIsLineAtCircle(const qreal x, const qreal y, const qreal radius)
{
	return CurScene->isLineAt(QPointF(x, -y), radius);
};

void DrawModule::drawNet()
{
	mutex.lock();

	QPointF start_d = CurView->mapToScene(CurView->geometry().topLeft());
	QPointF end_d = CurView->mapToScene(CurView->geometry().bottomRight());
	// QPointF center=CurView->mapToScene(CurView->geometry().center());
	// qDebug()<<"StartDeb"<<start_d<<end_d;
	// QPointF end=CurView->mapToScene(CurView->viewport()->rect().bottomRight().x(),CurView->viewport()->rect().bottomRight().y());

	CurView->setSceneRect(QRectF(QPointF(start_d.x() - (CurView->geometry().width()) * (1 / zoom()), start_d.y() - (CurView->geometry().height() * 2) * (1 / zoom())),
			QPointF(end_d.x() + 2000 * (1 / zoom()), end_d.y() + 2000 * (1 / zoom()))));
	QPointF start = CurView->sceneRect().topLeft();
	QPointF end = CurView->sceneRect().bottomRight();
	CurScene->drawNet(start.x(), end.x(), start.y(), end.y(), netColor, netStepX, NetStepY(), CurView->isNet(),
		mySettings()->value("NetWidth", 1).toFloat(), mySettings()->value("AxisWidth", 2).toFloat());

	// CurView->centerOn(center);
	mutex.unlock();
	qDebug() << "NETSEPX" << NetStepX();
	navigator->reDraw(zoom(), netStepY, NetStepX());
	CurView->update();

};


void DrawModule::zoomIn()
{
	autoFit = false;
	CurView->setZoom(CurView->zoom() * 2);
	scalePen(Pen()->scale() * (0.5));
	CurView->setNet();
	drawNet();
};
void DrawModule::zoomOut()
{
	autoFit = false;
	CurView->setZoom(CurView->zoom() * 0.5);
	scalePen(Pen()->scale() * (2));
	CurView->setNet();
	drawNet();
};
void DrawModule::zoomNorm()
{
	autoFit = false;


	CurView->setZoom(KUM_MULTI);

	mPen->setScale(0.03);
	CurView->centerOn(3, -3);
	if (isAutoNet()) {
		setNetStepX(1);
		setNetStepY(1);

		navigator->updateSelf(1, 1);
	}
	CurView->setNet();
	drawNet();

};

void DrawModule::showInitialView()
{
	if (fitPending) {
		zoomFullDraw();
	} else {
		fitView(QRectF(-5, -5, 10, 10), QSizeF(0, 0));
	}
}

void DrawModule::zoomFullDraw()
{
	mutex.lock();
	QRectF drawRect = CurScene->getRect();
	drawRect |= QRectF(mPen->pos() - QPointF(0.5, 0.5), QSizeF(1, 1));
	const QSizeF penPixels = mPen->sceneBoundingRect().size() * CurView->zoom();
	mutex.unlock();
	fitView(drawRect, penPixels);
	fitPending = false;
	autoFit = true;
}

void DrawModule::fitView(const QRectF &rect, const QSizeF &marginPixels)
{
	const QSize viewSize = CurView->viewport()->size();
	if (viewSize.isEmpty()) {
		return;
	}
	const qreal fitWidth = qMax(viewSize.width() - 2 * marginPixels.width(), viewSize.width() / 2.0);
	const qreal fitHeight = qMax(viewSize.height() - 2 * marginPixels.height(), viewSize.height() / 2.0);
	qreal oldZoom = CurView->zoom();
	CurView->setZoom(0.9 * qMin(fitWidth / rect.width(), fitHeight / rect.height()));
	CurView->setNet();
	qreal marginX = viewSize.width() / CurView->zoom();
	qreal marginY = viewSize.height() / CurView->zoom();
	CurView->setSceneRect(CurView->sceneRect() | rect.adjusted(-marginX, -marginY, marginX, marginY));
	CurView->centerOn(rect.center());
	drawNet();
	CurView->centerOn(rect.center());
	scalePen(Pen()->scale() * (oldZoom / CurView->zoom()));
	qDebug() << "fitView" << rect << "viewport" << viewSize << "zoom" << CurView->zoom();
	navigator->updateSelf(NetStepX(), NetStepY());
	CurView->update();
	CurView->forceRedraw();
	redraw();
}

void DrawModule::CreatePen(void)
{
	static const int points[] = {  0, 0, 6, -3, 18, -15, 15, -18, 3, -6 };
	QPolygon polygon;
	polygon.setPoints(5, points);
	QPolygonF polygonf = QPolygonF(polygon);

	mPen = new QGraphicsPolygonItem();
	mPen->setPolygon(polygonf);
	mPen->setZValue(100);
	mPen->setBrush(QBrush(QColor("black")));
	CurScene->addItem(mPen);
//        mPen->scale(0.5,0.5);
//        mPen->scale(0.5,0.5);
//        mPen->scale(0.5,0.5);
//        mPen->scale(0.5,0.5);
//        mPen->scale(0.5,0.5);
	mPen->setScale((0.0625) * mPen->scale());
	mPen->setZValue(100);
}


void DrawModule:: autoNetChange(bool value)
{
	setAutoNet(value);
	CurView->setNet();
	drawNet();
	redraw();
}

void DrawModule::netStepChange(double value)
{
	double oldValue = NetStepY();

	setNetStepY(value);
	setNetStepX(value);

	if (oldValue != value && value != 0) {
		CurView->setNet();
		drawNet();
	}
}

void DrawModule::updateDraw()
{
	CurView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	mutex.lock();

	CurScene->fromBufferToScene();

	redrawPicture();

	mutex.unlock();

	drawNet();
	CurView->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
}
void DrawModule::redraw()
{
	if (currentState != ExtensionSystem::GlobalState::GS_Running) {
		return;
	}
	updateDraw();



	//qApp->processEvents();

}
} // namespace ActorDraw
