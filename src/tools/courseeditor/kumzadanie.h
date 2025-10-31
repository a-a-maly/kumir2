#ifndef KUMZADANIE_H
#define KUMZADANIE_H

#include <QStringList>
#include <QMap>

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

	QStringList isps; //исполнители используемые в задание
	QStringList Scripts; //скрипты используемые в задание
	QMap<QString, QString> fields; //Обстановки для каждого исполнителя fields[исп,обст]
};

#endif // KUMZADANIE_H
