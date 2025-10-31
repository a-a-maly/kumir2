#include "interface.h"

void ControlInterface::start(QString csName)
{
	CSname = csName;
	w.setCS(CSName());
	w.setup();
}

void ControlInterface::setCSmode(int mode)
{
	qDebug() << "DUMMY call ControlInterface::setCSmode";
}

void ControlInterface::setCSinterface(CSInterface *csInterface)
{
	this->csInterface = csInterface;
	w.setInterface(csInterface);
}

QString ControlInterface::ispName() const
{
	return "";
}

void ControlInterface::checkFinished(int mark)
{
	Mark = mark;
	qDebug() << "Get Mark" << mark;
	qDebug() << " ControlInterface::checkFinished calls duumy w.setMark(mark)";
	//w.setMark(mark);
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(controlInterface, ControlInterface);
#endif
