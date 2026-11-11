#ifndef MSTRINGS_H
#define MSTRINGS_H

#include <kumir2/actorinterface.h>

namespace StdLibModules
{

class Strings : public Shared::ActorInterface
{
	QByteArray asciiModuleName() const
	{
		return QByteArray("String Utilities");
	}

	QString localizedModuleName(QLocale::Language) const
	{
		return QString::fromUtf8("Строки");
	}

	void terminateEvaluation() {}

	FunctionList functionList() const;
};

}

#endif
