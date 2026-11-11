#ifndef MFILES_H
#define MFILES_H

#include <kumir2/actorinterface.h>

namespace StdLibModules
{

class Files : public Shared::ActorInterface
{
	QByteArray asciiModuleName() const
	{
		return QByteArray("Files");
	}

	QString localizedModuleName(QLocale::Language) const
	{
		return QString::fromUtf8("Файлы");
	}

	void terminateEvaluation() {}

	TypeList typeList() const;
	FunctionList functionList() const;
};

}

#endif
