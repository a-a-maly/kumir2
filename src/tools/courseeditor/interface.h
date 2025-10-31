#ifndef INTERFACE_H
#define INTERFACE_H

#include "taskControlInterface.h"
class MainWindowTask;
class CSInterface;

class ControlInterface: public QObject, public  taskControlInterface
{
	Q_OBJECT
	Q_INTERFACES(taskControlInterface)

public:
	ControlInterface();
	virtual ~ControlInterface();

	void start(QString csName);
	void setCSmode(int mode);
	void setCSinterface(CSInterface *csInterface);
	void setWindowGeometry(QRect rect);
	void show();
	void checkFinished(int mark);
	const KumZadanie *Task() const;
	QString Isp(int no) const;
	QString ispName() const;

private:
	QString csName;
	CSInterface *csInterface;
	MainWindowTask *w;
};
#endif // INTERFACE_H
