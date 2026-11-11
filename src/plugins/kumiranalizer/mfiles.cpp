#include "mfiles.h"

namespace StdLibModules
{

	Files::TypeList Files::typeList() const
	{
		// fprintf(stderr, "Files::typeList \n");

		Field fileKey(QByteArray("key"), Int);
		Field openMode(QByteArray("mode"), Int);
		Field specialType(QByteArray("type"), Int);
		Field fileName(QByteArray("name"), String);
		Record fileRecord;
		fileRecord << fileKey << openMode << specialType << fileName;

		RecordSpecification fileType;
		fileType.asciiName = "file";
		fileType.localizedNames[QLocale::Russian] = QString::fromUtf8("файл");
		fileType.record = fileRecord;

		TypeList result;
		result << fileType;
		return result;
	}

	Files::FunctionList Files::functionList() const
	{
		// fprintf(stderr, "Files::functionList \n");

		FunctionList result;
		Function func;
		func.accessType = PublicFunction;
		const RecordSpecification FileType = typeList().first();

		/* алг файл открыть на чтение(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0000;
		func.asciiName = "read open";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("открыть на чтение");
		func.returnType = RecordType;
		func.returnTypeSpecification = FileType;
		func.arguments << Argument("name", String);
		result << func;

		/* алг файл открыть на запись(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0001;
		func.asciiName = "write open";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("открыть на запись");
		func.returnType = RecordType;
		func.returnTypeSpecification = FileType;
		func.arguments << Argument("name", String);
		result << func;

		/* алг файл открыть на добавление(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0002;
		func.asciiName = "append open";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("открыть на добавление");
		func.returnType = RecordType;
		func.returnTypeSpecification = FileType;
		func.arguments << Argument("name", String);
		result << func;

		/* алг закрыть(файл ключ) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0003;
		func.asciiName = "close";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("закрыть");
		func.returnType = Void;
		func.arguments << Argument("key", RecordType);
		func.arguments.last().typeSpecification = FileType;
		result << func;

		/* алг начать чтение(файл ключ) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0004;
		func.asciiName = "start reading";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("начать чтение");
		func.returnType = Void;
		func.arguments << Argument("key", RecordType);
		func.arguments.last().typeSpecification = FileType;
		result << func;

		/* алг лог конец файла(файл ключ) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0005;
		func.asciiName = "eof";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("конец файла");
		func.returnType = Bool;
		func.arguments << Argument("key", RecordType);
		func.arguments.last().typeSpecification = FileType;
		result << func;

		/* алг установить кодировку(лит кодировка) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0006;
		func.asciiName = "set encoding";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("установить кодировку");
		func.returnType = Void;
		func.arguments << Argument("name", String);
		result << func;

		/* алг лог можно открыть на чтение(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0007;
		func.asciiName = "readable";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("можно открыть на чтение");
		func.returnType = Bool;
		func.arguments << Argument("name", String);
		result << func;

		/* алг лог можно открыть на запись(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0008;
		func.asciiName = "writable";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("можно открыть на запись");
		func.returnType = Bool;
		func.arguments << Argument("name", String);
		result << func;

		/* алг лог есть данные(файл ключ) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0009;
		func.asciiName = "nonempty";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("есть данные");
		func.returnType = Bool;
		func.arguments << Argument("key", RecordType);
		func.arguments.last().typeSpecification = FileType;
		result << func;

		/* алг лог существует(лит имя файла Или каталога) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x000a;
		func.asciiName = "exists";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("существует");
		func.returnType = Bool;
		func.arguments << Argument("path", String);
		result << func;

		/* алг лог является каталогом(лит имя файла Или каталога) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x000b;
		func.asciiName = "isdir";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("является каталогом");
		func.returnType = Bool;
		func.arguments << Argument("path", String);
		result << func;

		/* алг лог создать каталог(лит имя файла Или каталога) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x000c;
		func.asciiName = "mkdir";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("создать каталог");
		func.returnType = Bool;
		func.arguments << Argument("path", String);
		result << func;

		/* алг лит полный путь(лит имя файла Или каталога) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x000d;
		func.asciiName = "abspath";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("полный путь");
		func.returnType = String;
		func.arguments << Argument("path", String);
		result << func;

		/* алг лог удалить_файл(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x000e;
		func.asciiName = "rm";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("удалить_файл");
		func.returnType = Bool;
		func.arguments << Argument("path", String);
		result << func;

		/* алг лит РАБОЧИЙ КАТАЛОГ */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0011;
		func.asciiName = "CURRENT DIR";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("РАБОЧИЙ КАТАЛОГ");
		func.returnType = String;
		result << func;

		/* алг лит КАТАЛОГ ПРОГРАММЫ */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0012;
		func.asciiName = "PROGRAM DIR";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("КАТАЛОГ ПРОГРАММЫ");
		func.returnType = String;
		result << func;

		/* алг лог удалить_каталог(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0013;
		func.asciiName = "rmdir";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("удалить_каталог");
		func.returnType = Bool;
		func.arguments << Argument("path", String);
		result << func;

		/* алг файл консоль */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0014;
		func.asciiName = "get console";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("консоль");
		func.returnType = RecordType;
		func.returnTypeSpecification = FileType;
		result << func;

		/* алг лог =(файл ф1, файл ф2) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0015;
		func.asciiName = "=";
		func.returnType = Bool;
		func.arguments << Argument("first", RecordType) << Argument("second", RecordType);
		func.arguments[0].typeSpecification = func.arguments[1].typeSpecification = FileType;
		result << func;

		/* алг лог <>(файл ф1, файл ф2) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0016;
		func.asciiName = "<>";
		func.returnType = Bool;
		func.arguments << Argument("first", RecordType) << Argument("second", RecordType);
		func.arguments[0].typeSpecification = func.arguments[1].typeSpecification = FileType;
		result << func;

		return result;
	}
}

