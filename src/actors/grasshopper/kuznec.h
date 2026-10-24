
/****************************************************************************
**
** Copyright (C) 2004-2008 NIISI RAS. All rights reserved.
**
** This file is part of the KuMir.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef KUZNEC_H
#define KUZNEC_H
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QDebug>

class QLineEdit;
class QToolButton;
class QCheckBox;
class QLabel;
class QCloseEvent;
class QDir;

class pultLogger;
class GrasshopperPult;
namespace ActorGrasshopper
{
	class GrasshopperModule;
};

#define DEFAULT_SIZEX 500
#define DEFAULT_SIZEY 260
#define BUTTON_SIZE 20
#define YCENTER -20
#define BASEZOOM 2
#define ZOOMMULTIP 20

//--------------------------------------------------------
class KuznSled: public QGraphicsItem
{
public:
	KuznSled(int start, int fin);
	~KuznSled() {};

	QRectF boundingRect() const
	{
		return QRectF(St, -ZOOMMULTIP, Fn, ZOOMMULTIP);
	}

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
		QWidget *widget);

	void  setStartFin(int start, int fin)
	{
		St = start;
		Fn = fin;
	}

	QPoint start_Fin()
	{
		return QPoint(St, Fn);
	}

	void setInt(int Int)
	{
		Intens = Int;
		update();
	}

	void setColor(const QColor &color)
	{
		Color = color;
		update();
	}

private:
	int St, Fn;
	int Intens;
	QColor Color;
};

//_____________________________________________________
class KumScene: public QGraphicsScene
{
	Q_OBJECT
public:
	KumScene(QWidget *parent = 0);

public:
signals:
	void MousePress(qreal x, qreal y, bool Flag, qreal xScene, qreal yScene);
	void MouseRelease(qreal x, qreal y, bool Flag);
	void MouseMove(int x, int y, bool Flag);
	void MouseWheel(int Delta);

private:
	void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
	void wheelEvent(QGraphicsSceneWheelEvent *wheelEvent);
};

class DrawView : public QGraphicsView
{
public:
	DrawView(QGraphicsScene *, QWidget *);
protected:
	void scrollContentsBy(int dx, int dy);
};

struct KuznFlag {
	QGraphicsPolygonItem *flagItem;
	int f_pos;
};

struct KuznZakr {
	QGraphicsRectItem *zakraska;
	int pos;
};


class KumKuznec : public QMainWindow
{
	friend ActorGrasshopper::GrasshopperModule;
	Q_OBJECT
public:

	KumKuznec(const QDir &mresd);

	void PenDown();
	void PenUp();

	int GoVector(double x, double y);
	int GoPoint(double x, double y);

	void Reset()
	{
		qDebug() << "Reset";
		GoPoint(startPos * ZOOMMULTIP, -0.25);
		createFlags();
		ClearSledi();
		ClearZakr();
		scene->update();
		//   MV->show();
	}

	void ClearZakr()
	{
		for (int i = 0; i < zakr.count(); i++) {
			if (!zakr[i]->zakraska) {
				qDebug() << "KumKuznec::ClearZakr():No rect!";
				return;
			}
			scene->removeItem(zakr[i]->zakraska);
		}
		zakr.clear();
	}

	void closeEvent(QCloseEvent *event);
	void SetWindowSize(int x1, int y1, int x2, int y2);

	//void DrawText(QString Text, qreal Scale);

	bool init();
	bool isReady() const
	{
		return ready;
	}

	void setMode(bool EditMode)
	{
		isEditor = EditMode;
	}

	void reset()
	{
		Reset();
	}

	void setColors(const QColor &back, const QColor &ruler, const QColor &kuznec, const QColor &track);

	void ClearSledi()
	{
		for (int i = 0; i < sledi.count(); i++) {
			scene->removeItem(sledi[i]);
		}
		sledi.clear();
	}

public slots:
	void setSteps(int fwd, int backwd)
	{
		Fstep = fwd;
		Bstep = backwd;
	}

	void setFwdStep(int fwd)
	{
		Fstep = fwd;
	}

	void setBackStep(int bwd)
	{
		Bstep = bwd;
	}

	int stepForward()
	{
		return Fstep;
	}

	int stepBackward()
	{
		return Bstep;
	}

	void resizeSlot(QSize *oldSize, QSize *newSize);
	void lockControls();
	void unlockControls();

	void ClearPicture();
	void ToDubl();
	void FromDubl();
	void ToWindow(void);
	void MoveLeft(void);
	void MoveRight(void);
	void MoveUp(void);
	void MoveDown(void);
	void ZoomUp(void);
	void ZoomDown(void);
	bool PrintToPDF(void);
	void Close();
	void ToCenter(void);
	void FindKuznec();
	void Info();
	void MousePress(qreal x, qreal y, bool Flag, qreal xScene, qreal yScene);
	void MouseRelease(qreal x, qreal y, bool Flag);
	void MouseMove(int x, int y, bool Flag);
	void MouseWheel(int Delta);
	void HorizWindow();
	void VertWindow();

	void StepXChanged(void);
	void StepYChanged(void);
	void NetCancelClick(void);
	void NetOKClick(void);
	void InfoOKClick(void);
	void LoadFromFileActivated();

	void SaveToFileActivated();

	void CloseInst(void)
	{
		MV->close();
		//delete MV;
	}

	void SwitchMode(int mode)
	{

		if ((mode == 1) && (curMode != 1)) {
			Reset();
		}
		curMode = mode;
		emit sync();
	}

	void showHideWindow(bool show = true); //Show - true

	void setWindowPos(int x, int y)
	{
		MV->move(x, y);
	}

	void getWindowPos(int *x, int *y)
	{
		*x = MV->pos().x();
		*y = MV->pos().y();

	}

	QRect getWindowGeometry()
	{
		return MV->geometry();
	}

	void MoveFwd()
	{
		addSled(CurX, Fstep);
		GoVector(Fstep * ZOOMMULTIP, 0);
		redrawFlags();
	}

	bool canFwd()
	{
		if (!borderEnable) {
			return true;
		}
		qDebug() << "RightB*Z:" << rightBorder *ZOOMMULTIP << " CurX" << CurX << " StepF*Z" << Fstep *ZOOMMULTIP;
		if (CurX + Fstep * ZOOMMULTIP > rightBorder * ZOOMMULTIP) {
			return false;
		}
		return true;
	}

	void MoveBack()
	{
		addSled(CurX, -Bstep);

		GoVector(-Bstep * ZOOMMULTIP, 0);
		qDebug() << "Move Back";
		redrawFlags();
	}

	QWidget *mainWindow()
	{
		return view;
	}

	bool canBack()
	{
		if (!borderEnable) {
			return true;
		}
		qDebug() << "LeftB*Z:" << leftBorder *ZOOMMULTIP << " CurX" << CurX << " StepB*Z" << Bstep *ZOOMMULTIP;
		if (CurX - Bstep * ZOOMMULTIP < leftBorder * ZOOMMULTIP) {
			return false;
		}
		return true;
	}

	void createFlags();
	void redrawFlags()
	{
		qDebug() << "F count" << flags.count();
		for (int i = 0; i < flags.count(); i++) {
			qDebug() << "F pos" << flags[i]->f_pos << "CurX" << CurX;
			if (CurX / ZOOMMULTIP == flags[i]->f_pos) {
				flags[i]->flagItem->setBrush(QBrush(QColor(100, 150, 100)));
				qDebug() << "FINISH";
			}
		}
	}

	int colored() //Возвращает номер квадратика в списке
	{
		for (int i = 0; i < zakr.count(); i++)
			if (zakr[i]->pos == CurX) {
				return i;
			}
		return -1;
	}

	void ColorUncolor();
	void addSled(int start, int fin);

	void AutoClose()
	{
		autoClose = true;
	}

signals:
	void sync();


protected:
	void resizeEvent(QResizeEvent *event);

private:
	QPointF center()
	{
		return QPointF(
			visibleRect().x() + visibleRect().width() / 2,
			visibleRect().y() + visibleRect().height() / 2
		);
	}

	QRectF visibleRect() //IN SCENE COORD
	{
		return view->mapToScene(view->viewport()->rect()).boundingRect();
	}

	qreal sceneSizeY() //IN SCENE COORD
	{
		return visibleRect().height();
	}

	struct LColor {
		int Red;
		int Green;
		int Blue;
	};

	struct KumKuznecText {
		qreal x;
		qreal y;
		QString text;
		LColor color;
		qreal Size;
	};

	void Createbtn11();
	void Createbtn12();
	void Createbtn13();

	void CreateXYLabel();
	void ShowCoord(qreal xScene, qreal yScene);
	void HideCoord();
	void WindowRedraw();
	void DrawNet();
	float GetMinX(void);
	float GetMinY(void);
	float GetMaxX(void);
	float GetMaxY(void);

	void SetLineColor(int LRed, int LGreen, int LBlue);
	bool SetColorString(QString Color);

	void CreatePen(void);

	int LoadFromFile(QString p_FileName);
	int SaveToFile(QString p_FileName);

	void CreateMenu(void);
	void CreateNetWindow(void);
	void CreateInfoWindow(void);

private:
	qreal step;
	pultLogger *logger;
	int Fstep, Bstep, Finish;
	GrasshopperPult *Kpult;

	ActorGrasshopper::GrasshopperModule *myModule;

	int SizeX, SizeY; //размеры окна в пикселах
	QFrame *viewFrame;
	QMainWindow *MV;
	QMenu *menu;
	QMenu *menu1;
	KumScene *scene;
	uint MenuHigth;
	QGraphicsScene *Panel;
	QGraphicsView *view;

	QList<QGraphicsLineItem *> lines;
	QList<QGraphicsLineItem *> Netlines;
	QList<QGraphicsSimpleTextItem *> NetText;

	QList<QGraphicsLineItem *> linesDubl;

	QList<QGraphicsSimpleTextItem *> texts;
	QList<KumKuznecText *> kumtexts;
	QList<KuznSled *> sledi;

	qreal PenXDubl;
	qreal PenYDubl;
	bool PenPositionDubl;
	QGraphicsPolygonItem *mPen;
	QGraphicsPolygonItem *mFlag;
	QList<KuznFlag *> flags;
	QList<KuznZakr *> zakr;

	QList<int> flags_pos;
	QGraphicsLineItem  *mFlagLine;

	double WindowX0;
	double WindowY0;
	double WindowZoom;

	QLineEdit *eX0;
	QLineEdit *eY0;
	QLineEdit *eStepX;
	QLineEdit *eStepY;
	QCheckBox *BoxNet;
	QLabel *lX0, *lY0, *lX1, *lY1, *lXCen, *lYCen, *lXNetX0, *lYNetY0, *lXNetDX, *lYNetDY, *lXCur, *lYCur, *lColor, *lPen ;

	qreal StepX; //шаг сетки
	qreal StepY;
	bool SqwareNetFlag;
	qreal SdwigNetX; //сдвиг сетки
	qreal SdwigNetY;
	bool NetShowFlag;
	bool TmpNetShowFlag;

	int NLastVec;
	LColor CurColor;
	QString CurColorName;
	QColor BackColor;
	QColor RulerColor;
	QColor KuznecColor;
	QColor TrackColor;
	double CurX;
	double CurY;
	double CurZ;
	bool PenPosition;
	bool isEditor;
	bool ready;

	qreal MinInfinityX;
	qreal MaxInfinityX;
	qreal MinInfinityY;
	qreal MaxInfinityY;
	qreal MaxZoom;
	qreal MinZoom;
	qreal ButtonSdwig;
	int WindowSizeX;
	int WindowSizeY;
	int MVxmin, MVxmax, MVymin, MVymax;
	QMainWindow *NetWindow;
	QMainWindow *InfoWindow;
	QCheckBox *BoxSqware;

	QToolButton *btn11;
	QToolButton *btn12;
	QToolButton *btn13;

	QString curDir;
	qreal OldX, OldY;

	bool LeftMousePressFlag;
	bool RightMousePressFlag;

	bool moving;

	int curMode;
	int startPos;
	int leftBorder, rightBorder; //Ограничения поля Кузнечика
	bool borderEnable; //Ограничения поля Кузнечика
	QString fileName;
	bool advancedTask;//Наличие в задание чегонить крпме длины прыжка
	bool autoClose;
};

#endif
