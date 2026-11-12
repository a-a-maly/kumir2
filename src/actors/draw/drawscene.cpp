#include "drawscene.h"
#include "drawmodule.h"

#include <QGraphicsItem>
#include <QMessageBox>
#include <QDebug>

#define NET_RESERVE 15

namespace ActorDraw {

DrawScene::DrawScene(DrawModule *draw, QMutex *_mutex, QObject *parent) : QGraphicsScene(parent)
{
	DRAW = draw;
	mutex = _mutex;
}

DrawScene::~DrawScene()
{
	DRAW = 0;
	mutex = 0;
}

void DrawScene::reset()
{
	for (int i = 0; i < lines.count(); i++) {
		removeItem(lines.at(i));
	}
	lines.clear();

	for (int i = 0; i < texts.count(); i++) {
		removeItem(texts.at(i));
	}
	texts.clear();
	itemsBuffer.clear();
}

void DrawScene::fromBufferToScene()
{
	QGraphicsItemGroup *buff = createItemGroup(itemsBuffer);
	buff->setZValue(90);
	addItem(buff);
	itemsBuffer.clear();
}

//User lines and texts bounding rect
QRectF DrawScene::getRect() const
{
	if (lines.isEmpty() && texts.isEmpty()) {
		return QRectF(-5, -5, 10, 10);
	}
	QRectF boundRect;
	for (int i = 0; i < lines.count(); i++) {
		const QLineF line = lines.at(i)->line();
		boundRect |= QRectF(line.p1(), line.p2()).normalized();
	}
	for (int i = 0; i < texts.count(); i++) {
		boundRect |= texts.at(i)->sceneBoundingRect();
	}
	return boundRect;
}

bool DrawScene::isUserLine(QGraphicsItem *obj) const
{
	for (int i = 0; i < lines.count(); i++) {
		if (lines.at(i) == obj) {
			return true;
		}
	}
	return false;
}

bool DrawScene::isLineAt(const QPointF &pos, qreal radius)
{
	QGraphicsEllipseItem *testCirc = addEllipse(QRectF(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2));
	QList<QGraphicsItem *> colItems = collidingItems(testCirc);
	removeItem(testCirc);
	delete testCirc;

	for (int i = 0; i < colItems.count(); i++) {

		if (isUserLine(colItems.at(i))) {
			return true;
		}

	}
	return false;
}

qreal DrawScene::drawText(const QString &Text, qreal widthChar, QPointF from, QColor color)
{
	QFont font("Courier", 12);
	font.setPointSizeF(KUM_MULTI);
	QFontMetricsF fontMetric(font);
	qreal bs = fontMetric.boundingRect(Text).width();
	qreal psizeF = widthChar / bs;

	//  font.setPointSizeF(psizeF*inc);
	bs = fontMetric.boundingRect("OOOXX").width() / 5;

	fontMetric = QFontMetricsF(font);
	qDebug() << "Char Size:" << fontMetric.boundingRect("OOOXX").width() / 5000;

	texts.append(addSimpleText(Text, font));
//        texts.last()->scale(0.001,0.001);
	texts.last()->setScale(psizeF);
	texts.last()->setPos(from.x(), from.y() - (fontMetric.boundingRect(Text).height()*psizeF));
	texts.last()->setPen(QPen(color));
	texts.last()->setZValue(90);
	return widthChar;
};
void DrawScene::addDrawLine(QLineF lineF, QColor color, qreal width)
{
	if (lineF.length() == 0) {
		return;
	}
	QGraphicsLineItem *line = new QGraphicsLineItem(lineF);
	QPen mp = QPen(QColor(color));
	mp.setWidthF(width);
	mp.setCosmetic(true);
	line->setPen(mp);
	line->setZValue(90);
	lines.append(line);
	itemsBuffer.append(line);


}

void DrawScene::DestroyNet()
{
	//  qDebug()<<Netlines.count();
	for (int i = 0; i < Netlines.count(); i++) {

		delete Netlines[i];
	}
	Netlines.clear();

}

void DrawScene::drawOnlyAxis(double startx, double endx, double starty, double endy, qreal aw)
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



void DrawScene::drawNet(
	double startx, double endx, double starty, double endy,
	QColor color,
	double stepX, double stepY,
	bool net,
	qreal nw, qreal aw
) {

	QColor AxisColor = DRAW->axisColor();
	auto lp = QPen(color);
	lp.setWidthF(nw);
	lp.setCosmetic(true);

	DestroyNet();
	if (!net) {
		drawOnlyAxis(startx, endx, starty, endy, aw);
		return;
	}
	int lines = qRound(startx / stepX);
	startx = lines * stepX;
	double fx1 = startx - NET_RESERVE * stepX, fx2, fy1, fy2;

	while (fx1 < endx + NET_RESERVE * stepX) {
		fx1 = fx1 + stepX;
		fx2 = fx1;
		fy1 = starty - NET_RESERVE * stepX;
		fy2 = endy + NET_RESERVE * stepX;

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
	fy1 = starty - NET_RESERVE * stepY;

	while (fy1 < endy + NET_RESERVE * stepY) {
		fy1 = fy1 + stepY;
		fy2 = fy1;
		fx1 = startx - NET_RESERVE * stepY;
		fx2 = endx + NET_RESERVE * stepY;

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
}


int   DrawScene::loadFromFile(const QString &p_FileName)
{
	QFileInfo fi(p_FileName);
	QString name = fi.fileName();
	QString Title = QString::fromUtf8("Чертежник - ") + name;
	//  MV->setWindowTitle ( Title);

	QString tmp = "";
	char ctmp[200];
	QString l_String;
	QFile l_File(p_FileName);
	QColor CurColor;


	int NStrok = 0;

	qreal CurrentScale = 1.0;
	qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	if (!l_File.open(QIODevice::ReadOnly)) {
		QMessageBox::information(NULL, "", QString::fromUtf8("Ошибка открытия файла"), 0, 0, 0);
		return 1;
	}

	QByteArray ttt = l_File.readLine();
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
	if (!(l_List[1] == "Cherteznik")) {
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

	// koordinaty vektorov
	// CurZ = 1.;
	while (!l_File.atEnd()) {
		//считываем цвет

		//  l_String = l_File.readLine();
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
			CurrentScale = x1 = l_List[2].toFloat();
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
			x2 = l_List[0].toFloat();
			y2 = -l_List[1].toFloat();
			lines.append(addLine(x1, y1, x2, y2));
			lines.last()->setZValue(10);
			lines.last()->setPen(QPen(CurColor));
			continue;
		}

		sprintf(ctmp, "%4i", NStrok);
		tmp = QString::fromUtf8("Ошибка в строке ");
		tmp.append(QString(ctmp));

		break;
	}

	l_File.close();
	return 0;
}


int DrawScene::saveToFile(const QString &p_FileName) const
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

	qreal MinX = 1000000, MaxX = -1000000, MinY = 1000000, MaxY = -1000000;

	for (int i = 0; i < lines.count(); i++) {
		QLineF TmpLine = lines[i]->line();
		qreal VecX1 =  TmpLine.x1();
		qreal VecY1 = -TmpLine.y1();
		qreal VecX2 =  TmpLine.x2();
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

	if (MaxX <= MinX) {
		MaxX = MinX + 1;
	}
	if (MaxY <= MinY) {
		MaxY = MinY + 1;
	}

	double scaleX = (596 - 10) / (MaxX - MinX);
	double scaleY = (842 - 10) / (MaxY - MinY);
	double Scale = scaleX <= scaleY ? scaleX : scaleY;
	Scale = Scale * 0.9;

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
		QColor c = lines[i]->pen().color();
		sprintf(ctmp, "%i %i %i setrgbcolor\n", c.red(),  c.green(), c.blue());
		l_File.write(ctmp);

		qreal x1 =  TmpLine.x1();
		qreal y1 = -TmpLine.y1();
		qreal x2 =  TmpLine.x2();
		qreal y2 = -TmpLine.y2();

		sprintf(ctmp, "%f %f moveto\n", x1, y1);
		l_File.write(ctmp);
		sprintf(ctmp, "%f %f lineto\n", x2, y2);
		l_File.write(ctmp);
	}

	for (int i = 0; i < texts.count(); i++) {
		qreal FontSize = texts[i]->font().pointSizeF();
		sprintf(ctmp, "/Curier findfont %f scalefont setfont\n", FontSize);
		l_File.write(ctmp);

		qreal x = texts[i]->pos().x();
		qreal y = texts[i]->pos().y();
		sprintf(ctmp, "%f %f moveto\n", x, y);
		l_File.write(ctmp);

		QColor c = texts[i]->pen().color();
		sprintf(ctmp, "%i %i %i setrgbcolor\n", c.red(), c.green(), c.blue());
		l_File.write(ctmp);

		QString text = "(" + texts[i]->text() + ") show\n";
		l_File.write(text.toUtf8());
	}

	l_File.write("stroke\n");
	l_File.write("grestore\n");
	l_File.write("showpage\n");
	l_File.close();
	return 0;
}

} // namespace ActorDraw
