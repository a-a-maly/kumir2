/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

// Self include
#include "turtlemodule.h"
#include "pult.h"

// Kumir includes
#include <kumir2-libs/widgets/draw_turtle_field.h>
#include <kumir2-libs/extensionsystem/pluginmanager.h>

#include <QLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QVector2D>
#include <QTimer>
#include <math.h>

namespace ActorTurtle
{
const int maxBuff = 500;
const int AnimTime = 100;


const int KumMulti = 50;
const int NetReserve = 15;
const double Pi = 3.14159265;
static const qreal MAX_ZOOM = 1000000;

class AAA : public QWidget
{
public:
	inline explicit AAA(QWidget *pult): QWidget(), pult_(pult)
	{
		setLayout(new QVBoxLayout);
		layout()->setContentsMargins(0, 0, 0, 0);
		layout()->addWidget(pult);
	}
	inline QSize minimumSizeHint() const
	{
		return pult_->minimumSizeHint();
	}
private:
	QWidget *pult_;
};

ExtensionSystem::SettingsPtr TurtleModule::TurtleSettings()
{
	ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
	ExtensionSystem::KPlugin *plugin = pluginManager->loadedPlugins("ActorTurtle")[0];
	return plugin->mySettings();
//        return pluginManager->settingsByObject(plugin);
}//Get  settings


QRectF TurtleScene::getRect() //User lines bounding rect
{
	if (lines.isEmpty()) {
		return QRectF(-50, -50, 100, 100);
	}
	QRectF boundRect;
	for (int i = 0; i < lines.count(); i++) {
		const QLineF line = lines.at(i)->line();
		boundRect |= QRectF(line.p1(), line.p2()).normalized();
	}
	return boundRect;
}

bool TurtleScene::isUserLine(QGraphicsItem *obj)
{
	for (int i = 0; i < lines.count(); i++) {
		if (lines.at(i) == obj) {
			return true;
		}
	}
	return false;
};

bool TurtleScene::isLineAt(const QPointF &pos, qreal radius)
{
	QGraphicsEllipseItem *testCirc = addEllipse(QRectF(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2));
	QList<QGraphicsItem *> colItems = collidingItems(testCirc);
	delete testCirc;

	for (int i = 0; i < colItems.count(); i++) {

		if (isUserLine(colItems.at(i))) {
			return true;
		}

	}
	return false;

};

qreal TurtleScene::drawText(const QString &Text, qreal widthChar, QPointF from, QColor color)
{
	QFont font("Courier", 12);
	font.setPointSizeF(KumMulti);
	QFontMetricsF fontMetric(font);
	qreal bs = fontMetric.boundingRect(Text).width();
	qreal psizeF = widthChar / bs;

	//  font.setPointSizeF(psizeF*inc);
	bs = fontMetric.boundingRect("OOOXX").width() / 5;

	fontMetric = QFontMetricsF(font);
	qDebug() << "Char Size:" << fontMetric.boundingRect("OOOXX").width() / 5000;

	QGraphicsSimpleTextItem *tItem = new QGraphicsSimpleTextItem(Text);
	tItem->setFont(font);
	texts.append(tItem);
//        texts.last()->scale(0.001,0.001);
	texts.last()->setScale(psizeF);
	texts.last()->setPos(from.x(), from.y() - (fontMetric.boundingRect(Text).height()*psizeF));
	texts.last()->setPen(QPen(color));
	texts.last()->setZValue(90);
	return widthChar;
};
void TurtleScene::addDrawLine(QLineF lineF, QColor color, qreal width)
{
	if (lineF.length() == 0) {
		return;
	}
	QGraphicsLineItem *line = new QGraphicsLineItem(lineF);
	//addLine(lineF);
	QPen mp = QPen(QColor(color));
	mp.setWidthF(width);
	mp.setCapStyle(Qt::RoundCap);
	mp.setCosmetic(true);
	line->setPen(mp);
	line->setZValue(90);
	lines.append(line);
	itemsBuffer.append(line);

	//getfromB

}
void TurtleScene::DestroyNet()
{
	//  qDebug()<<Netlines.count();
	//  dr_mutex->lock();
	for (int i = 0; i < Netlines.count(); i++) {

		delete Netlines[i];
	}
	Netlines.clear();
	//   dr_mutex->unlock();
}
void TurtleScene::drawOnlyAxis(double startx, double endx, double starty, double endy, qreal aw)
{

	QPen axisPen = QPen(DRAW->axisColor());
	axisPen.setWidthF(aw);
	axisPen.setCosmetic(true);
	Netlines.append(addLine(startx - qAbs(startx - endx), 0, endx + qAbs(startx - endx), 0));
	Netlines.last()->setPen(axisPen);
	Netlines.last()->setZValue(1);

	Netlines.append(addLine(0, starty - qAbs(starty - endy), 0, endy + qAbs(startx - endx)));
	Netlines.last()->setPen(axisPen);
	Netlines.last()->setZValue(1);

}



void TurtleScene::drawNet(
	double startx, double endx,
	double starty, double endy,
	QColor color,
	double step, double stepY,
	bool net,
	qreal nw, qreal aw
) {
	QColor AxisColor = DRAW->axisColor();
	auto lp = QPen(color);
	lp.setWidthF(nw);
	lp.setCosmetic(true);

	DestroyNet();//MUTEXED
	if (!net) {
		drawOnlyAxis(startx, endx, starty, endy, aw);
		return;
	}
	int lines = qRound(startx / step);


	// return;
	if (!dr_mutex->tryLock(20)) {
		return;
	}
	startx = lines * step;
	double fx1 = startx - NetReserve * step, fx2, fy1, fy2;
	while (fx1 < endx + NetReserve * step) {
		fx1 = fx1 + step;
		fx2 = fx1;
		fy1 = starty - NetReserve * step;
		fy2 = endy + NetReserve * step;

		Netlines.append(addLine(fx1, fy1, fx2, fy2));
		Netlines.last()->setZValue(0.5);
		Netlines.last()->setPen(lp);
		if (fx1 > 0 - 1 / DRAW->zoom() && fx1 < 0 + 1 / DRAW->zoom()) {
			QPen axisPen = QPen(AxisColor);
			axisPen.setWidthF(aw);
			axisPen.setCosmetic(true);
			// axisPen.setWidth(3/DRAW->zoom());
			Netlines.last()->setPen(axisPen);
			Netlines.last()->setZValue(1);
			//  Netlines.append(addLine(fx1+1/DRAW->zoom(), fy1 , fx2+1/DRAW->zoom(), fy2 ));
			//  Netlines.last()->setZValue(1);
			//  Netlines.last()->setPen(axisPen);
		}
	}

	lines = starty / stepY;
	starty = lines * stepY;
	fy1 = starty - NetReserve * stepY;

	while (fy1 < endy + NetReserve * stepY) {
		fy1 = fy1 + stepY;
		fy2 = fy1;
		fx1 = startx - NetReserve * stepY;
		fx2 = endx + NetReserve * stepY;

		Netlines.append(addLine(fx1, fy1, fx2, fy2));
		Netlines.last()->setZValue(0.5);
		Netlines.last()->setPen(lp);
		if (fy1 > 0 - 1 / DRAW->zoom() && fy1 < 0 + 1 / DRAW->zoom()) {
			QPen axisPen = QPen(AxisColor);
			axisPen.setWidthF(aw);
			axisPen.setCosmetic(true);
			Netlines.last()->setPen(axisPen);
			Netlines.last()->setZValue(1);
			//Netlines.append(addLine(fx1, fy1+1/DRAW->zoom() , fx2, fy2+1/DRAW->zoom() ));
			//Netlines.last()->setZValue(1);
			// Netlines.last()->setPen(axisPen);
		}

	}
	dr_mutex->unlock();
}

int TurtleScene::loadFromFile(const QString &p_FileName)
{
	QFileInfo fi(p_FileName);
	QString name = fi.fileName();
	QString Title = QString::fromUtf8("Чертежник - ") + name;
	qreal CurrentScale = 1.0;
	qreal x1 = 0.0, y1 = 0.0;
	QColor CurColor;
	int NStrok = 0;

	QString tmp;
	QByteArray ttt;
	QString l_String;
	QFile l_File(p_FileName);

	if (!l_File.open(QIODevice::ReadOnly)) {
		QMessageBox::information(NULL, "", QString::fromUtf8("Ошибка открытия файла"), 0, 0, 0);
		return 1;
	}

	//QMessageBox::information( 0, "", tmp, 0,0,0);
	ttt = l_File.readLine();
	l_String = QString::fromUtf8(ttt);

	//QMessageBox::information( MV, "",l_String , 0,0,0);
	if (l_String.isNull()) {
		l_File.close();
		QMessageBox::information(NULL, "", QString::fromUtf8("Ошибка чтения строки"), 0, 0, 0);
		return 1;
	}
	l_String = l_String.simplified();
	QStringList l_List = l_String.split(' ');


	if (!(l_String == "%!PS-Adobe-1.0 EPSF-1.0")) {
		l_File.close();
		QMessageBox::information(NULL, "", QString::fromUtf8("Это не PS - файл"), 0, 0, 0);

		return 1;
	}

	// Вторая строка %%Creator: Cherteznik
	//  l_String = l_File.readLine();
	ttt = l_File.readLine();
	l_String = QString::fromUtf8(ttt);

	NStrok++;

	l_String = l_String.simplified();
	l_List = l_String.split(' ');
	if (!(l_List[1] == "Cherteznik"))
		//if (!(l_String == "%Creator: Cherteznik"))
	{
		QMessageBox::information(NULL, "", QString::fromUtf8("Это не файл чережника"), 0, 0, 0);
		l_File.close();
		return 1;
	}

	for (int i = 0; i < 15; i++) {
		//  l_String = l_File.readLine();
		ttt = l_File.readLine();
		l_String = QString::fromUtf8(ttt);
		NStrok++;
	}

	while (!l_File.atEnd()) {
		ttt = l_File.readLine();
		l_String = QString::fromUtf8(ttt);
		NStrok++;

		if (l_String.isNull()) {
			QMessageBox::information(NULL, "", QString::fromUtf8("Ошибка чтения строки"), 0, 0, 0);
			l_File.close();
			return 1;
		}

		l_String = l_String.simplified();
		l_List = l_String.split(' ');
		if (l_List.count() == 0) {
			continue;
		}

		if (l_List[l_List.count() - 1] == "stroke") {
			break;
		}

		if (l_List[l_List.count() - 1] == "setfont") {
			CurrentScale = l_List[2].toFloat();
			continue;
		}

		if (l_List[l_List.count() - 1] == "show") {
			tmp = "";
			int NWord = l_List.count() - 1;
			for (int j = 0; j < NWord; j++) {
				tmp += l_List[j] + " ";
			}
			//      QMessageBox::information( 0, "",  QString::fromUtf8(tmp), 0,0,0);
			int l = tmp.length();
			tmp.chop(2);
			tmp = tmp.right(l - 3);
			drawText(tmp, CurrentScale, QPoint(0, 0), CurColor);
			continue;
		}

		if (l_List[l_List.count() - 1] == "setrgbcolor") {
			CurColor.setRed(l_List[0].toInt());
			CurColor.setGreen(l_List[1].toInt());
			CurColor.setBlue(l_List[2].toInt());
			continue;
		}

		if (l_List[l_List.count() - 1] == "moveto") {
			x1 = l_List[0].toFloat();
			y1 = -l_List[1].toFloat();
			continue;
		}

		if (l_List[l_List.count() - 1] == "lineto") {
			qreal x2 = l_List[0].toFloat();
			qreal y2 = -l_List[1].toFloat();
			lines.append(addLine(x1, y1, x2, y2));
			lines.last()->setZValue(10);
			lines.last()->setPen(QPen(CurColor));
			continue;
		}

		tmp = QString::fromUtf8("Ошибка в строке %i").arg(NStrok);
		break;
	}

	l_File.close();
	return 0;
}


int TurtleScene::saveToFile(const QString &p_FileName)
{
	QFile l_File(p_FileName);
	char ctmp[200];
	if (!l_File.open(QIODevice::WriteOnly)) {
		return 1;
	}

	l_File.write("%!PS-Adobe-1.0 EPSF-1.0\n");
	l_File.write("%%Creator: Cherteznik\n");
	l_File.write("%%Pages: 1\n");
	l_File.write("%%Orientation: Portrait\n");

	// maximum, minimum
	qreal MinX = 100000, MaxX = -100000, MinY = 100000, MaxY = -100000;

	for (int i = 0; i < lines.count(); i++) {

		QLineF TmpLine = lines[i]->line();
		qreal VecX1 = TmpLine.x1();
		qreal VecY1 = -TmpLine.y1();
		qreal VecX2 = TmpLine.x2();
		qreal VecY2 = -TmpLine.y2();

		if (VecX1 < MinX) {
			MinX = VecX1;
		}
		if (VecY1 < MinY) {
			MinY = VecY1;
		}
		if (VecX1 > MaxX) {
			MaxX = VecX1;
		}
		if (VecY1 > MaxY) {
			MaxY = VecY1;
		}

		if (VecX2 < MinX) {
			MinX = VecX2;
		}
		if (VecY2 < MinY) {
			MinY = VecY2;
		}
		if (VecX2 > MaxX) {
			MaxX = VecX2;
		}
		if (VecY2 > MaxY) {
			MaxY = VecY2;
		}
	}

	double Scale = 1.0;

	if (MaxX - MinX > MaxY - MinY) {
		Scale = (596 - 10) / (MaxX - MinX);
	} else {
		Scale = (842 - 10) / (MaxY - MinY);
	}
	Scale *= 0.9;

	l_File.write("%%BoundingBox: 0 0 596 842\n");
	l_File.write("%%HiResBoundingBox: 0 0 596 842\n");
	l_File.write("%%EndComments\n");
	l_File.write("%%Page: 1 1\n");
	sprintf(ctmp, "%f %f translate\n", -MinX, -MinY);
	l_File.write(ctmp);
	sprintf(ctmp, "%f %f scale\n", Scale, Scale);
	l_File.write(ctmp);
	l_File.write("gsave [1 0 0 1 0 0] concat\n");
	l_File.write("0 0 0 setrgbcolor\n");
	l_File.write("[] 0 setdash\n");
	l_File.write("1 setlinewidth\n");
	l_File.write("0 setlinejoin\n");
	l_File.write("0 setlinecap\n");
	l_File.write("newpath\n");

	for (int i = 0; i < lines.count(); i++) {
		QLineF TmpLine = lines[i]->line();
		QColor color = lines[i]->pen().color();
		sprintf(ctmp, "%i %i %i setrgbcolor\n", color.red(), color.green(), color.blue());
		l_File.write(ctmp);

		qreal VecX1 = TmpLine.x1();
		qreal VecY1 = -TmpLine.y1();
		qreal VecX2 = TmpLine.x2();
		qreal VecY2 = -TmpLine.y2();

		sprintf(ctmp, "%f %f moveto\n", VecX1, VecY1);
		l_File.write(ctmp);

		sprintf(ctmp, "%f %f lineto\n", VecX2, VecY2);
		l_File.write(ctmp);
	}

	QString TmpText;
	QByteArray ccc;
	for (int i = 0; i < texts.count(); i++) {
		qreal FontSize = texts[i]->font().pointSizeF();
		sprintf(ctmp, "/Courier findfont %f scalefont setfont\n", FontSize);
		l_File.write(ctmp);

		qreal tmpX = texts[i]->pos().x();
		qreal tmpY = texts[i]->pos().y();
		sprintf(ctmp, "%f %f moveto\n", tmpX, tmpY);
		l_File.write(ctmp);

		QColor color = texts[i]->pen().color();
		sprintf(ctmp, "%i %i %i setrgbcolor\n", color.red(), color.green(), color.blue());
		l_File.write(ctmp);

		TmpText = "(" + texts[i]->text() + ") show\n";
		ccc = TmpText.toUtf8();
		l_File.write(ccc);
	}

	l_File.write("stroke\n");
	l_File.write("grestore\n");
	l_File.write("showpage\n");
	l_File.close();
	return 0;
}

void TurtleView::paintEvent(QPaintEvent *event)
{
	//dr_mutex->lock();

	QGraphicsView::paintEvent(event);
	event->accept();
	//   dr_mutex->unlock();

}
void TurtleView::resizeEvent(QResizeEvent *event)
{
	if (viewport()->size().isEmpty()) {
		return;
	}
	bool centerOnOrigin = firstResize;
	if (firstResize) {
		qDebug() << "FirstresizeEvent";
		centerOn(0, 0);
	}
	firstResize = false;
	qDebug() << "resizeEvent";
	// setViewportUpdateMode (QGraphicsView::NoViewportUpdate);
	if (!pressed) {
		DRAW->drawNet();
	}
	if (DRAW->isFitPending()) {
		DRAW->fitDrawing();
	} else if (centerOnOrigin) {
		centerOn(0, 0);
	}
	update();
	// setViewportUpdateMode (QGraphicsView::SmartViewportUpdate);
};


void TurtleView::mousePressEvent(QMouseEvent *event)
{
	pressed = true;
	press_pos = event->pos();
	qDebug() << "Mouse press" << mapToScene(press_pos);

};
void TurtleView::mouseReleaseEvent(QMouseEvent *event)
{
	pressed = false;

	DRAW->drawNet();
};
void TurtleView::mouseMoveEvent(QMouseEvent *event)
{
	if (pressed) {
		DRAW->setAutoFit(false);
		//  dr_mutex->lock();
		//   setViewportUpdateMode (QGraphicsView::SmartViewportUpdate);
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
		//   update();
		//  setViewportUpdateMode (QGraphicsView::NoViewportUpdate);
		// dr_mutex->unlock();
	}
};
void TurtleView::setZoom(double zoom)
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

void TurtleView::setNet()
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
			// DRAW->drawNet();


		}
		DRAW->setNetStepX(stepX);
		DRAW->setNetStepY(stepY);
		lastStep = stepX;
		qDebug() << "c_scale" << c_scale << "NetStep" << DRAW->NetStepX() << "PPC" << pixel_per_cell;
		// update();
	} else {
		double pixel_per_cell = DRAW->NetStepX() / (1 / c_scale);
		//if(!net)pixel_per_cell=lastStep/(1/c_scale);
		if (pixel_per_cell < 15) { //Net step too short
			net = false;
			smallNetLabel->show();
			// lastStep=DRAW->NetStepX();
		} else {
			if (pixel_per_cell > 15 && !net && pixel_per_cell < this->width() * 2) {
				net = true;
				smallNetLabel->hide();
				// DRAW->setNetStepX(lastStep);
				// DRAW->setNetStepY(lastStep);
			}
			if (pixel_per_cell > this->width() * 2) {
				net = false;
			}

		}


	}
};

void    TurtleView::wheelEvent(QWheelEvent *event)
{
	if (!dr_mutex->tryLock(20)) {
		return;
	}
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


	} else {
		if (c_scale < 3e-05) {
			dr_mutex->unlock();
			return;
		}
		setZoom(zoom() * (1 / 1.189207));
		setNet();

		DRAW->scalePen(DRAW->Pen()->scale() * ((1.189207)));


	}
	dr_mutex->unlock();
	DRAW->drawNet();
}



TurtleModule::TurtleModule(ExtensionSystem::KPlugin *parent)
	: TurtleModuleBase(parent)
{
	//Turtle = 0;
	curAngle = 0;
	AncX = 0;
	AncY = 0;
	//Tpult = 0;
	animation = false;
	autoFit = true;
	fitPending = false;
}

void TurtleModule::createGui()
{
	// Turtle=new turtle(myResourcesDir());
	pult = new TurtlePult(myResourcesDir());
	pult->turtleObj = this;
	CurView = new TurtleView();
	netStepX = 1;
	netStepY = 1;
	autoNet = true;
	netColor = QColor("gray");
	penIsDrawing = false;
	CurScene = new TurtleScene(CurView);
	navigator = new DrawNavigator(CurView);
	showToolsBut = new QToolButton(CurView);
	showToolsBut->move(20, 20);
	showToolsBut->setCheckable(true);


	connect(showToolsBut, SIGNAL(toggled(bool)), this, SLOT(showNavigator(bool)));

	showToolsBut->setIcon(QIcon(myResourcesDir().absoluteFilePath("menu-24x24-black.png")));


	showTurtleBut = new QToolButton(CurView);
	showTurtleBut->move(50, 20);
	showTurtleBut->setCheckable(true);
	showTurtleBut->setIcon(QIcon(myResourcesDir().absoluteFilePath("Trtl1.svg")));
	showTurtleBut->setChecked(true);
	connect(showTurtleBut, SIGNAL(toggled(bool)), this, SLOT(showTurtleSlt(bool)));
	// connect(m_actionDrawSaveDrawing,SIGNAL(triggered()),this,SLOT(saveFile()));
//   connect(m_actionDrawLoadDrawing,SIGNAL(triggered()),this,SLOT(openFile()));
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

	CurScene->setDraw(this, &mutex);
	CurView->setScene(CurScene);
	penColor.r = penColor.g = penColor.b = 0;
	penColor.a = 255;
	CurView->setDraw(this, &mutex);
	CurView->centerOn(5, -5);
	CurView->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
	drawNet();
	CreatePen();
	CurView->setZoom(50);
	CurView->setNet();
	netStepX = 1;
	netStepY = 1;



	CurView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	CurView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QBrush curBackground = QBrush(QColor(TurtleSettings()->value("BackColor", "#FFFFCC").toString()));

	CurScene->setBackgroundBrush(curBackground);
	Color Black;
	Black.r = 0;
	Black.g = 0;
	Black.b = 0;
	Black.a = 255;

	penColor = Black;
	zoomOut();
	zoomOut();
	zoomOut();
	zoomOut();
	zoomOut();
	autoFit = true;
}

QString TurtleModule::initialize(const QStringList &configurationParameters, const ExtensionSystem::CommandLine &)
{
	if (!configurationParameters.contains("tablesOnly")) {
		createGui();
		currentState = Shared::PluginInterface::GS_Unlocked;
		redrawTimer = new QTimer(this);
		connect(redrawTimer, SIGNAL(timeout()), this, SLOT(redraw()));
		redrawTimer->start(AnimTime);
	}
	return "";
}

/* public static */ QList<ExtensionSystem::CommandLineParameter> TurtleModule::acceptableCommandLineParameters()
{
	// See "src/shared/extensionsystem/commandlineparameter.h" for constructor details
	return QList<ExtensionSystem::CommandLineParameter>();
}

/* public slot */ void TurtleModule::changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current)
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
	usleep(10);
	CurScene->fromBufferToScene();
	CurScene->fromBufferToScene();
	mPen->setTransform(QTransform().translate(AncX * CurView->zoom(), AncY * CurView->zoom()).rotate(curAngle).translate(-AncX * CurView->zoom(), -AncY * CurView->zoom()));
	mPen->setPos(mPen->penPos());
	mPen->show();
	currentState = current;
	CurView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	CurView->forceRedraw();
	CurScene->update(CurScene->sceneRect());
	CurView->repaint();
	CurView->viewport()->update();

	drawNet();

	const bool programFinished = old == Shared::PluginInterface::GS_Running
		&& (current == Shared::PluginInterface::GS_Observe || current == Shared::PluginInterface::GS_Unlocked);
	if (programFinished && autoFit) {
		zoomFullDraw();
	}
}

/* public slot */ void TurtleModule::loadActorData(QIODevice *source)
{
	// Set actor specific data (like environment)
	// The source should be ready-to-read QIODevice like QBuffer or QFile
	Q_UNUSED(source);  // By default do nothing

}

/* public */ QWidget *TurtleModule::mainWidget() const
{

	return  CurView;;
}

/* public */ QWidget *TurtleModule::pultWidget() const
{
	// Returns module control view widget, or nullptr if there is no control view
	// NOTE: the method is const and might be called at any time,
	//       so DO NOT create widget here, just return!

	// TODO implement me
	if (!pult) {
		return 0;
	}
	// Returns module control view widget, or nullptr if there is no control view
	// NOTE: the method is const and might be called at any time,
	//       so DO NOT create widget here, just return!
	// TODO implement me
	static QWidget *dummy = new AAA(pult);
	return dummy;
	//return dummy;

}

/* public slot */ void TurtleModule::reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList &keys)
{
	// Updates setting on module load, workspace change or appliyng settings dialog.
	// If @param keys is empty -- should reload all settings, otherwise load only setting specified by @param keys
	// TODO implement me
	Q_UNUSED(settings);  // Remove this line on implementation
	Q_UNUSED(keys);  // Remove this line on implementation
}

/* public slot */ void TurtleModule::reset()
{
	CurScene->clearBuffer();
	mPen->tailUp();
	penIsDrawing = false;
	mPen->setPos(0, 0);
	mPen->resetPen();
	mPen->setRotation(0);
	curAngle = 0;
	mPen->setTransform(QTransform().translate(AncX * CurView->zoom(), AncY * CurView->zoom()).rotate(curAngle).translate(-AncX * CurView->zoom(), -AncY * CurView->zoom()));
	CurScene->reset();
	Color Black;
	Black.r = 0;
	Black.g = 0;
	Black.b = 0;
	Black.a = 255;

	penColor = Black;
	QList<QRectF> rect;
	rect.append(CurView->sceneRect());
	CurView->updateScene(rect);
	CurView->show();
	CurView->forceRedraw();
	CurView->setZoom(CurView->zoom() * 2);
	CurView->setZoom(CurView->zoom() * 0.5);

}

/* public slot */ void TurtleModule::setAnimationEnabled(bool enabled)
{
	// Sets GUI animation flag on run
	// NOTE this method just setups a flag and might be called anytime, even module not needed
	// TODO implement me
	animation = enabled;
	qDebug() << "Anim" << animation;

}

/* public slot */ void TurtleModule::runTailUp()
{

	mutex.lock();
	mPen->tailUp();
	CurScene->update();
	mutex.unlock();
}

/* public slot */ void TurtleModule::runTailDown()
{
	mutex.lock();
	mPen->tailDown();
	CurScene->update();
	mutex.unlock();
}

/* public slot */ void TurtleModule::runForward(const qreal dist)
{
	// if(animation)msleep(50);
	mutex.lock();
	qreal oldX = mPen->penPos().x();
	qreal oldY = mPen->penPos().y();
	qreal moveX = dist * sin(curAngle * (Pi / 180));
	qreal moveY = -dist * cos(curAngle * (Pi / 180));

	mPen->movePen(moveX, moveY);

	if (!mPen->isTailUp()) {
		CurScene->addDrawLine(QLineF(QPointF(oldX, oldY), mPen->penPos()), QColor(penColor.r, penColor.g, penColor.b, penColor.a), mySettings()->value("LineWidth", 4).toFloat());
	}
	// CurScene->update();
	mutex.unlock();
	int bsize = maxBuff;
	while (bsize > maxBuff - 1) {
		usleep(1);
		mutex.lock();
		bsize = CurScene->buffSize();
		mutex.unlock();
	}
}

/* public slot */ void TurtleModule::runBack(const qreal dist)
{
	mutex.lock();
	qreal oldX = mPen->penPos().x();
	qreal oldY = mPen->penPos().y();

	qreal moveX = -dist * sin(curAngle * (Pi / 180));
	qreal moveY = dist * cos(curAngle * (Pi / 180));

	mPen->movePen(moveX, moveY);


	if (!mPen->isTailUp()) {
		CurScene->addDrawLine(QLineF(QPointF(oldX, oldY), mPen->penPos()), QColor(penColor.r, penColor.g, penColor.b, penColor.a), mySettings()->value("LineWidth", 4).toFloat());
	}
	mutex.unlock();
	//  CurScene->update();
	int bsize = maxBuff;
	while (bsize > maxBuff - 1) {
		usleep(1);
		mutex.lock();
		bsize = CurScene->buffSize();
		mutex.unlock();

	}

}

/* public slot */ void TurtleModule::runLeft(const qreal angle)
{
	// if(animation)msleep(50);
	mutex.lock();
	curAngle = curAngle - angle;
	if (curAngle < 0) {
		curAngle = 360 + curAngle;
	}
	if (curAngle > 360) {
		curAngle = curAngle - 360;
	}


	// Turtle->rotate();
	mutex.unlock();
	// CurScene->update();

}

/* public slot */ void TurtleModule::runRight(const qreal angle)
{
	// if(animation)msleep(50);
	mutex.lock();
	curAngle = curAngle + angle;
	if (curAngle < 0) {
		curAngle = 360 + curAngle;
	}
	if (curAngle > 360) {
		curAngle = curAngle - 360;
	}
	//mPen->setTransform(QTransform().translate(AncX*CurView->zoom(), AncY*CurView->zoom()).rotate(curAngle).translate(-AncX*CurView->zoom(), -AncY*CurView->zoom()));

	// Turtle->rotate();
	mutex.unlock();
	//CurScene->update();

}
/* public slot */ void TurtleModule::runSetPenColor(const Color &color)
{
	/* алг установить цвет(цвет color) */

	penColor = color;
	mPen->setTailColor(QColor(penColor.r, penColor.g, penColor.b, penColor.a));

}
void TurtleModule::zoomIn()
{
	setAutoFit(false);
	CurView->setZoom(CurView->zoom() * 2);
	scalePen(Pen()->scale() * (0.5));
	CurView->setNet();
	drawNet();
};
void TurtleModule::zoomOut()
{
	setAutoFit(false);
	mutex.lock();
	CurView->setZoom(CurView->zoom() * 0.5);
	scalePen(Pen()->scale() * (2));


	mutex.unlock();
	CurView->setNet();
	drawNet();
};
void TurtleModule::zoomNorm()
{
	setAutoFit(false);


	CurView->setZoom(50);
	CurView->setNet();
	mPen->setScale(0.05);
	setNetStepX(1);
	setNetStepY(1);

	CurView->centerOn(3, -3);
	CurView->setNet();
	drawNet();
	navigator->updateSelf(1, 1);
};
void TurtleModule::showNavigator(bool state)
{
	navigator->setVisible(state);
};


void TurtleModule::showTurtleSlt(bool state)
{

	mPen->setTurtleVis(state);
	//mPen->setVisible(state);
	mPen->update();
	mPen->hide();
	mPen->show();
	// mPen->moveBy(-1,-1);

	CurScene->update();
	CurView->resetCachedContent();
	CurView->update();
	CurView->forceRedraw();
	CurView->repaint();
	qApp->processEvents();

}
void TurtleModule::netStepChange(double value)
{
	double oldValue = NetStepY();
	setNetStepY(value);
	setNetStepX(value);
	if (oldValue != value && value > navigator->netStepYS->minimum()) {
		drawNet();
	}
}
void TurtleModule:: autoNetChange(bool value)
{
	setAutoNet(value);
	getCurView()->setNet();
	drawNet();
}
void TurtleModule::drawNet()
{
	//  mutex.lock();
	//  mutex.lock();
	QPointF start_d = CurView->mapToScene(CurView->geometry().topLeft());
	QPointF end_d = CurView->mapToScene(CurView->geometry().bottomRight());


	CurView->setSceneRect(QRectF(QPointF(start_d.x() - (CurView->geometry().width()) * (1 / zoom()), start_d.y() - (CurView->geometry().height() * 2) * (1 / zoom())),
			QPointF(end_d.x() + 2000 * (1 / zoom()), end_d.y() + 2000 * (1 / zoom()))));
	// mutex.unlock();
	QPointF start = CurView->sceneRect().topLeft();
	QPointF end = CurView->sceneRect().bottomRight();
	CurScene->drawNet(start.x(), end.x(), start.y(), end.y(), netColor, netStepX, NetStepY(), CurView->isNet(), mySettings()->value("NetWidth", 1).toFloat(), mySettings()->value("AxisWidth", 2).toFloat());


	//  mutex.unlock();
	qDebug() << "NETSEPX" << NetStepX();
	navigator->reDraw(zoom(), netStepY, NetStepX());
	// CurView->update();

};

void TurtleModule::CreatePen(void)
{


	QPainterPath myPath;
	myPath.cubicTo(QPointF(-10 * 2, 13 * 2), QPointF(19, 17), QPointF(0, -1));

	mPen = new TurtlePen(nullptr, myResourcesDir().absoluteFilePath("Trtl1.svg"));




	mPen->setZValue(100);

	CurScene->addItem(mPen);


	mPen->setScale((0.03)*mPen->scale());
	mPen->setZValue(100);

	qDebug() << myResourcesDir().absoluteFilePath("Trtl1.svg");




}


void TurtleModule::handleGuiReady()
{
	fitPending = true;
	fitDrawing();
	fitPending = false;
}


void TurtleModule::zoomFullDraw()
{
	fitPending = true;
	QTimer::singleShot(0, this, SLOT(fitDrawing()));
}

void TurtleModule::fitDrawing()
{
	if (!fitPending) {
		return;
	}
	const QSize viewSize = CurView->viewport()->size();
	if (viewSize.isEmpty()) {
		return;
	}
	if (!mutex.tryLock()) {
		QTimer::singleShot(50, this, SLOT(fitDrawing()));
		return;
	}
	QRectF drawRect = CurScene->getRect();
	mutex.unlock();
	drawRect |= QRectF(mPen->penPos() - QPointF(1, 1), QSizeF(2, 2));
	const QSizeF penPixels = mPen->sceneBoundingRect().size() * CurView->zoom();
	const qreal fitWidth = qMax(viewSize.width() - 2 * penPixels.width(), viewSize.width() / 2.0);
	const qreal fitHeight = qMax(viewSize.height() - 2 * penPixels.height(), viewSize.height() / 2.0);

	qreal oldZoom = CurView->zoom();
	CurView->setZoom(0.9 * qMin(fitWidth / drawRect.width(), fitHeight / drawRect.height()));
	CurView->setNet();
	qreal marginX = viewSize.width() / CurView->zoom();
	qreal marginY = viewSize.height() / CurView->zoom();
	CurView->setSceneRect(CurView->sceneRect() | drawRect.adjusted(-marginX, -marginY, marginX, marginY));
	CurView->centerOn(drawRect.center());
	drawNet();
	CurView->centerOn(drawRect.center());
	scalePen(Pen()->scale() * (oldZoom / CurView->zoom()));
	navigator->updateSelf(NetStepX(), NetStepY());
	fitPending = false;
	autoFit = true;
	CurView->update();
}


void TurtleModule::updateTurtle()
{
	CurScene->fromBufferToScene();
	mPen->setTransform(QTransform().translate(AncX * CurView->zoom(), AncY * CurView->zoom()).rotate(curAngle).translate(-AncX * CurView->zoom(), -AncY * CurView->zoom()));
	mPen->setPos(mPen->penPos());
	mPen->show();
	CurScene->update();
	CurView->update();
}


void TurtleModule::redraw()
{
	if (currentState != Shared::PluginInterface::GS_Running) {
		return;
	}
	redrawTimer->stop();
	mutex.lock();

	updateTurtle();
	qApp->processEvents();
	mutex.unlock();
	if (!CurScene->netIsShown()) {
		drawNet();
	}
	redrawTimer->start(AnimTime);
}

TurtlePen::TurtlePen(QGraphicsItem *parent, QString svgFileName):
	QGraphicsItem(parent)
{
	turtle = new QGraphicsSvgItem(svgFileName, parent);

	// See Qt documentation!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	turtle->setTransform(QTransform::fromScale(0.01, 0.01), true);

	QMatrix mat;
	mat.rotate(180);
	mat.scale(0.5, 0.5);
	// turtle.setBoundingRect(QRect(-15,-15,15,15));
	tail.cubicTo(QPointF(-10 * 2, 13 * 2), QPointF(19, 17), QPointF(0, -1));
	tailColor = QColor(Qt::black);
	tail = mat.map(tail);
	mX = 30;
	mY = 30;
	mPosX = 0;
	mPosY = 0;
	tailup = false;
	turtleImage = new QImage(55, 60, QImage::Format_ARGB32);
	QPainter p(turtleImage);
	turtle->renderer()->render(&p, boundingRect());
}


void TurtlePen::paint(
	QPainter *painter,
	const QStyleOptionGraphicsItem *option,
	QWidget *widget
) {
	Q_UNUSED(option) Q_UNUSED(widget);
	painter->setPen(QColor(Qt::black));
	if (!tailup) {
		painter->setBrush(tailColor);
		painter->drawPath(tail);
		qDebug() << "TPE";
	}

	if (turtle->isVisible()) {
		turtle->renderer()->render(painter, boundingRect());
	}
	if (! turtle->renderer()->isValid()) {
		qDebug() << "BAD SVG RENDERER!!";
	}
}

} // namespace ActorTurtle
