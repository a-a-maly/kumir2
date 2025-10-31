#ifndef TASKCONTROLINTERFACE_H
#define TASKCONTROLINTERFACE_H

#include <QVariant>
#include <QStringList>
#include <QMap>
#include <QtPlugin>

class CSInterface;
class KumZadanie
{
public:
	KumZadanie()
	{
		isps.clear();
	}

	QString Isp(int no)const
	{
		if (isps.count() <= no) {
			return "";
		}
		return isps[no];
	}

	QString field(QString ispName, int fieldNo)
	{
		QList<QString> ispFields = fields.values(ispName);
		if (fieldNo >= ispFields.count()) {
			return "";
		}
		return ispFields.at(fieldNo);
	}

	int fieldsCount(QString ispName)
	{
		QList<QString> ispFields = fields.values(ispName);

		return ispFields.count();
	}

	QStringList isps;//исполнители используемые в задание
	QStringList Scripts;//скрипты используемые в задание
	QMap<QString, QString> fields;//Обстановки для каждого исполнителя fields[исп,обст]

};

class taskControlInterface
{
	// Q_OBJECT
public:
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
