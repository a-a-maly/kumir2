#ifndef TASKCONTROLINTERFACE_H
#define TASKCONTROLINTERFACE_H

#include <QtPlugin>
#include <QRect>

class CSInterface;
class KumZadanie;

class taskControlInterface
{
	// Q_OBJECT
public:
	taskControlInterface() {}
	virtual ~taskControlInterface() {}
	virtual void start(QString csName) = 0;
	virtual void setCSmode(int mode) = 0;
	virtual void setCSinterface(CSInterface *_interface) = 0;
	virtual void setWindowGeometry(QRect retc) = 0;
	virtual void show() = 0;
	virtual void checkFinished(int mark) = 0;
	virtual const KumZadanie *Task() const = 0;
	virtual QString Isp(int no) const = 0;
	virtual QString ispName() const = 0;
};

Q_DECLARE_INTERFACE(taskControlInterface, "kumir.taskControlInterface/1.7.1");
#endif // TASKCONTROLINTERFACE_H
