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

	const KumZadanie *Task() const
	{
		return &w.task;
	}

	QString Isp(int no) const
	{
		return w.task.Isp(no);
	}

	QString ispName() const;

	QString CSName() const
	{
		return CSname;
	}

public:
	class CSInterface *Interface()
	{
		return csInterface;
	}

private:
	QString CSname;
	class CSInterface *csInterface;
	int Mark;
	MainWindowTask w;
};
#endif // INTERFACE_H
