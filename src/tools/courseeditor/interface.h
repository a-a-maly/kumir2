#ifndef INTERFACE_H
#define INTERFACE_H

#include "mainwindow.h"

class ControlInterface: public QObject, public  taskControlInterface
{
	Q_OBJECT
	Q_INTERFACES(taskControlInterface)

public:
	void start(QString csName);
	void setCSmode(int mode);
	void setCSinterface(class CSInterface *csInterface);

	void setWindowGeometry(QRect rect)
	{
		w.setGeometry(rect);
	}

	void show()
	{
		w.showNormal();
	}

	void checkFinished(int mark);

	KumZadanie *Task()
	{
		return &w.task;
	}

	QString Isp(int no)
	{
		return w.task.Isp(no);
	}
	QString ispName();

	QString CSName()
	{
		return CSname;
	}

	MainWindowTask w;

public:
	class CSInterface  *Interface()
	{
		return csInterface;
	}

private:
	QString CSname;
	class CSInterface *csInterface;
	int Mark;
};
#endif // INTERFACE_H
