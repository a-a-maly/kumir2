//****************************************************************************
//**
//** Copyright (C) 2004-2008 IMPB RAS. All rights reserved.
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


#include "vodoley.h"
#include "vial.h"
#include "pult.h"
#include "dialog.h"

#include <QSettings>
#include <QFile>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QStatusBar>
#include <QResizeEvent>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QAction>
#include <QDebug>

#define MAX_SIZE 160.0
//static const double Pi = 4.1415926535897932384626433832795028841971694;


Vodoley::Vodoley()
{
	autoClose = false;
	pult = new VodoleyPult(0, 0, this);

	scene = new QGraphicsScene(this);
	view = new QGraphicsView(this);
	view->setSceneRect(0, 0, 354, 220);
	view->resize(370, 245);

	//Asize()=80;Bsize()=7;Csize()=9; //Размер емкости
	Maxfill[0] = 8;
	Maxfill[1] = 5;
	Maxfill[2] = 3;


	Afill = 0;
	Bfill = 0;
	Cfill = 0; //Сколько  налито изначально

	Curfill[0] = Afill;//Сколько сейчас налито
	Curfill[1] = Bfill;//Сколько сейчас налито
	Curfill[2] = Cfill;//Сколько сейчас налито

	AfillR = 4, BfillR = 4, CfillR = 5;


	//  view->resize(500,500);
	view->setScene(scene);
	setCentralWidget(view);
//	view->move(0,menuBar->height ()+HEADER_SIZE);
	//qDebug()<<menuBar->height ();
	this->setBaseSize(364, 235);
	this->resize(160, 145 + 1);



	this->setMinimumSize(this->size());
	//this->setMaximumSize(this->size());
	this->setSizePolicy(QSizePolicy::Expanding,
		QSizePolicy::Expanding);
	this->statusBar()->hide();
//	vodHeader->setChildWidget(this);


	scene->setBackgroundBrush(QBrush(QColor(175, 200, 203)));

	setWindowTitle(QString::fromUtf8("Водолей"));
//	vodHeader->setWMTitle(QString::fromUtf8("Водолей"));

	QPen redPen(QColor(250, 10, 10));

	QSettings vSett("NIISI RAS", "Vodoley");
	curDir = vSett.value("Dir", QApplication::applicationDirPath() + "/Addons/vodoley/resources/").toString();
	QString curFile = vSett.value("File", "").toString();
	qDebug() << "curFile:" << curFile;

	needFrame = new QFrame(view);
	needFrame->setGeometry(165, 5, 40, 25);
	needFrame->setFrameShadow(QFrame::Sunken);
	needFrame->setFrameShape(QFrame::Panel);
	needFrame->setPalette(QPalette(QColor(140, 140, 140), QColor(100, 100, 100)));
	needFrame->setBackgroundRole(QPalette::Window);
	needFrame->setAutoFillBackground(true);
	needFrame->show();
	needLabel = new QLabel(needFrame);
	needLabel->setText("999");
	needLabel->move(2, 2);
	needLabel->setGeometry(1, 5, 39, 24);
	needLabel->setAlignment(Qt::AlignHCenter);
	needLabel->show();

	CreateVodoley();

	CreateDummyTask();

	if (curFile != "" && QFile::exists(curFile)) {
		//loadFile(curFile);
	} else {
		CreateDummyTask();
#ifndef Q_OS_MAC
		//loadFile(QApplication::applicationDirPath()+"/Addons/vodoley/resources/test.vod");
#endif
#ifdef Q_OS_MAC
		//loadFile(QApplication::applicationDirPath()+"/../Resources/Addons/vodoley/resources/test.vod");
#endif
	}
}

void Vodoley::CreateDummyTask()
{
	Maxfill[0] = 3;
	Maxfill[1] = 5;
	Maxfill[2] = 7;
	qDebug() << "ASize" << Asize();

	AfillR = 1;

	Amen->setNeedFill(1);
	Bmen->setNeedFill(1);
	Cmen->setNeedFill(1);

	setBaseFill(0, 0, 0);
	Afill = 0;
	Bfill = 0;
	Cfill = 0;
	Curfill[0] = 0;
	Curfill[1] = 0;
	Curfill[2] = 0;
	updateMenzur();
}

void Vodoley::CreateVodoley(void)
{
	float literSize = MAX_SIZE / maxSize();
	qDebug() << "Liter Size:" << literSize;
	Amen = new Vial(30, 30 + (maxSize() - Asize())*literSize, Asize(), literSize, &mutex);
	Bmen = new Vial(140, 30 + (maxSize() - Bsize())*literSize, Bsize(), literSize, &mutex);
	Cmen = new Vial(250, 30 + (maxSize() - Csize())*literSize, Csize(), literSize, &mutex);

	Amen->setCurFill(CurA());
	Bmen->setCurFill(CurB());
	Cmen->setCurFill(CurC());

	Amen->setNeedFill(AfillR);
	Bmen->setNeedFill(BfillR);
	Cmen->setNeedFill(CfillR);

	qDebug() << CurB();
	scene->addItem(Amen);
	scene->addItem(Bmen);
	scene->addItem(Cmen);

	Atext = scene->addSimpleText("A");
	Atext->setPos(35, 15 + (maxSize() - Asize())*literSize);
	Btext = scene->addSimpleText("B");
	Btext->setPos(145, 15 + (maxSize() - Bsize())*literSize);
	Ctext = scene->addSimpleText("C");
	Ctext->setPos(255, 15 + (maxSize() - Csize())*literSize);

	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void Vodoley::createActions(QList<QAction *> actions)
{
	if (actions.count() < 3) {
		qDebug() << "Vodoley: cant create actions!";
		return;
	}

	actNew = actions.at(0);
	connect(actNew, SIGNAL(triggered()), this, SLOT(newZ()));
	actLoad = actions.at(1);
	connect(actLoad, SIGNAL(triggered()), this, SLOT(loadZ()));
	actSave = actions.at(2);
	connect(actSave, SIGNAL(triggered()), this, SLOT(saveZ()));
}

void Vodoley::resizeEvent(QResizeEvent *event)
{
	this->setSizePolicy(QSizePolicy::Expanding,
		QSizePolicy::Expanding);
	view->resize(event->size());
	QRectF sizeRect = QRectF(scene->sceneRect().topLeft(), QSize(qMax(scene->sceneRect().width(), scene->sceneRect().height()) + 43, qMax(scene->sceneRect().width(), scene->sceneRect().height())));
	view->fitInView(sizeRect, Qt::KeepAspectRatio);
	needFrame->move(view->width() / 2 - 10, 5);

	QWidget::resizeEvent(event);
}

void Vodoley::reset()
{
	mutex.tryLock(30);
	setBaseFill(Afill, Bfill, Cfill);
	mutex.unlock();
	//updateMenzur();
	redraw();
}


void Vodoley::FillA()
{
	mutex.lock();
	Curfill[0] = Asize();
	mutex.unlock();
	// updateMenzur();
	redraw();
}

void Vodoley::FillB()
{
	mutex.lock();
	Curfill[1] = Bsize();
	mutex.unlock();
	// updateMenzur();
	redraw();
}

void Vodoley::FillC()
{
	mutex.lock();
	Curfill[2] = Csize();
	mutex.unlock();
	//updateMenzur();
	redraw();
}


void Vodoley::MoveFromTo(uint from, uint to)
{
	mutex.lock();
	if (to > 2) {
		Curfill[from] = 0;
		mutex.unlock();
		//updateMenzur();
		redraw();
		return;
	};//Выливаем
	int svobodno = Maxfill[to] - Curfill[to];
	Curfill[to] = Curfill[to] + Curfill[from];
	if (Curfill[to] > Maxfill[to]) {
		Curfill[to] = Maxfill[to];
	}

	if ((uint)svobodno > Curfill[from]) {
		Curfill[from] = 0;
	} else {
		Curfill[from] = Curfill[from] - svobodno;
	}
	mutex.unlock();
	// updateMenzur();
	redraw();
	QApplication::processEvents();

}


Vodoley::~Vodoley()
{
}

void Vodoley::newZ()
{
	Dialog *newZdialog = new Dialog();

	newZdialog->setSizes(Asize(), Bsize(), Csize());
	newZdialog->setFill(Afill, Bfill, Cfill);
	newZdialog->setNeed(AfillR);

	if (!newZdialog->exec()) {
		return;
	}

	Maxfill[0] = newZdialog->ASize();
	Maxfill[1] = newZdialog->BSize();
	Maxfill[2] = newZdialog->CSize();

	AfillR = newZdialog->getNeed();

	Amen->setNeedFill(AfillR);
	Bmen->setNeedFill(AfillR);
	Cmen->setNeedFill(AfillR);

	setBaseFill(newZdialog->AFill(), newZdialog->BFill(), newZdialog->CFill());
	Afill = newZdialog->AFill();
	Bfill = newZdialog->BFill();
	Cfill = newZdialog->CFill();

	delete newZdialog;
	//updateMenzur();
	redraw();
	setWindowTitle(QString::fromUtf8("Водолей - новое"));
}

bool Vodoley::loadIoDevice(QIODevice *source)
{
	QString curLine;
	bool size = false;
	bool fill = false;
	bool need = false;

	while (!source->atEnd()) { //Читаем файл
		curLine = source->readLine().simplified();
		if ((curLine.isEmpty()) || (curLine[0] == ';')) {
			qDebug() << "Continue";
			continue;
		}

		QStringList data = curLine.split(" ");
		if (data.count() != 3) {
			QMessageBox::information(0, "", trUtf8("Ошибка чтения задания! "), 0, 0, 0);
		}
		if (!size) { //Читаем размер
			bool ok;
			Maxfill[0] = data[0].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения задания: размер не верен!"), 0, 0, 0);
			}

			Maxfill[1] = data[1].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения задания: размер не верен! "), 0, 0, 0);
			}
			Maxfill[2] = data[2].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения задания: размер не верен!"), 0, 0, 0);
			}
			size = true;
			continue;
		}

		if (!fill) { //Читаем наполненность
			bool ok;
			Afill = data[0].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения:наполненость A не верна! "), 0, 0, 0);
			}
			Bfill = data[1].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения:наполненость B не верна!"), 0, 0, 0);
			}
			Cfill = data[2].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения:наполненость C не верна!"), 0, 0, 0);
			}
			fill = true;
			continue;
		}

		if (!need) { //Читаем задание
			bool ok;
			AfillR = data[0].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: необходимое количество!"), 0, 0, 0);
				return false;
			}
			BfillR = data[1].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения:необходимое количество! "), 0, 0, 0);
				return false;
			}
			CfillR = data[2].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: необходимое количество!"), 0, 0, 0);
				return false;
			}
			need = true;
			continue;
		}
	}

	return true;
}

bool Vodoley::loadFile(QString fileName)
{
	QFile VodFile(fileName);
	if (!VodFile.open(QIODevice::ReadOnly)) {
		QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
		return false;
	}

	if (VodFile.atEnd()) {
		QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
	}

	QString curLine;
	bool size = false;
	bool fill = false;
	bool need = false;

	while (!VodFile.atEnd()) { //Читаем файл
		curLine = VodFile.readLine().simplified();
		if ((curLine.isEmpty()) || (curLine[0] == ';')) {
			qDebug() << "Continue";
			continue;
		}

		QStringList data = curLine.split(" ");
		if (data.count() != 3) {
			QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
		}

		if (!size) { //Читаем размер
			bool ok;
			Maxfill[0] = data[0].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения размера: ") + fileName, 0, 0, 0);
			}

			Maxfill[1] = data[1].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения размера:  ") + fileName, 0, 0, 0);
			}
			Maxfill[2] = data[2].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения размера:  ") + fileName, 0, 0, 0);
			}
			size = true;
			continue;
		}

		if (!fill) { //Читаем наполненость
			bool ok;
			Afill = data[0].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
			}
			Bfill = data[1].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
			}
			Cfill = data[2].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
			}
			fill = true;
			continue;
		}

		if (!need) { //Читаем задание
			bool ok;
			AfillR = data[0].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
				return false;
			};
			BfillR = data[1].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
				return false;
			};
			CfillR = data[2].toInt(&ok);
			if (!ok) {
				QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
				return false;
			};
			need = true;
			continue;
		}
	}

	VodFile.close();
	QSettings vSett("NIISI RAS", "Vodoley");
	QFileInfo fi(VodFile);
	curDir = fi.absolutePath();
	vSett.setValue("Dir", fi.absolutePath());
	vSett.setValue("File", fileName);

	setBaseFill(Afill, Bfill, Cfill);
	Amen->setNeedFill(AfillR);
	Bmen->setNeedFill(BfillR);
	Cmen->setNeedFill(CfillR);
	updateMenzur();
	setWindowTitle(QString::fromUtf8("Водолей - ") +  fi.baseName());
	emit FileLoaded(fileName);
	return true;
};

void Vodoley::loadZ()
{
	QString VodFileN = QFileDialog::getOpenFileName(this, QString::fromUtf8("Открыть файл"), curDir, "(*.vod)");

	if (VodFileN.isEmpty()) {
		return;
	}

	loadFile(VodFileN);
}

void Vodoley::saveZ()
{
	QString VodFile = QFileDialog::getSaveFileName(this, QString::fromUtf8("Сохранить файл"), curDir, "(*.vod)");

	if (VodFile.right(4) != ".vod") {
		VodFile += ".vod";
	}
	QFile vFile(VodFile);
	if (!vFile.open(QIODevice::WriteOnly)) {
		QMessageBox::information(0, "", trUtf8("Ошибка записи: ") + VodFile, 0, 0, 0);
		return;
	};

	//Zapis v fayl
	vFile.write(";Capacity\n");
	vFile.write((QString::number(Asize()) + " " + QString::number(Bsize()) + " " + QString::number(Csize())).toLatin1());
	vFile.write("\n;Fullness\n");
	vFile.write((QString::number(Afill) + " " + QString::number(Bfill) + " " + QString::number(Cfill)).toLatin1());
	vFile.write("\n;Need\n");
	vFile.write((QString::number(AfillR) + " " + QString::number(BfillR) + " " + QString::number(CfillR)).toLatin1());

	vFile.close();

	QSettings vSett("NIISI RAS", "Vodoley");
	QFileInfo fi(vFile);
	curDir = fi.absolutePath();
	vSett.setValue("Dir", fi.absolutePath());
	vSett.setValue("File", fi.filePath());
	setWindowTitle(QString::fromUtf8("Водолей - ") +  fi.baseName());
}


void Vodoley::updateMenzur()
{
	float literSize = MAX_SIZE / maxSize();
	if (Asize() == 0) {
		Amen->hide();    //TODO LockPult
		Atext->hide();
	} else {
		Amen->show();
		Atext->show();
	};
	Amen->setSize(Asize());
	Amen->setLiterSize(literSize);
	Amen->setYoffset(40 + (maxSize() - Asize())*literSize);
	Amen->setCurFill(CurA());

	if (Bsize() == 0) {
		Bmen->hide();    //TODO LockPult
		Btext->hide();
	} else {
		Bmen->show();
		Btext->show();
	}

	Bmen->setSize(Bsize());
	Bmen->setLiterSize(literSize);
	Bmen->setYoffset(40 + (maxSize() - Bsize())*literSize);
	Bmen->setCurFill(CurB());

	if (Csize() == 0) {
		Cmen->hide();    //TODO LockPult
		Ctext->hide();
	} else {
		Cmen->show();
		Ctext->show();
	}

	Cmen->setSize(Csize());
	Cmen->setLiterSize(literSize);
	Cmen->setYoffset(40 + (maxSize() - Csize())*literSize);
	Cmen->setCurFill(CurC());

	Atext->setPos(35, 15 + (maxSize() - Asize())*literSize);
	Btext->setPos(145, 15 + (maxSize() - Bsize())*literSize);
	Ctext->setPos(255, 15 + (maxSize() - Csize())*literSize);

	updateNeedBirka();
}

void Vodoley::redraw()
{
	mutex.lock();
	updateMenzur();
	view->update();
	scene->update();
	mutex.unlock();
}

void Vodoley::updateNeedBirka()
{
	if (needFrame) {
		if ((CurA() == AfillR) || (CurB() == AfillR) || (CurC() == AfillR)) {
			needFrame->setPalette(QPalette(QColor(50, 90, 50), QColor(100, 190, 100)));
		} else {
			needFrame->setPalette(QPalette(QColor(140, 140, 160), QColor(140, 140, 160)));
		}

	} else {
		qDebug() << "updateNeedBirka():No needFrame";
	}

	needLabel->setText(" " + QString::number(AfillR));
	qDebug() << "NEED:" << QString::number(AfillR);
}

void Vodoley::mousePressEvent(QMouseEvent *event)
{
	QPointF point = event->pos() - rect().center();
	qDebug() << "TailPoint:" << point;
	//Curfill[0]=CurA()-1;
	redraw();
	qDebug() << "View geometrey" << view->geometry();
}

#if 0
NewDialog::NewDialog()
{
	setWindowFlags(Qt::Dialog);
	QVBoxLayout *layout = new QVBoxLayout;
	sizeFrame = new QFrame(this);
	fillFrame = new QFrame(this);;
	valueFrame = new QFrame(this);
	buttonFrame = new QFrame(this);

	btnOK = new QToolButton(buttonFrame);
	btnOK->move(150, 5);
	btnOK->setText(trUtf8("OK"));
	btnCancel = new QToolButton(buttonFrame);
	btnCancel->setText(trUtf8("Отмена"));
	btnCancel->move(50, 5);
	connect(btnCancel, SIGNAL(pressed()), this, SLOT(close()));

	sizeFrame->setFrameStyle(QFrame::Panel | QFrame::Raised);
	fillFrame->setFrameStyle(QFrame::Panel | QFrame::Raised);
	valueFrame->setFrameStyle(QFrame::Panel | QFrame::Raised);
	buttonFrame->setFrameStyle(QFrame::Panel | QFrame::Raised);

	resize(200, 170);
	sizeFrame->setMaximumSize(200, 70);
	fillFrame->setMaximumSize(200, 70);
	valueFrame->setMaximumSize(200, 70);
	buttonFrame->setMaximumSize(200, 40);

	layout->addWidget(sizeFrame);
	layout->addWidget(fillFrame);
	layout->addWidget(valueFrame);
	layout->addWidget(buttonFrame);
	this->setLayout(layout);
}
#endif
