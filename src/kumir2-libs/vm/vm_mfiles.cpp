#include "vm.hpp"

namespace VM
{

void KumirVM::do_filescall(uint16_t alg)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	valuesStack_.pop(); // Args count
	switch (alg) {
	/* алг файл открыть на чтение(лит имя файла) */
	case 0x0000: {
		const String x = valuesStack_.pop().toString();
		Kumir::FileType y = Kumir::Files::open(x, Kumir::FileType::Read, true, 0);
		Record yy = toRecordValue(y);
		Variable res(yy, Kumir::Core::fromUtf8("файл"), std::string("file"));
		valuesStack_.push(res);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг файл открыть на запись(лит имя файла) */
	case 0x0001: {
		const String x = valuesStack_.pop().toString();
		Kumir::FileType y = Kumir::Files::open(x, Kumir::FileType::Write, true, 0);
		Record yy = toRecordValue(y);
		valuesStack_.push(Variable(yy, Kumir::Core::fromUtf8("файл"), std::string("file")));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг файл открыть на добавление(лит имя файла) */
	case 0x0002: {
		const String x = valuesStack_.pop().toString();
		Kumir::FileType y = Kumir::Files::open(x, Kumir::FileType::Append, true, 0);
		Record yy = toRecordValue(y);
		valuesStack_.push(Variable(yy, Kumir::Core::fromUtf8("файл"), std::string("file")));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг закрыть(файл ключ) */
	case 0x0003: {
		const Variable xvar = valuesStack_.pop();
		const Record xx = xvar.toRecord();
		Kumir::FileType x = fromRecordValue(xx);
		Kumir::Files::close(x);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг начать чтение(файл ключ) */
	case 0x0004: {
		const Variable &xval = valuesStack_.pop();
		const Record xx = xval.toRecord();
		Kumir::FileType x = fromRecordValue(xx);
		Kumir::Files::reset(x);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг лог конец файла(файл ключ) */
	case 0x0005: {
		const Variable &xval = valuesStack_.pop();
		const Record xx = xval.toRecord();
		Kumir::FileType x = fromRecordValue(xx);
		bool y = Kumir::Files::eof(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг установить кодировку(лит имя кодировки) */
	case 0x0006: {
		const String x = valuesStack_.pop().toString();
		Kumir::Files::setFileEncoding(x);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг лог можно открыть на чтение(лит имя файла) */
	case 0x0007: {
		const String x = valuesStack_.pop().toString();
		bool y = Kumir::Files::canOpenForRead(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг лог можно открыть на запись(лит имя файла) */
	case 0x0008: {
		const String x = valuesStack_.pop().toString();
		bool y = Kumir::Files::canOpenForWrite(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг лог есть данные(файл ключ) */
	case 0x0009: {
		const Variable xval = valuesStack_.pop();
		const Record xx = xval.toRecord();
		Kumir::FileType x = fromRecordValue(xx);
		bool y = Kumir::Files::hasData(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг лог существует(лит имя файла Или каталога) */
	case 0x000a: {
		const String x = valuesStack_.pop().toString();
		bool y = Kumir::Files::exist(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг лог является каталогом(лит имя файла Или каталога) */
	case 0x000b: {
		const String x = valuesStack_.pop().toString();
		bool y = Kumir::Files::isDirectory(x);
		valuesStack_.push(Variable(y));
		break;
	}
	case 0x000c: {
		const String x = valuesStack_.pop().toString();
		bool y = Kumir::Files::mkdir(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг лит полный путь(лит имя файла Или каталога) */
	case 0x000d: {
		const String x = valuesStack_.pop().toString();
		const String y = Kumir::Files::getAbsolutePath(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг удалить_файл(лит имя файла) */
	case 0x000e: {
		const String x = valuesStack_.pop().toString();
		valuesStack_.push(Variable(Kumir::Files::unlink(x)));
		break;
	}
	/* алг лит РАБОЧИЙ КАТАЛОГ */
	case 0x0011: {
		const String res = Kumir::Files::CurrentWorkingDirectory();
		valuesStack_.push(Variable(res));
		break;
	}
	/* алг лит КАТАЛОГ ПРОГРАММЫ */
	case 0x0012: {
		String res;
		if (programDirectory_.length() == 0) {
			res = Kumir::Core::fromAscii("./");
		} else {
			res = programDirectory_;
		}
		valuesStack_.push(Variable(res));
		break;
	}
	/* алг удалить_каталог(лит имя файла) */
	case 0x0013: {
		const String x = valuesStack_.pop().toString();
		valuesStack_.push(Variable(Kumir::Files::rmdir(x)));
		break;
	}
	/* алг файл консоль */
	case 0x0014: {
		const Kumir::FileType console = Kumir::Files::getConsoleBuffer();
		const Record record = toRecordValue(console);
		valuesStack_.push(Variable(record, Kumir::Core::fromUtf8("файл"), std::string("file")));
		break;
	}
	/* алг лог =(фaйл ф1, файл ф2) */
	case 0x0015: {
		const Variable f2var = valuesStack_.pop();
		const Variable f1var = valuesStack_.pop();
		const Record f2rec = f2var.toRecord();
		const Record f1rec = f1var.toRecord();
		const Kumir::FileType f1 = fromRecordValue(f1rec);
		const Kumir::FileType f2 = fromRecordValue(f2rec);
		valuesStack_.push(Variable(f1 == f2));
		break;
	}
	/* алг лог =(фaйл ф1, файл ф2) */
	case 0x0016: {
		const Variable f2var = valuesStack_.pop();
		const Variable f1var = valuesStack_.pop();
		const Record f2rec = f2var.toRecord();
		const Record f1rec = f1var.toRecord();
		const Kumir::FileType f1 = fromRecordValue(f1rec);
		const Kumir::FileType f2 = fromRecordValue(f2rec);
		valuesStack_.push(Variable(f1 != f2));
		break;
	}
	default: {
		error_ = Kumir::Core::fromUtf8("Вызов неизвестного алгоритма, возможно из более новой версии Кумир");
	}
	}
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

}

