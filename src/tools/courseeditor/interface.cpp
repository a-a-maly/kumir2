#include "interface.h"
#include "kumzadanie.h"
#include "mainwindow.h"
#include <QDebug>

ControlInterface::ControlInterface() :
	csName(),
	csInterface(0),
	w(new MainWindowTask())
{
}

ControlInterface::~ControlInterface()
{
	delete w;
	w = 0;
}


void ControlInterface::start(QString _csName)
{
	csName = _csName;
	w->setCS(csName);
	w->setup();
}

void ControlInterface::setCSmode(int mode)
{
	qDebug() << "DUMMY call ControlInterface::setCSmode";
}

void ControlInterface::setCSinterface(CSInterface *csInterface)
{
	this->csInterface = csInterface;
	w->setInterface(csInterface);
}

QString ControlInterface::ispName() const
{
	return "";
}

void ControlInterface::setWindowGeometry(QRect rect)
{
	w->setGeometry(rect);
}

void ControlInterface::show()
{
	w->showNormal();
}

const KumZadanie *ControlInterface::Task() const
{
	return w->get_task();
}

QString ControlInterface::Isp(int no) const
{
	return w->get_task()->Isp(no);
}

void ControlInterface::checkFinished(int mark)
{
	qDebug() << "Get Mark" << mark;
	qDebug() << " ControlInterface::checkFinished calls duumy w.setMark(mark)";
	//w.setMark(mark);
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(controlInterface, ControlInterface);
#endif
