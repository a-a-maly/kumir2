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

#include "pult.h"
#include "turtlemodule.h"
#include <kumir2-libs/widgets/pultlogger.h>

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QMessageBox>
#include <QPen>

const double DegreesPerMinute = 7.0;


OvenTimer::OvenTimer(QWidget *parent) : QWidget(parent)
{
	// finishTime = QDateTime::currentDateTime();
	//    updateTimer = new QTimer(this);
	// connect(updateTimer, SIGNAL(timeout()), this, SLOT(update()));

	// finishTimer = new QTimer(this);
	// finishTimer->setSingleShot(true);
	//  connect(finishTimer, SIGNAL(timeout()), this, SIGNAL(timeout()));
	//   connect(finishTimer, SIGNAL(timeout()), updateTimer, SLOT(stop()));
	gradValue = 45;
	QFont font;
	font.setPointSize(8);
	setFont(font);
}

void OvenTimer::setDuration(int secs)
{
	gradValue = gradValue + secs;
	if (gradValue > 360) {
		gradValue = gradValue - 360;
	}
	if (gradValue < 0) {
		gradValue = 360 - gradValue;
	}
	emit angChange(gradValue);
	qDebug() << "Set " << secs;
}
int OvenTimer::duration() const
{

	return gradValue;
}
void OvenTimer::setValue(int value)
{

	gradValue = value;
	update();
}
void OvenTimer::mousePressEvent(QMouseEvent *event)
{
	mouseFlag = true;
	QPointF point = event->pos() - rect().center();
	old_mouse_pos = point;
	qDebug() << "Mouse pos" << point;
	update();
}
void OvenTimer:: mouseMoveEvent(QMouseEvent *event)
{
	if (!mouseFlag) {
		return;
	}
	QPointF point = event->pos() - rect().center();
	QPointF delta = old_mouse_pos - point;
	qDebug() << "Mouse Delta" << old_mouse_pos - point << " x coord" << point.x();
	if (point.x() > 0) {
		setDuration(-delta.y());
	} else {
		setDuration(delta.y());
	}

	old_mouse_pos = point;
	update();
};
void OvenTimer::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	mouseFlag = false;
};
void OvenTimer::paintEvent(QPaintEvent * /* event */)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	int side = qMin(width(), height());

	painter.setViewport((width() - side) / 2, (height() - side) / 2,
		side, side);
	painter.setWindow(-50, -50, 100, 100);

	draw(&painter);
}

void OvenTimer::draw(QPainter *painter)
{
	static const int triangle[3][2] = {
		{-2, -49}, {+2, -49}, {0, -47}
	};
	QPen thickPen(palette().foreground(), 1.5);
	QPen thinPen(palette().foreground(), 0.5);
	QColor niceBlue(200, 180, 130);
	QColor sand(220, 190, 150);
	QColor sandLight(244, 229, 111);
	painter->setPen(thinPen);
	painter->setBrush(palette().foreground());
	painter->drawPolygon(QPolygon(3, &triangle[0][0]));
	QConicalGradient coneGradient(0, 0, -90.0);
	coneGradient.setColorAt(0.0, sand);
	coneGradient.setColorAt(0.2, niceBlue);
	coneGradient.setColorAt(0.5, Qt::white);
	coneGradient.setColorAt(1.0, sand);

	painter->setBrush(coneGradient);
	painter->drawEllipse(-46, -46, 92, 92);
	QRadialGradient haloGradient(0, 0, 20, 0, 0);
	haloGradient.setColorAt(0.0, sandLight);
	haloGradient.setColorAt(0.8, sand);
	haloGradient.setColorAt(0.9, Qt::white);
	haloGradient.setColorAt(1.0, Qt::black);

	painter->setPen(Qt::NoPen);
	painter->setBrush(haloGradient);
	painter->drawEllipse(-20, -20, 40, 40);
	QLinearGradient knobGradient(-7, -25, 7, -25);
	knobGradient.setColorAt(0.0, Qt::black);
	knobGradient.setColorAt(0.2, niceBlue);
	knobGradient.setColorAt(0.3, sandLight);
	knobGradient.setColorAt(0.8, Qt::white);
	knobGradient.setColorAt(1.0, Qt::black);

	painter->rotate(duration());
	qDebug() << "Duration " << duration();
	painter->setBrush(knobGradient);
	painter->setPen(thinPen);
	painter->drawRoundRect(-7, -25, 14, 50, 99, 49);

	for (int i = 0; i <= 359; ++i) {
		if (i % 60 == 0) {
			painter->setPen(thickPen);
			painter->drawLine(0, -41, 0, -44);
			painter->drawText(-15, -41, 30, 30,
				Qt::AlignHCenter | Qt::AlignTop,
				QString::number(i));
		} else {
			if (i % 5 == 0) {
				painter->setPen(thinPen);
				painter->drawLine(0, -42, 0, -44);
			};
		}
		painter->rotate(-DegreesPerMinute);
	}
}



linkLight::linkLight(QWidget *parent): QWidget(parent)
{
	posX = 1;
	posY = 1;
	onLine = true;
	text = trUtf8("СВЯЗЬ");
}

void linkLight::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	QRectF rectangle(posX, posY, 10.0, 10.0);

	QPen pen(Qt::green);  // creates a default pen

	QBrush brush(Qt::green);
	if (!onLine) {
		brush.setColor(QColor(20, 60, 20));
	}
	brush.setStyle(Qt::SolidPattern);
	painter.setPen(pen);
	painter.setBrush(brush);
	painter.drawEllipse(rectangle);

	brush.setColor(QColor(10, 10, 10));
	painter.setPen(QPen(QColor(10, 10, 10)));
	painter.setBrush(brush);
	QFont font("Arial");
	font.setBold(true);
	painter.setFont(font);
	int cur_pos = 0;
	for (int i = 0; i < text.length(); i++) {
		painter.drawText(posX, posY + TEXT_STEP * i + 26, QString(text[i]));
		cur_pos = posY + TEXT_STEP * i + 26;
	};
	QRectF rectangle2(posX, cur_pos + 7, 10, 10);
	brush.setColor(QColor(30, 0, 0));
	if (!onLine) {
		brush.setColor(QColor(250, 50, 50));
	}
	pen.setColor(Qt::red);

	brush.setStyle(Qt::SolidPattern);
	painter.setPen(pen);
	painter.setBrush(brush);
	painter.drawEllipse(rectangle2);


};





TurtlePult::TurtlePult(QDir resdir, QWidget *parent, Qt::WindowFlags fl)
	: QWidget(parent, fl), Ui::TurtlePult()
{
	autoClose = false;
	setupUi(this);
	//setWindowFlags(Qt::CustomizeWindowHint|Qt::WindowSystemMenuHint|Qt::WindowMinimizeButtonHint);
	libMode = false;
	Logger = new pultLogger(resdir, this);

	GradInput = new OvenTimer(this);
	GradInput->setGeometry(gradProto->geometry());
	gradProto->hide();

	Logger->setSizes(164, 150);
	Logger->Move(40, 5);
	greenLight = new linkLight(this);
	greenLight->move(15, 29);
	greenLight->resize(12, 104);

	UpB->hide();
	buttFwd = new MainButton(resdir, this);
	buttFwd->move(UpB->pos());
	buttFwd->setGeometry(UpB->geometry());
	buttFwd->setText("  ");
	buttFwd->loadIcon(resdir.absoluteFilePath("vpered.png"));

	DownB->hide();
	buttBack = new MainButton(resdir, this);
	buttBack->move(DownB->pos());
	buttBack->setGeometry(DownB->geometry());
	buttBack->setText("  ");
	buttBack->loadIcon(resdir.absoluteFilePath("nazad.png"));

	LeftB->hide();
	turnLeft = new MainButton(resdir, this);
	turnLeft->move(LeftB->pos());
	turnLeft->setGeometry(RightB->geometry());
	turnLeft->setText("  ");
	turnLeft->loadIcon(resdir.absoluteFilePath("vlevo.png"));

	RightB->hide();
	turnRight = new MainButton(resdir, this);
	turnRight->move(RightB->pos());
	turnRight->setGeometry(LeftB->geometry());
	turnRight->loadIcon(resdir.absoluteFilePath("vpravo.png"));
	turnRight->setText("  ");

	//        StenaB->hide();
	askStena = new MainButton(resdir, this);
	//        askStena->move(StenaB->pos());
	askStena->setCheckable(true);
	askStena->setText("  ");
	askStena->loadIcon(resdir.absoluteFilePath("stena.png"));
	askStena->setCheckable(true);
	askStena->hide();
	//        SvobodnoB->hide();
	askFree = new MainButton(resdir, this);
	//        askFree->move(SvobodnoB->pos());
	askFree->setCheckable(true);
	askFree->setText(trUtf8(" "));
	askFree->loadIcon(resdir.absoluteFilePath("svobodno.png"));
	askFree->setCheckable(true);
	askFree->hide();

	RadB->hide();
	buttRad = new MainButton(resdir, this);
	buttRad->setGeometry(RadB->geometry());
	buttRad->setText(trUtf8(" "));
	if (!buttRad->loadIcon(resdir.absoluteFilePath("tailup.png"))) {
		qWarning("Image not loaded!");
	}
	TempB->hide();

	buttTemp = new MainButton(resdir, this);
	buttTemp->setText("  ");
	buttTemp->setGeometry(TempB->geometry());

	buttTemp->setText(" ");
	buttTemp->loadIcon(resdir.absoluteFilePath("taildown.png"));
	QIcon toKumirIco(resdir.absoluteFilePath("kumir.png"));
	toKumir->setIcon(toKumirIco);

	//  CenterB->setIcon(QIcon(":/icons/robo_field.png"));
	connect(buttFwd, SIGNAL(clicked()), this, SLOT(Up()));
	connect(GradInput, SIGNAL(angChange(int)), gradVal, SLOT(setValue(int)));
	connect(gradVal, SIGNAL(valueChanged(int)), GradInput, SLOT(setValue(int)));
	connect(buttBack, SIGNAL(clicked()), this, SLOT(Down()));
	connect(turnLeft, SIGNAL(clicked()), this, SLOT(Left()));
	connect(turnRight, SIGNAL(clicked()), this, SLOT(Right()));

	connect(buttRad, SIGNAL(clicked()), this, SLOT(RadS()));
	connect(buttTemp, SIGNAL(clicked()), this, SLOT(TempS()));

	connect(askStena, SIGNAL(clicked()), this, SLOT(SwStena()));
	connect(askFree, SIGNAL(clicked()), this, SLOT(SwSvobodno()));
	//connect(CenterB,SIGNAL(clicked()),this,SLOT(CenterButton()));

	connect(ClearLog, SIGNAL(clicked()), Logger, SLOT(ClearLog()));
	connect(ClearLog, SIGNAL(clicked()), this, SLOT(resetTurtle()));

	connect(toKumir, SIGNAL(clicked()), this, SLOT(logToKumir()));

	//  connect(CopyLog,SIGNAL(clicked()),Logger,SLOT(CopyLog()));
	//setMinimumSize(100,200);
	link = true;
	toKumir->setEnabled(true);
};
void TurtlePult::connectTurtle()
{
	if (turtleObj != 0) {

	}
}

void TurtlePult::noLink()
{
	link = false;
	greenLight->setLink(link);
	greenLight->repaint();
	qWarning("NoLINK");
	ClearLog->setEnabled(false);
};
void TurtlePult::LinkOK()
{
	qDebug() << "LinkOK";
	link = true;
	greenLight->setLink(link);
	greenLight->repaint();
	ClearLog->setEnabled(true);
};

void TurtlePult::Up()
{
	if (!greenLight->link()) {
		return;
	}



	turtleObj->runForward(stepVal->value());
	Logger->appendText(QString::fromUtf8("вперед(") + QString::number(stepVal->value()) + ")\n", QString::fromUtf8("вперед(") + QString::number(stepVal->value()) + ")", "OK");
	turtleObj->updateTurtle();


};
void TurtlePult::Down()
{
	if (!greenLight->link()) {
		return;
	}

	turtleObj->runBack(stepVal->value());

	Logger->appendText(QString::fromUtf8("назад(") + QString::number(stepVal->value()) + ")\n", QString::fromUtf8("назад(") + QString::number(stepVal->value()) + ")", "OK");
	turtleObj->updateTurtle();

};
void TurtlePult::Left()
{
	turtleObj->runLeft(gradVal->value());
	Logger->appendText(QString::fromUtf8("влево(") + QString::number(gradVal->value()) + ")\n", QString::fromUtf8("влево(") + QString::number(gradVal->value()) + ")", "OK");
	turtleObj->updateTurtle();

};
void TurtlePult::Right()
{
	if (!greenLight->link()) {
		return;
	}
	turtleObj->runRight(gradVal->value());
	Logger->appendText(QString::fromUtf8("вправо(") + QString::number(gradVal->value()) + ")\n", QString::fromUtf8("вправо(") + QString::number(gradVal->value()) + ")", "OK");

	turtleObj->updateTurtle();
};

void TurtlePult::SwStena()
{
	//
	//  if(!greenLight->link())return;
//   turtleObj->runTailDown();

	// if(askFree->isChecked () ) askFree->setChecked(false);
};
void TurtlePult::SwSvobodno()
{
	//  if(!greenLight->link())return;
	//  turtleObj->runTailUp();
};
void TurtlePult::TempS()
{
	if (!greenLight->link()) {
		return;
	}
	turtleObj->runTailDown();
	Logger->appendText(QString::fromUtf8("опустить хвост\n"), QString::fromUtf8("опустить хвост"), "OK");

};
void TurtlePult::RadS()
{
	if (!greenLight->link()) {
		return;
	}
	turtleObj->runTailUp();
	Logger->appendText(QString::fromUtf8("поднять хвост\n"), QString::fromUtf8("поднять хвост"), "OK");
	turtleObj->getCurView()->forceRedraw();
};

void TurtlePult::resetTurtle()
{
	turtleObj->reset();
};
void TurtlePult::newClient(QString message)
{
	label->setText(QString::fromUtf8("Подключился ") + message);
	toKumir->setEnabled(true);
}

void TurtlePult::clientDisconnect()
{
	label->setText(QString::fromUtf8("Клиент отключился"));
	// toKumir->setEnabled(false);
	LinkOK();
};


void TurtlePult::logToKumir()
{
	Logger->CopyLog();
	emit sendText(Logger->log());
};


void TurtlePult::closeEvent(QCloseEvent *event)
{

	if ((libMode) || (autoClose)) {
		close();
		event->accept();
		return;
	};
	int ret = QMessageBox::warning(this, QString::fromUtf8("Черепаха"),
			QString::fromUtf8("Закрыть исполнитель черепаха?"),
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {
//		turtleObj->AutoClose();
//		turtleObj->close();
		event->accept();
	} else {
		event->ignore();
	}
};
void TurtlePult::paintEvent(QPaintEvent *event)
{
	QPainter p(this);
	p.save();
	p.setPen(Qt::NoPen);
	p.setBrush(QColor("#D8C78F"));
	p.drawRect(0, 0, width(), height());
	p.restore();
	QWidget::paintEvent(event);
}
