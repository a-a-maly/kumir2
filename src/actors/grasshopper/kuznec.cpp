//****************************************************************************
//**
//** Copyright (C) 2004-2014 IMPB RAS. All rights reserved.
//**
//** This file is part of the KuMir.
//**
//** This file may be used under the terms of the GNU General Public
//** License version 2.0 as published by the Free Software Foundation
//** and appearing in the file LICENSE.GPL included in the packaging of
//** this file.
//**
//** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
//** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//**
//****************************************************************************/

//begin 14-02-2008

#include "kuznec.h"
#include "dialog.h"
#include "pult.h"
#include "grasshoppermodule.h"

#include <QGraphicsSimpleTextItem>
#include <QCloseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QMessageBox>
#include <QDesktopServices>
#include <QMenu>
#include <QMenuBar>
#include <QLineEdit>
#include <QFileDialog>
#include <math.h>

static QColor contrastColor(const QColor &back)
{
	return qGray(back.rgb()) <= 127 ? Qt::white : Qt::black;
}


void DrawView:: scrollContentsBy(int dx, int dy)
{
//qDebug() << QString("SCROLL EVENT ") << dx << " " << dy;
	QGraphicsView::scrollContentsBy(dx,  dy);
}


DrawView::DrawView(QGraphicsScene *scene, QWidget *window) :
	QGraphicsView(scene, window)
{
}


KumScene::KumScene(QWidget *parent) : QGraphicsScene(parent)
{
}


void KumScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	bool LeftButtonFlag;
	if (mouseEvent->button() == 0x00000001) {
		LeftButtonFlag = true;
	} else {
		LeftButtonFlag = false;
	}

	QPointF posScene = mouseEvent->scenePos();
	QPointF pos = mouseEvent->screenPos();
	qreal x = pos.x();
	qreal y = pos.y();
	qreal xScene = posScene.x();
	qreal yScene = posScene.y();

	emit MousePress(x, y, LeftButtonFlag, xScene, yScene);
};

void KumScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	bool LeftButtonFlag = false;
//qWarning("robot release");
	if (mouseEvent->button() == 0x00000001) {
		LeftButtonFlag = true;
	}

	QPointF pos = mouseEvent->screenPos();
	qreal x = pos.x();
	qreal y = pos.y();
	emit MouseRelease(x, y, LeftButtonFlag);
}

void KumScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	bool LeftButtonFlag = false;
//qWarning("robot move");
	if (mouseEvent->button() == 0x00000001) {
		LeftButtonFlag = true;
	}

	QPoint pos = mouseEvent->screenPos();
	int x = pos.x();
	int y = pos.y();
	emit MouseMove(x, y, LeftButtonFlag);
	QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void KumScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
	int Delta = wheelEvent->delta();
//qWarning("wheelEvent %i",Delta);
	emit MouseWheel(Delta);
}

//------------------------

KuznSled::KuznSled(int start, int fin)
{
	St = start;
	Fn = fin;
	Intens = 0;
	Color = Qt::black;
}

void KuznSled::paint(
	QPainter *painter,
	const QStyleOptionGraphicsItem *option,
	QWidget *widget
) {
	QColor color(
		Color.red() + (255 - Color.red()) * Intens / 255,
		Color.green() + (255 - Color.green()) * Intens / 255,
		Color.blue() + (255 - Color.blue()) * Intens / 255
	);
	qDebug() << "ABS " << abs(Fn - St);
	painter->setPen(QPen(color));
	painter->drawArc(St, 0, Fn, 32, 0, 360 * 8);
	//qDebug()<<"SLED PAINT";
}

//-------------------------------------------------------------------------------

void KumKuznec::showHideWindow(bool show) //Show - true
{
	MV->showNormal();
}

void KumKuznec::Close()
{
	qDebug() << "KUZNEC CLOSE Close";
	if ((Kpult->getLibMode()) || (autoClose)) {
		close();

		return;
	}
	Kpult->close();
}

void KumKuznec::resizeEvent(QResizeEvent *event)
{
	qDebug() << "RESIZE" << event->size();
	qDebug() << "RESIZE old:" << event->oldSize();
	if (event->oldSize() == QSize(-1, -1)) {
		return;
	}
	int Delta = SizeX;
	SizeX = event->size().width();
//SizeY=event->size().height ()-MenuHigth;

	Delta -= SizeX;
	qDebug() << "Delta" << Delta;
	SetWindowSize(10, 10, SizeX, SizeY);
	resize(event->size());
	qDebug() << "SizeX" << SizeX;

}

KumKuznec::KumKuznec(const QDir &mresd)
{
	autoClose = false;
	BackColor = QColor("lightgreen");
	RulerColor = QColor("blue");
	KuznecColor = QColor("blue");
	TrackColor = QColor("black");
#if QT_VERSION >= 0x050000
	curDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
#else
	curDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif
	if (!QDir(curDir).exists()) {
		QDir::root().mkpath(curDir);
	}
	curMode = 4;
	moving = false;

	MV = new QMainWindow();
	Kpult = new GrasshopperPult(mresd, this);
	MV->setAttribute(Qt::WA_KeyboardFocusChange, false);
	MV->setGeometry(10, 10, 500, 500);
	QIcon icon(":/grasshopper.png");
	Q_ASSERT(!icon.isNull());
	MV->setWindowIcon(icon);

	MV->setWindowTitle(QString::fromUtf8("Кузнечик - нет файла"));
	QPalette PallGrey;
	MV->setPalette(PallGrey);
	CreateMenu(); // debug09

	scene = new KumScene(MV);
	viewFrame = new QFrame(MV);
	viewFrame->setGeometry(0, MenuHigth + 1, DEFAULT_SIZEX, DEFAULT_SIZEY - MenuHigth);
	view = new DrawView(scene, viewFrame);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	connect(scene, SIGNAL(MousePress(qreal, qreal, bool, qreal, qreal)), this, SLOT(MousePress(qreal, qreal, bool, qreal, qreal)));
	connect(scene, SIGNAL(MouseRelease(qreal, qreal, bool)), this, SLOT(MouseRelease(qreal, qreal, bool)));
	connect(scene, SIGNAL(MouseMove(int, int, bool)), this, SLOT(MouseMove(int, int, bool)));
	connect(scene, SIGNAL(MouseWheel(int)), this, SLOT(MouseWheel(int)));
	this->setGeometry(MV->geometry());

	scene->setBackgroundBrush(QBrush(BackColor));
	StepX = 1.;
	StepY = 1.;
	SizeX = DEFAULT_SIZEX;
	SizeY = DEFAULT_SIZEY;
	this->setFixedHeight(SizeY + MenuHigth + 2);
	ButtonSdwig = 100;
	SqwareNetFlag = true;
	NetShowFlag = true;
	TmpNetShowFlag = true;
	LeftMousePressFlag = false;
	lines.clear();

	Createbtn12();
	Createbtn13();
	CreateXYLabel();

	WindowZoom = 1;
	WindowX0 = -SizeX / 2;
	WindowY0 = -SizeY / 2;

	NLastVec = 0;
	CurColor.Red = 0;
	CurColor.Green = 0;
	CurColor.Blue = 0;
	CurY = 0;
	CurX = 0;
	CurZ = 1;
	leftBorder = 0;
	rightBorder = 0;
	borderEnable = false;

	startPos = 0;
	PenPosition = true;
	mPen = NULL;
	SdwigNetX = 0.;
	SdwigNetY = 0.;

	CreatePen();
	SetColorString(QString::fromUtf8("красный"));
	MinZoom = 0.5;
	MaxZoom = 2;

	SetWindowSize(10, 10, SizeX, SizeY);
	WindowZoom = BASEZOOM;
	WindowX0 =  - SizeX / (WindowZoom);
	WindowY0 =  - SizeY / (WindowZoom * 2) - 1;

	connect(scene, SIGNAL(sceneRectChanged(QRectF)), view, SLOT(updateSceneRect(QRectF)));

	view->scale(BASEZOOM, BASEZOOM);
	view->centerOn(0, YCENTER);

	qDebug() << "Center: " << view->mapToScene(view->viewport()->rect().center()) << " YCENTER=" << YCENTER;

	WindowX0 = visibleRect().x();
	WindowY0 = visibleRect().y();

	RightMousePressFlag = false;
	Fstep = 3;
	Bstep = 2;
	Finish = 1;

	//Risuem flag:
	static const int points[] = { 0, 2, 2, 2, 1, 1, 2, 0, 0, 0, 0, 4};

	QPolygon polygon;
	polygon.setPoints(6, points);
	QPolygonF polygonf = QPolygonF(polygon);

	mFlag = new QGraphicsPolygonItem();
	mFlag->setPolygon(polygonf);
	mFlag->setZValue(100);
	mFlag->setBrush(QBrush(QColor(150, 100, 100)));
	mFlag->setRotation(180);
	mFlag->setScale(0.5);
	mFlag->moveBy(0, 0);

	mFlagLine = new QGraphicsLineItem(0, 0, 0, -1.5);
	mFlagLine->setPen(QPen(QColor(0, 0, 0)));
	mFlagLine->setZValue(100);

	redrawFlags();

	//Reset();
	mPen->setPos(0, -0.25);
	mPen->setBrush(QBrush(KuznecColor));
	scene->addItem(new QGraphicsLineItem(0, -ZOOMMULTIP * 2, 0, -ZOOMMULTIP * 2 + 0.0005));

	fileName = "";
	advancedTask = false;
	MV->hide();
}


//сдвиг на вектор
int KumKuznec::GoVector(double DeltaXX, double DeltaYY)
{
	qreal x1 = CurX;
	qreal y1 = -CurY;

	qreal x2 = CurX + DeltaXX;
	qreal y2 = -(CurY + DeltaYY);
	PenPosition = false;
	if (PenPosition) {
		mPen->setBrush(QBrush(QColor(CurColor.Red, CurColor.Green, CurColor.Blue)));
		lines.append(scene->addLine(x1, y1, x2, y2));
		lines.last()->setZValue(CurZ);
		lines.last()->setPen(QPen(QColor(CurColor.Red, CurColor.Green, CurColor.Blue)));
		CurZ += 0.01;
		CurX += DeltaXX;
		CurY += DeltaYY;
	} else {
		//QGraphicsLineItem *tmp = new QGraphicsLineItem(x1, y1 , x2, y2 );
		//lines.append(tmp);
		//mPen->setBrush(QBrush(QColor("white")));
		CurX += DeltaXX;
		CurY += DeltaYY;
	}
	mPen->setPos(CurX, -CurY);
	redrawFlags();
	return 0;
}

//опустить перо
void KumKuznec::PenDown()
{
	PenPosition = true;
}

//поднять перо
void KumKuznec::PenUp()
{
	PenPosition = false;
}

//переход в точку
int KumKuznec::GoPoint(double xx, double yy)
{
	double DeltaX = xx - CurX;
	double DeltaY = yy - CurY;

	GoVector(DeltaX, DeltaY);

	return 0;
}

//----------------------------------------------------------------------------
//--------------------------------------------------------------
// сдвиг окна в мировых координатах
//void KumKuznec::SetWindowPosition(double x0,double y0, double zoom)
//{
//}
//-----------------------------------------------------------------------

//-------------------------------------------------------------------
//сдвиг окна вправо
void KumKuznec::MoveRight(void)
{

	WindowX0 = WindowX0 - SizeX / WindowZoom / 4;

	WindowRedraw();

}
//------------------------------------------------------
//сдвиг окна влево
void KumKuznec::MoveLeft(void)
{

	WindowX0 = WindowX0 + SizeX / WindowZoom / 4;

	WindowRedraw();

}
//-----------------------------------------------------
//сдвиг окна вверх
void KumKuznec::MoveUp(void)
{
	WindowY0 = WindowY0 - SizeY / WindowZoom / 4;

	WindowRedraw();
}
//-----------------------------------------------------
//сдвиг окна вниз
void KumKuznec::MoveDown(void)
{
	WindowY0 = WindowY0 + SizeY / WindowZoom / 4;

	WindowRedraw();
}
//-----------------------------------------------------
void KumKuznec::ZoomUp(void)
{
	if (WindowZoom > MaxZoom) {
		return;
	}





	WindowZoom = WindowZoom * 2;


	//(WindowZoom>64)WindowY0 -= 1;
	view->scale(2., 2.);
	//view->centerOn(CenterX,0);
	WindowX0 = visibleRect().x();
	WindowY0 = visibleRect().y();
	qDebug() << "Zoom" << WindowZoom << " viewRECT" << view->sceneRect() << "sceneRect" << scene->sceneRect();
	//WindowRedraw();

	//if(WindowZoom>64){MoveDown();MoveDown();qDebug()<<"MOVE DOWN";};


}
//--------------------------------------------------
void KumKuznec::ZoomDown(void)
{
	if (WindowZoom < MinZoom) {
		return;
	}



	//if(WindowZoom>64)WindowY0 += 1;
	WindowZoom = WindowZoom / 2;


	QPointF center_p = view->mapToScene(view->viewport()->rect().center());
	view->scale(0.5, 0.5);
	//view->centerOn(CenterX,0);

	WindowX0 = visibleRect().x();
	WindowY0 = visibleRect().y();
	//view->centerOn(CenterX,0);
	qDebug() << "Zoom OUT" << WindowZoom << "SCENE RECT" << view->sceneRect() << "ViewRect" << visibleRect() << "Center" << center_p;
}
//-------------------------------------------------------------------
void KumKuznec::lockControls()
{
	/*
	    if  ( btnBox->isEnabled() )
	        btnBox->setEnabled(false);

	*/
}
//----------------------------------------
void KumKuznec::unlockControls()
{
	/*
	    if ( !btnBox->isEnabled() )
	        btnBox->setEnabled(true);
	*/
}
//----------------------------------------------------------
//сохранение в ps-файл
int KumKuznec::SaveToFile(QString p_FileName)
{
	QFile l_File(p_FileName);
	char ctmp[200];
	if (!l_File.open(QIODevice::WriteOnly)) {
		return 1;
	}

	//QString ttt = QString::fromUtf8("Чертежник - Начало");

//l_File.write( ttt.toUtf8());
	l_File.write("%!PS-Adobe-1.0 EPSF-1.0\n");
	QString l_String;
	l_File.write("%%Creator: Cherteznik\n");
	l_File.write("%%Pages: 1\n");
	l_File.write("%%Orientation: Portrait\n");


	// maximum, minimum

	qreal MinX, MaxX, MinY, MaxY, VecX1, VecX2, VecY1, VecY2;

	QLineF TmpLine;
	MinX = 1000000;
	MinY = 1000000;

	MaxX = -1000000;
	MaxY = -1000000;


	for (int i = 0; i < lines.count(); i++) {

		TmpLine = lines[i]->line();
		VecX1 = TmpLine.x1();
		VecY1 = -TmpLine.y1();
		VecX2 = TmpLine.x2();
		VecY2 = -TmpLine.y2();
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
	double Scale;

	if (MaxX - MinX > MaxY - MinY) {
		Scale = (596 - 10) / (MaxX - MinX);
	} else {
		Scale = (842 - 10) / (MaxY - MinY);
	}
	Scale = Scale * 0.9;

//	QString tmp1 = QString(ctmp)+" scale\n";



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
//QColor TmpColor;
	QPen TmpPen;
	QColor TmpColor;
	for (int i = 0; i < lines.count(); i++) {

		TmpLine = lines[i]->line();
		TmpPen = lines[i]->pen();
		TmpColor = TmpPen.color();
		sprintf(ctmp, "%i %i %i setrgbcolor\n", TmpColor.red(),  TmpColor.green(), TmpColor.blue());
		l_File.write(ctmp);

		VecX1 = TmpLine.x1();
		VecY1 = -TmpLine.y1();
		VecX2 = TmpLine.x2();
		VecY2 = -TmpLine.y2();

		sprintf(ctmp, "%f %f moveto\n", VecX1, VecY1);
		l_File.write(ctmp);

		sprintf(ctmp, "%f %f lineto\n", VecX2, VecY2);

		l_File.write(ctmp);


	}

	for (int i = 0; i < kumtexts.count(); i++) {
		qreal FontSize = kumtexts[i]->Size;
		sprintf(ctmp, "/Curier findfont %f scalefont setfont\n", FontSize);
		l_File.write(ctmp);

		qreal tmpX = kumtexts[i]->x;
		qreal tmpY = kumtexts[i]->y;
		sprintf(ctmp, "%f %f moveto\n", tmpX, tmpY);
		l_File.write(ctmp);


		//TmpPen = texts[i]->pen();
		//TColor = TmpPen.color();
		sprintf(ctmp, "%i %i %i setrgbcolor\n", kumtexts[i]->color.Red,  kumtexts[i]->color.Green, kumtexts[i]->color.Blue);
		l_File.write(ctmp);

		QString TmpText = "(" + texts[i]->text() + ") show\n";
		QByteArray ccc = TmpText.toUtf8();
		l_File.write(ccc);
	}

	l_File.write("stroke\n");
	l_File.write("grestore\n");
	l_File.write("showpage\n");
	l_File.close();
	return 0;
}


int KumKuznec::LoadFromFile(QString p_FileName)
{
	if ((fileName == "") && (advancedTask)) {
		if (QMessageBox::question(this, "", QString::fromUtf8("Сохранить изменения?"), QMessageBox::Yes, QMessageBox::No, 0) == QMessageBox::Yes) {
			SaveToFileActivated();
		}
	}

	QFileInfo fi(p_FileName);
	QString name = fi.fileName();                // name = "archive.tar.gz"

	QString Title = QString::fromUtf8("Кузнечик - ") + name;

	MV->setWindowTitle(Title);

	qreal CurrentScale;
	Q_UNUSED(CurrentScale);

	QString tmp = "";

	QString l_String;
	QFile l_File(p_FileName);

	if (!l_File.open(QIODevice::ReadOnly)) {
		QMessageBox::information(MV, "", QString::fromUtf8("Ошибка открытия файла"), 0, 0, 0);
		return 1;
	}

	QByteArray ttt;
	ttt = l_File.readLine();

	//Читаем размеры прыжков
	while (ttt[0] == ';') {
		ttt = l_File.readLine();
	}
	qDebug() << ttt;
	l_String = QString::fromUtf8(ttt);
	l_String = l_String.simplified();
	QStringList zadanie = l_String.split(" ");
	if (zadanie.count() < 2) {
		return 1;
	}
	Fstep = zadanie[0].toInt();
	Bstep = zadanie[1].toInt();
	Kpult->setStepSizes(Fstep, Bstep);

	ttt = l_File.readLine();
	//Читаем стартовую позицию
	while (ttt[0] == ';') {
		ttt = l_File.readLine();
	}
	qDebug() << ttt;
	l_String = QString::fromUtf8(ttt);
	bool ok;
	startPos = l_String.toInt(&ok);
	if (!ok) {
		return 2;
	}

	ttt = l_File.readLine();
	//Читаем границы
	while (ttt[0] == ';') {
		ttt = l_File.readLine();
	}
	qDebug() << ttt;
	l_String = QString::fromUtf8(ttt);
	l_String = l_String.simplified();
	zadanie = l_String.split(" ");
	if (zadanie.count() < 2) {
		return 3;
	}
	leftBorder = zadanie[0].toInt(&ok);
	if (!ok) {
		return 4;
	}
	rightBorder = zadanie[1].toInt(&ok);
	if (!ok) {
		return 4;
	}
	borderEnable = true;
	if (leftBorder < -1000) {
		borderEnable = false;
	}
	if (rightBorder > 1000) {
		borderEnable = false;
	}
	ttt = l_File.readLine();
	//Читаем флаги
	flags_pos.clear();
	while (ttt[0] == ';') {
		ttt = l_File.readLine();
	}

	qDebug() << ttt;
	l_String = QString::fromUtf8(ttt);
	l_String = l_String.simplified();
	zadanie = l_String.split(" ", QString::SkipEmptyParts);
	for (int i = 0; i < zadanie.count() - 1; i++) {
		qDebug() << "flag:" << zadanie[i];
		flags_pos.append(zadanie[i].toInt(&ok));
		if (!ok) {
			return 5;
		}
	}


	Reset();
	fileName = QFileInfo(l_File).fileName();
	MV->setWindowTitle(QString::fromUtf8("Кузнечик - ") + QFileInfo(l_File).fileName());
	createFlags();
	return 0;

}


float KumKuznec::GetMinX(void)
{
	qreal MinX = 1000000;
	qreal VecX1, VecX2;
	QLineF TmpLine;

	for (int i = 0; i < lines.count(); i++) {

		TmpLine = lines[i]->line();
		VecX1 = TmpLine.x1();
		VecX2 = TmpLine.x2();

		if (VecX1 < MinX) {
			MinX = VecX1;
		}
		if (VecX2 < MinX) {
			MinX = VecX2;
		}
	}
	return MinX;
}

float KumKuznec::GetMinY(void)
{
	qreal MinY = 1000000;
	qreal VecY1, VecY2;
	QLineF TmpLine;

	for (int i = 0; i < lines.count(); i++) {
		TmpLine = lines[i]->line();
		VecY1 = -TmpLine.y1();
		VecY2 = -TmpLine.y2();

		if (VecY1 < MinY) {
			MinY = VecY1;
		}
		if (VecY2 < MinY) {
			MinY = VecY2;
		}
	}

	return MinY;
}

float KumKuznec::GetMaxX(void)
{
	qreal MaxX = -1000000;
	qreal VecX1, VecX2;
	QLineF TmpLine;

	for (int i = 0; i < lines.count(); i++) {
		TmpLine = lines[i]->line();
		VecX1 = TmpLine.x1();
		VecX2 = TmpLine.x2();

		if (VecX1 > MaxX) {
			MaxX = VecX1;
		}
		if (VecX2 > MaxX) {
			MaxX = VecX2;
		}
	}
	return MaxX;
}

float KumKuznec::GetMaxY(void)
{
	qreal MaxY = -1000000;
	qreal VecY1, VecY2;
	QLineF TmpLine;

	for (int i = 0; i < lines.count(); i++) {
		TmpLine = lines[i]->line();
		VecY1 = -TmpLine.y1();
		VecY2 = -TmpLine.y2();

		if (VecY1 > MaxY) {
			MaxY = VecY1;
		}
		if (VecY2 > MaxY) {
			MaxY = VecY2;
		}
	}
	return MaxY;
}

//загоняет картинку в окно
void KumKuznec::ToWindow(void)
{
	qreal MinX = GetMinX();
	qreal MaxX = GetMaxX();
	qreal MinY = GetMinY();
	qreal MaxY = GetMaxY();

	for (int i = 0; i < kumtexts.count(); i++) {
		qreal TextX = kumtexts[i]->x;
		qreal TextY = kumtexts[i]->y;
		int len = kumtexts[i]->text.length();
		qreal TextScale = kumtexts[i]->Size;
		if (TextX < MinX) {
			MinX = TextX;
		}
		if (TextY < MinY) {
			MinY = TextY;
		}
		if (TextX + len * TextScale > MaxX) {
			MaxX = TextX + len * TextScale;
		}
		if (TextY + TextScale > MaxY) {
			MaxY = TextY + TextScale;
		}
	}

	qreal dx = MaxX - MinX, dy = MaxY - MinY;

	MinX -= dx * 0.1;
	MaxX += dx * 0.1;
	dx = MaxX - MinX;

	MinY -= dy * 0.1;
	MaxY += dy * 0.1;
	dy = MaxY - MinY;

	qreal zoom = WindowZoom;
	qreal XSdwig = 0;
	qreal YSdwig = 0;

	if ((dx < 0.00001) && (dy < 0.00001)) {
		return;
	}

	if (dx / SizeX > dy / SizeY) {
		zoom = SizeX / dx;
		YSdwig = 0.5 * (SizeY / zoom - dy);
	} else {
		zoom = SizeY / dy;
		XSdwig = 0.5 * (SizeX / zoom - dx);
	}

	qreal Scale = zoom / WindowZoom;

	WindowZoom = zoom;
	WindowX0 = MinX  - XSdwig;
	WindowY0 = -MaxY - YSdwig;
	view->scale(Scale, Scale);

	WindowRedraw();
}

void KumKuznec::ClearPicture(void)
{
	HideCoord();

	Dialog *newZdialog = new Dialog(MV);

	newZdialog->setSizes(Fstep, Bstep);

	newZdialog->setFlags(flags_pos);
	newZdialog->setStart(startPos);
	newZdialog->setLeftBorder(leftBorder);
	newZdialog->setRightBorder(rightBorder);
	newZdialog->setBordersChB(borderEnable);
	qDebug() << "Task:" << advancedTask;
	newZdialog->setTaskNeeded(advancedTask);
	if (!newZdialog->exec()) {
		return;
	}
	qDebug() << "Asize" << newZdialog->ASize();
	Kpult->setStepSizes(newZdialog->ASize(), newZdialog->BSize());
	Fstep = newZdialog->ASize();
	Bstep = newZdialog->BSize();
	startPos = newZdialog->getStart();
	flags_pos = newZdialog->getFlags();

	MV->setWindowTitle(QString::fromUtf8("Кузнечик - без имени"));
	for (int i = 0; i < lines.count(); i++) {
		delete lines[i];
	}
	lines.clear();

	for (int i = 0; i < texts.count(); i++) {
		delete texts[i];
	}
	texts.clear();

	for (int i = 0; i < kumtexts.count(); i++) {
		delete kumtexts[i];
	}
	kumtexts.clear();
	borderEnable = newZdialog->borders();
	if (borderEnable) {
		leftBorder = newZdialog->getLeftBorder();
		rightBorder = newZdialog->getRightBorder();
	}
	WindowRedraw();
	advancedTask = newZdialog->isTaskNeeded();
	CurX = 0;
	CurY = 0;
	mPen->setPos(startPos * ZOOMMULTIP, 0);
	CurX = ZOOMMULTIP * startPos;
	mPen->setBrush(QBrush(KuznecColor));
	createFlags();
	ClearSledi();
	ClearZakr();
	ToDubl();
	fileName = "";
	myModule->jumpsChanged();
}

void KumKuznec::CreatePen(void)
{
	static const int points[] = { 0, 0, -3, -6, 3, -6, 0, 0 };

	QPolygon polygon;
	polygon.setPoints(4, points);
	QPolygonF polygonf = QPolygonF(polygon);

	mPen = new QGraphicsPolygonItem();
	mPen->setPolygon(polygonf);
	mPen->setZValue(100);
	mPen->setBrush(QBrush(QColor(200, 200, 200)));

	scene->addItem(mPen);
	mPen->moveBy(0, -0.25);
}

void KumKuznec::SetLineColor(int LRed, int LGreen, int LBlue)
{

	CurColor.Red = LRed;
	CurColor.Green = LGreen;
	CurColor.Blue = LBlue;
	mPen->setBrush(QBrush(QColor(CurColor.Red, CurColor.Green, CurColor.Blue)));
}

bool KumKuznec::SetColorString(QString Color)
{
	CurColorName = Color;
	if (Color == QString::fromUtf8("черный")) {
		SetLineColor(1, 1, 1);
		return true;
	}
	if (Color == QString::fromUtf8("белый")) {
		SetLineColor(254, 254, 254);
		return true;
	}
	if (Color == QString::fromUtf8("красный")) {
		SetLineColor(254, 0, 0);
		return true;
	}
	if (Color == QString::fromUtf8("желтый")) {
		SetLineColor(254, 254, 0);
		return true;
	}
	if (Color == QString::fromUtf8("оранжевый")) {
		SetLineColor(254, 128, 0);
		return true;
	}
	if (Color == QString::fromUtf8("зеленый")) {
		SetLineColor(0, 254, 0);
		return true;
	}
	if (Color == QString::fromUtf8("голубой")) {
		SetLineColor(137, 175, 245);
		return true;
	}
	if (Color == QString::fromUtf8("синий")) {
		SetLineColor(0, 0, 254);
		return true;
	}
	if (Color == QString::fromUtf8("фиолетовый")) {
		SetLineColor(128, 0, 255);
		return true;
	}
	return false;
}

void KumKuznec::Createbtn11()
{
	btn11 = new QToolButton(MV);
	btn11->setIcon(QIcon(QString::fromUtf8(":/icons/media-skip-forward.png")));
	btn11->setIconSize(QSize(20, 20));
	btn11->setToolTip(trUtf8("Крупнее"));
	btn11->move(ButtonSdwig, 5);
	btn11->resize(BUTTON_SIZE, BUTTON_SIZE);
	btn11->show();
}

void KumKuznec::Createbtn12()
{
	btn12 = new QToolButton(MV);
	btn12->setIcon(QIcon(QString::fromUtf8(":/icons/document-open.png")));
	btn12->setIconSize(QSize(20, 20));
	btn12->setToolTip(trUtf8("Загрузить"));
	btn12->move(100 + BUTTON_SIZE, 5);
	btn12->resize(BUTTON_SIZE, BUTTON_SIZE);
	btn12->hide();
}

void KumKuznec::Createbtn13()
{
	btn13 = new QToolButton(MV);
	btn13->setIcon(QIcon(QString::fromUtf8(":/icons/media-skip-backward.png")));
	btn13->setIconSize(QSize(20, 20));
	btn13->setToolTip(trUtf8("Мельче"));
	btn13->move(ButtonSdwig + BUTTON_SIZE, 5);
	btn13->resize(BUTTON_SIZE, BUTTON_SIZE);
	btn13->hide();
}

void KumKuznec::ToCenter(void)
{
	qDebug() << "WZ" << WindowZoom << " BZ" << BASEZOOM;
	view->scale(BASEZOOM / WindowZoom, BASEZOOM / WindowZoom);
	view->centerOn(0, 0);
	WindowZoom = BASEZOOM;
}


// пересветка окна
void KumKuznec::WindowRedraw(void)
{
	qDebug() << "KumKuznec::WindowRedraw: WindowY0 " << WindowY0;
	DrawNet();
}

void KumKuznec::DrawNet()
{
	int i;
	for (i = 0; i < Netlines.count(); i++) {
		delete Netlines[i];
	}
	for (i = 0; i < NetText.count(); i++) {
		delete NetText[i];
	}
	Netlines.clear();
	NetText.clear();

// вертикальная сетка

	double BegX = -1024 * ZOOMMULTIP;
	double EndX = 1024 * ZOOMMULTIP;
	float Step = ZOOMMULTIP;

	double fx1 = BegX;

	while (fx1 < EndX) {
		fx1  += Step;
		double fx2 = fx1 + ZOOMMULTIP;

		Netlines.append(scene->addLine((fx1 - 0.2 * ZOOMMULTIP), 0.2 * ZOOMMULTIP, fx1 - 0.2 * ZOOMMULTIP, -0.2 * ZOOMMULTIP));
		Netlines.last()->setZValue(0.5);
		Netlines.last()->setPen(QPen(RulerColor)); // Левая вертикаль

		Netlines.append(scene->addLine((fx1 + 0.2 * ZOOMMULTIP), 0.2 * ZOOMMULTIP, fx1 + 0.2 * ZOOMMULTIP, -0.2 * ZOOMMULTIP));
		Netlines.last()->setZValue(0.5);
		Netlines.last()->setPen(QPen(RulerColor));//Правая вертикаль

		Netlines.append(scene->addLine((fx1 - 0.2 * ZOOMMULTIP), -0.2 * ZOOMMULTIP, fx1 + 0.2 * ZOOMMULTIP, -0.2 * ZOOMMULTIP));
		Netlines.last()->setZValue(0.5);
		Netlines.last()->setPen(QPen(RulerColor));//верх

		Netlines.append(scene->addLine((fx1 - 0.2 * ZOOMMULTIP), 0.2 * ZOOMMULTIP, fx1 + 0.2 * ZOOMMULTIP, 0.2 * ZOOMMULTIP));
		Netlines.last()->setZValue(0.5);
		Netlines.last()->setPen(QPen(RulerColor));//Низ

		Netlines.append(scene->addLine((fx1 + 0.2 * ZOOMMULTIP), 0, fx2 - 0.2 * ZOOMMULTIP, 0));
		Netlines.last()->setZValue(0.5);
		Netlines.last()->setPen(QPen(RulerColor));//соединялка

		if ((borderEnable) && ((fx1 / ZOOMMULTIP == rightBorder) || (fx1 / ZOOMMULTIP == leftBorder))) {
			qreal zdvig = Step / 2;
			if (fx1 / ZOOMMULTIP == leftBorder) {
				zdvig = -zdvig;
			}
			Netlines.append(scene->addLine(fx1 + zdvig, -16, fx1 + zdvig, 16));
			Netlines.last()->setZValue(0.5);
			Netlines.last()->setPen(QPen(QColor("red")));
		}
	}

//Оси координат
	QBrush textBrush(contrastColor(BackColor));
	qreal textX, textY;
	Q_UNUSED(textY);
	textX = BegX;
	while (textX < EndX) {
		NetText.append(new QGraphicsSimpleTextItem(QString::number(textX / ZOOMMULTIP)));

		qreal zdvig = -0.2 * ZOOMMULTIP;
		if ((textX / ZOOMMULTIP > 0.5) || (textX / ZOOMMULTIP < 9.5)) {
			zdvig += 0.1 * ZOOMMULTIP;
		}
		//if(textX<-9.5)zdvig+=0.1;
		if (QString::number(textX / ZOOMMULTIP).length() > 1) {
			zdvig = zdvig - 3;
		}
		if (QString::number(textX / ZOOMMULTIP).length() > 2) {
			zdvig = zdvig - 2;
		}
		NetText.last()->setBrush(textBrush);
		NetText.last()->setPos(textX + zdvig, 0.2 * ZOOMMULTIP);
		int Zoom = Step;

		//qDebug()<<((int)textX)%100;
		if ((((int)textX / ZOOMMULTIP) % 100) == 0) {
			//qDebug()<<"Text ZoomUP";
			Zoom = Zoom * 1.5;
		}

		NetText.last()->setScale(0.03 * Zoom);
		NetText.last()->setZValue(1000);

		scene->addItem(NetText.last());


		textX = textX + Step;
	}
}


void KumKuznec::ToDubl()
{
	QLineF TmpLine;
	QPen TmpPen;
	QGraphicsLineItem *TmpItem;
	for (int i = 0; i < linesDubl.count(); i++) {
		delete linesDubl[i];
	}

	linesDubl.clear();

	for (int i = 0; i < lines.count(); i++) {

		TmpLine = lines[i]->line();
		TmpPen = lines[i]->pen();
		//TmpItem = lines[i];
		TmpItem = new QGraphicsLineItem(TmpLine);
		TmpItem->setPen(TmpPen);
		linesDubl.append(TmpItem);
	}

	PenXDubl = CurX;
	PenYDubl = CurY;
	PenPositionDubl = PenPosition;
}

void KumKuznec::FromDubl()
{
	if (linesDubl.count() == 0) {
		//ClearPicture();
		return;
	}

	QLineF TmpLine;

	for (int i = 0; i < lines.count(); i++) {
		delete lines[i];
	}

	lines.clear();

	QGraphicsLineItem *TmpItem;
	QPen TmpPen;
	QColor TmpColor;

	for (int i = 0; i < linesDubl.count(); i++) {

		TmpItem = linesDubl[i];
		TmpLine = TmpItem->line();
		TmpPen = TmpItem->pen();
		TmpColor = TmpPen.color();
		lines.append(scene->addLine(TmpLine));
		lines.last()->setPen(QPen(TmpColor));
	}

	CurX = PenXDubl;
	CurY = PenYDubl;
	PenPosition = PenPositionDubl;
	mPen->setPos(CurX, -CurY);
}

void KumKuznec::Info()
{
	char ctmp[200];
	QString tmp;

	sprintf(ctmp, "%7.2f", WindowX0);
	tmp = QString(ctmp);
	lX0->setText(tmp);

	sprintf(ctmp, "%7.2f", -WindowY0 - SizeY / WindowZoom);
	tmp = QString(ctmp);
	lY0->setText(tmp);

	sprintf(ctmp, "%7.2f", WindowX0 + SizeX / WindowZoom);
	tmp = QString(ctmp);
	lX1->setText(tmp);

	sprintf(ctmp, "%7.2f", -WindowY0);
	tmp = QString(ctmp);
	lY1->setText(tmp);

	sprintf(ctmp, "%7.2f", WindowX0 + SizeX / WindowZoom / 2);
	tmp = QString(ctmp);
	lXCen->setText(tmp);

	sprintf(ctmp, "%7.2f", -WindowY0 - SizeY / WindowZoom / 2);
	tmp = QString(ctmp);
	lYCen->setText(tmp);

	sprintf(ctmp, "%7.2f", SdwigNetX);
	tmp = QString(ctmp);
	lXNetX0->setText(tmp);

	sprintf(ctmp, "%7.2f", SdwigNetY);
	tmp = QString(ctmp);
	lYNetY0->setText(tmp);

	sprintf(ctmp, "%7.2f", StepX);
	tmp = QString(ctmp);
	lXNetDX->setText("(" + tmp);

	sprintf(ctmp, "%7.2f", StepY);
	tmp = QString(ctmp);
	lYNetDY->setText(tmp + ")");

	QPalette Pall;
	Pall.setColor(QPalette::BrightText, QColor(CurColor.Red, CurColor.Green, CurColor.Blue));
	Pall.setBrush(QPalette::WindowText, QBrush(QColor(CurColor.Red, CurColor.Green, CurColor.Blue)));
	lColor->setPalette(Pall);

	tmp = QString::fromUtf8("Цвет ") + CurColorName;
	lColor->setText(tmp);

	if (PenPosition) {
		tmp = QString::fromUtf8("Перо опущено");
	} else {
		tmp = QString::fromUtf8("Перо поднято");
	}

	lPen->setText(tmp);
}

bool KumKuznec::PrintToPDF(void)
{
	return false;
}

void KumKuznec::MouseWheel(int Delta)
{
	qWarning("wheel %i", Delta);
	if (Delta > 0) {
		ZoomUp();
	} else {
		ZoomDown();
	}
}

void KumKuznec::SetWindowSize(int x1, int y1, int w, int h)
{
	SizeX = w;
	SizeY = h;
	MV->setGeometry(x1, y1, w, h);
	MV->setFixedSize(w, h);
	MV->resize(SizeX, SizeY + MenuHigth);
	MV->setFixedSize(SizeX, SizeY + MenuHigth);
	MV->setFixedHeight(SizeY + MenuHigth + 1);
	viewFrame->setGeometry(0, MenuHigth + 1, SizeX + 1, SizeY);

	view->setGeometry(0, 0, SizeX, SizeY);

	qDebug() << "Wx0" << WindowX0;

	view->show();

	WindowRedraw();
	qDebug() << "Redraw";
	this->setGeometry(MV->geometry());
}

void KumKuznec::CreateMenu(void)
{
	menu = MV->menuBar()->addMenu(QString::fromUtf8("&Задание"));

	QAction *ClearPicture = new QAction(QString::fromUtf8("&Новое"), MV);
	menu->addAction(ClearPicture);

	QAction *LoadPicture = new QAction(QString::fromUtf8("&Загрузить"), MV);
	menu->addAction(LoadPicture);

	QAction *SavePicture = new QAction(QString::fromUtf8("&Сохранить"), MV);
	menu->addAction(SavePicture);

	connect(ClearPicture, SIGNAL(triggered()), this, SLOT(ClearPicture()));
	connect(LoadPicture, SIGNAL(triggered()), this, SLOT(LoadFromFileActivated()));
	connect(SavePicture, SIGNAL(triggered()), this, SLOT(SaveToFileActivated()));
	MenuHigth =  MV->menuBar()->height() - 1;

	menu1 = MV->menuBar()->addMenu(QString::fromUtf8("&Вид"));
	QAction *UpAction = new QAction(QString::fromUtf8("&Крупнее"), MV);
	menu1->addAction(UpAction);
	connect(UpAction, SIGNAL(triggered()), this, SLOT(ZoomUp()));
	QAction *DownAction = new QAction(QString::fromUtf8("&Мельче"), MV);
	menu1->addAction(DownAction);
	connect(DownAction, SIGNAL(triggered()), this, SLOT(ZoomDown()));

	QAction *Home = new QAction(QString::fromUtf8("&Восстановить"), MV);
	menu1->addAction(Home);
	connect(Home, SIGNAL(triggered()), this, SLOT(ToCenter()));

	QAction *FindK = new QAction(QString::fromUtf8("&Найти кузнечика"), MV);
	menu1->addAction(FindK);
	connect(FindK, SIGNAL(triggered()), this, SLOT(FindKuznec()));
}

void KumKuznec:: HorizWindow()
{
}

void KumKuznec:: VertWindow()
{
}

void KumKuznec::StepXChanged(void)
{
	if (BoxSqware->isChecked()) {
		eStepY->setText(eStepX->displayText());
	}
}

void KumKuznec::StepYChanged(void)
{
	if (BoxSqware->isChecked()) {
		eStepX->setText(eStepY->displayText());
	}
}

void KumKuznec::NetCancelClick(void)
{
	NetWindow->close();
}

void KumKuznec::NetOKClick(void)
{
	if (BoxNet->isChecked()) {
		NetShowFlag = true;
	} else {
		NetShowFlag = false;
	}

	if (BoxSqware->isChecked()) {
		SqwareNetFlag = true;
	} else {
		SqwareNetFlag = false;
	}

	bool ok, ok1;

	StepX = eStepX->displayText().toFloat(&ok);
	StepY = eStepY->displayText().toFloat(&ok1);
	SdwigNetX = 0.;
	SdwigNetY = 0.;

	if (!(ok && ok1)) {
		QString tmp = QString::fromUtf8("Проверьте данные!");
		QMessageBox::warning(0, QString::fromUtf8("Ошибки ввода чисел"), tmp, 0, 0, 0);
		return;
	}

	NetWindow->close();
	DrawNet();
}


#if 0
void KumKuznec::DrawText(QString Text, qreal Scale)
{
	QString t = Text;
	KumKuznecText  *KText = new KumKuznecText();
	KText->x = CurX;
	KText->y = CurY;
	KText->text = t;
	KText->color.Red = CurColor.Red;
	KText->color.Green = CurColor.Green;
	KText->color.Blue = CurColor.Blue;
	KText->Size = Scale;

	QFont font("Courier", -1, -1,  false);

//QGraphicsTextItem *m_text = scene->addText(t,font);
	QGraphicsSimpleTextItem *m_text = scene->addSimpleText(t, font);
//m_text->setDefaultTextColor(QColor(CurColor.Red,CurCoizelor.Green,CurColor.Blue));
	m_text->setBrush(QColor(CurColor.Red, CurColor.Green, CurColor.Blue));
	m_text->setPen(QColor(CurColor.Red, CurColor.Green, CurColor.Blue));
	qreal x0 = CurX;
//- Scale*0.2; //Цифры подобраны экспериментально
	qreal y0 = -CurY - Scale * 1.4;

	m_text->setFont(font);
	texts.append(m_text);
	kumtexts.append(KText);
	QList<QGraphicsItem *> scene_items = scene->items();

//scene_items.last()->scale(0.1*Scale,0.1*Scale);
	scene_items.last()->setScale(0.1 * Scale);
	scene_items.last()->setPos(x0, y0);

	CurZ += 0.01;
	scene_items.last()->setZValue(CurZ);

	qreal TextWidth = t.length() * Scale;

	bool OldPenPosition = PenPosition;
	PenPosition = false;
	GoVector(TextWidth, 0);
	PenPosition = OldPenPosition;
	GoVector(0, 0);   // Чтобы цвет пера был правильным
	WindowRedraw();
}
#endif

void KumKuznec::InfoOKClick(void)
{
	InfoWindow->close();
}

void KumKuznec::CreateXYLabel(void)
{
	lXCur = new QLabel(QString::fromUtf8("X= "), MV, 0);
	lXCur->setGeometry(QRect(ButtonSdwig + 90, 0, 80, 20));
	lXCur->hide();

	lYCur = new QLabel(QString::fromUtf8("Y= "), MV, 0);
	lYCur->setGeometry(QRect(ButtonSdwig + 90, 12, 80, 20));
	lYCur->hide();
//qWarning("XYYYYYYYY");
}

void KumKuznec::ShowCoord(qreal xScene, qreal yScene)
{
	QString tmp;
	char ctmp[50];
	sprintf(ctmp, "X = %7.4f", xScene);
	tmp = QString(ctmp);
	lXCur->setText(tmp);
	sprintf(ctmp, "Y = %7.4f", yScene);
	tmp = QString(ctmp);
	lYCur->setText(tmp);

	QPalette PallBlue;
	PallBlue.setColor(QPalette::BrightText, QColor(1, 1, 1));
	PallBlue.setBrush(QPalette::WindowText, QBrush(QColor(1, 1, 1)));
	lXCur->setPalette(PallBlue);
	lYCur->setPalette(PallBlue);

	lXCur->show();
	lYCur->show();
}

void KumKuznec::HideCoord(void)
{
	lXCur->hide();
	lYCur->hide();
}

void KumKuznec::LoadFromFileActivated()
{
	QSettings sett("NIISI RAS", "Grasshopper");
	QString cd = sett.value("Last", curDir).toString();

	QString    VodFileN = QFileDialog::getOpenFileName(this, QString::fromUtf8("Открыть файл"), curDir, "(*.kz)");
	if (VodFileN.isEmpty()) {
		return;
	}

	sett.setValue("Last", VodFileN);
	if (LoadFromFile(VodFileN) > 0) {
		QMessageBox::information(0, "", trUtf8("Чтение записи: ") + VodFileN, 0, 0, 0);
		return;
	}
	myModule->jumpsChanged();
}

void KumKuznec::SaveToFileActivated()
{
	QSettings sett("NIISI RAS", "Grasshopper");
	QString cd = sett.value("Last", curDir).toString();
	QFileDialog dialog(MV, trUtf8("Сохранить файл"), curDir, "(*.kz)");
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	if (!dialog.exec()) {
		return;
	}
	QString VodFile = dialog.selectedFiles().first();
	if (VodFile.right(3) != ".kz") {
		VodFile += ".kz";
	}
	sett.setValue("Last", VodFile);
	QFile vFile(VodFile);
	if (!vFile.open(QIODevice::WriteOnly)) {
		QMessageBox::information(0, "", trUtf8("Ошибка записи: ") + VodFile, 0, 0, 0);
		return;
	};
	if (!borderEnable) {
		leftBorder = -1024;
	}
	if (!borderEnable) {
		rightBorder = 1024;
	}

	//Zapis v fayl
	vFile.write(";Step forward Step Back\n");
	vFile.write(QString::number(Fstep).toLatin1() + " " + QString::number(Bstep).toLatin1() + "\n");
	vFile.write(";StartPos\n");
	vFile.write(QString::number(startPos).toLatin1() + "\n");
	vFile.write(";Left border Right border\n");
	vFile.write(QString::number(leftBorder).toLatin1() + " " + QString::number(rightBorder).toLatin1() + "\n");
	vFile.write(";Flags\n");
	for (int i = 0; i < flags_pos.count(); i++) {
		vFile.write(QString::number(flags_pos[i]).toLatin1() + " ");
	}
	vFile.write("\n;END\n");
	fileName = QFileInfo(vFile).fileName();
	MV->setWindowTitle(QString::fromUtf8("Кузнечик - ") + QFileInfo(vFile).fileName());
}

void KumKuznec::MousePress(
	qreal x, qreal y,
	bool LeftButtonFlag,
	qreal xScene, qreal yScene
) {

	qWarning("Draw::MousePress %i", LeftButtonFlag);

	qDebug() << "Center" << center();
	if (LeftButtonFlag) {
		LeftMousePressFlag = true;
		OldX = x;
		OldY = y;
		MVxmin = MV->x() - 1000;
		MVxmax = MVxmin + MV->width() + 2000;
		MVymin = MV->y();
		MVymax = MVymin + MV->height();
		QApplication::setOverrideCursor(Qt::PointingHandCursor);
	}
}

void KumKuznec::MouseRelease(qreal x, qreal y, bool LeftButtonFlag)
{
	if (LeftButtonFlag) {
		LeftMousePressFlag = false;
		QApplication::restoreOverrideCursor();
	} else {
		RightMousePressFlag = false;
	}
}

void KumKuznec::MouseMove(int x, int y, bool LeftButtonFlag)
{
	if (moving) {
		return;
	}

	if (!LeftMousePressFlag) {
		return;
	}

	if ((fabs(x - OldX) < 5) && (fabs(y - OldY) < 5)) {
		qDebug() << "return 1";
		return;
	}

	WindowX0 = WindowX0 + (view->mapToScene(QPoint(OldX, OldY)).x() - view->mapToScene(QPoint(x, y)).x());

	qDebug() << "sceneSizeY()" << sceneSizeY();
	qDebug() << "XZdvig" << - (x - OldX) / WindowZoom;
	moving = true;
	view->centerOn(center().x() - (x - OldX) / WindowZoom, -1);
	scene->update();
	view->update();
	scene->invalidate();
	OldX = x;
	OldY = y;
	qDebug() << "WX0" << WindowX0;

	if (sledi.count() > 0) {
		sledi.last()->show();
	}
//qWarning("Draw::MouseMove %f %f",x,y);
	moving = false;
}

void KumKuznec::createFlags()
{
	qDebug() << "Create f";
	for (int i = 0; i < flags.count(); i++) {
		if (flags[i]->flagItem) {
			scene->removeItem(flags[i]->flagItem);
		}
	}

	flags.clear();
	for (int i = 0; i < flags_pos.count(); i++) {
		KuznFlag *curFlag = new KuznFlag();
		curFlag->flagItem = new QGraphicsPolygonItem(mFlag->polygon());
		curFlag->f_pos = flags_pos[i];
// curFlag->flagItem->scale(BASEZOOM*2,BASEZOOM*2);
		curFlag->flagItem->setScale(BASEZOOM * 2);
		curFlag->flagItem->setZValue(99);
		curFlag->flagItem->setBrush(QBrush(QColor(150, 100, 100)));
		curFlag->flagItem->setPen(QPen(contrastColor(BackColor)));
		curFlag->flagItem->setPos(curFlag->f_pos * ZOOMMULTIP, -ZOOMMULTIP);
		flags.append(curFlag);
		scene->addItem(curFlag->flagItem);
	}
}

void KumKuznec::ColorUncolor()
{
	int id = colored();

	if (id > -1) { //Убираем закраску
		if (!zakr[id]->zakraska) {
			qDebug() << "KumKuznec::ColorUncolor():No rect!";
			return;
		}
		scene->removeItem(zakr[id]->zakraska);
		zakr.removeAt(id);
		return;
	} else { //красим
		zakr.append(new KuznZakr());
		zakr.last()->zakraska = new QGraphicsRectItem(CurX - 0.2 * ZOOMMULTIP, -0.2 * ZOOMMULTIP, 0.4 * ZOOMMULTIP, 0.4 * ZOOMMULTIP);
		zakr.last()->zakraska->setBrush(QBrush(QColor(100, 100, 100)));
		zakr.last()->pos = CurX;
		scene->addItem(zakr.last()->zakraska);
	}
}

void KumKuznec::FindKuznec()
{
	qDebug() << "WZ:" << WindowZoom << "SizeX/Zoom" << (SizeX / 2) / WindowZoom;

	view->centerOn(CurX, WindowY0 - SizeY / 2);
	WindowX0 = CurX - SizeX / (WindowZoom * 2);
	WindowRedraw();
}

void KumKuznec::addSled(int start, int fin)
{
	int maxColor = 200;
	int delitel = sledi.count() + 1;
	int step = maxColor / delitel;
	sledi.append(new KuznSled(start, fin * ZOOMMULTIP));
	sledi.last()->setColor(TrackColor);
	scene->addItem(sledi.last());
	sledi.last()->moveBy(0, -1.1 * ZOOMMULTIP);
	sledi.last()->setZValue(1);
	for (int i = 0; i < sledi.count(); i++) {
		if (maxColor - step * (i + 1) > 150) {
			sledi[i]->hide();
		}
		sledi[i]->setInt(maxColor - step * (i + 1));

	};
	scene->update();
}

void KumKuznec::setColors(const QColor &back, const QColor &ruler, const QColor &kuznec, const QColor &track)
{
	BackColor = back;
	RulerColor = ruler;
	KuznecColor = kuznec;
	TrackColor = track;

	scene->setBackgroundBrush(QBrush(BackColor));
	DrawNet();
	mPen->setBrush(QBrush(KuznecColor));
	mPen->setPen(QPen(contrastColor(BackColor)));
	for (int i = 0; i < flags.count(); ++i) {
		flags[i]->flagItem->setPen(QPen(contrastColor(BackColor)));
	}
	for (int i = 0; i < sledi.count(); ++i) {
		sledi[i]->setColor(TrackColor);
	}
	scene->update();
}

void KumKuznec::closeEvent(QCloseEvent *event)
{
	qDebug() << "KUZNEC CLOSE CloseEvent";
	bool libMode = Kpult->getLibMode();
	if ((libMode) || (autoClose)) {
		qDebug() << "Lib mode: " << libMode << " autoClose: " << autoClose;
		close();
		event->accept();
		return;
	}

	int ret = QMessageBox::warning(this, QString::fromUtf8("Кузнечик"),
			QString::fromUtf8("Закрыть исполнитель кузнечик?"),
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {

		Kpult->AutoClose();
		Kpult->close();
		event->accept();
	} else {
		event->ignore();
	}
}

void KumKuznec::resizeSlot(QSize *oldSize, QSize *newSize)
{
	QSize newSize_p = *newSize;

	resizeEvent(new QResizeEvent(newSize_p, size()));
	FindKuznec();
};
