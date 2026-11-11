#include "mstrings.h"
#include <QDebug>

namespace StdLibModules
{
	Strings::FunctionList Strings::functionList() const
	{
		// fprintf(stderr, "Strings::functionList \n");

		FunctionList result;
		Function func;
		func.accessType = PublicFunction;

		/* алг лит верхний регистр(лит строка) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0000;
		func.asciiName = "uppercase";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("верхний регистр");
		func.returnType = String;
		func.arguments << Argument("s", String);
		result << func;

		/* алг лит нижний регистр(лит строка) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0001;
		func.asciiName = "lowercase";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("нижний регистр");
		func.returnType = String;
		func.arguments << Argument("s", String);
		result << func;

		/* алг цел позиция после(цел От, лит фрагмент, лит строка) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0002;
		func.asciiName = "index after";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("позиция после");
		func.returnType = Int;
		func.arguments << Argument("from", Int);
		func.arguments << Argument("pattern", String);
		func.arguments << Argument("source", String);
		result << func;

		// дубликат: поз вместо позиция
		func.id = 0x0007;
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("поз после");
		result << func;

		/* алг цел позиция(лит фрагмент, лит строка) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0003;
		func.asciiName = "index";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("позиция");
		func.returnType = Int;
		func.arguments << Argument("pattern", String);
		func.arguments << Argument("source", String);
		result << func;

		// дубликат: поз вместо позиция
		func.id = 0x0008;
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("поз");
		result << func;

		/* алг вставить(лит фрагмент, аргрез лит строка, арг цел позиция) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0004;
		func.asciiName = "insert";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("вставить");
		func.returnType = Void;
		func.arguments << Argument("data", String);
		func.arguments << Argument("source", String);
		func.arguments[1].accessType = InOutArgument;
		func.arguments << Argument("ind", Int);
		result << func;

		/* алг заменить(аргрез лит строка, арг лит старый фрагмент, лит новый фрагмент, лог каждый) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0005;
		func.asciiName = "replace";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("заменить");
		func.returnType = Void;
		func.arguments << Argument("source", String);
		func.arguments[0].accessType = InOutArgument;
		func.arguments << Argument("pattern", String);
		func.arguments << Argument("data", String);
		func.arguments << Argument("each", Bool);
		result << func;

		/* алг удалить(аргрез лит строка, арг цел начало, цел количество) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0006;
		func.asciiName = "remove";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("удалить");
		func.returnType = Void;
		func.arguments << Argument("source", String);
		func.arguments[0].accessType = InOutArgument;
		func.arguments << Argument("start", Int);
		func.arguments << Argument("count", Int);
		result << func;

		return result;
	}
}

