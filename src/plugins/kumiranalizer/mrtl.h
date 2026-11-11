#ifndef MRTL_H
#define MRTL_H

#include <kumir2/actorinterface.h>

namespace StdLibModules
{

class RTL : public Shared::ActorInterface
{
public:
	QByteArray asciiModuleName() const
	{
		return QByteArray("Kumir Standard Library");
	}

	QString localizedModuleName(const QLocale::Language) const
	{
		return QString::fromUtf8("Стандартные функции");
	}

	void terminateEvaluation() {}

	FunctionList functionList() const;
};

}

#endif
