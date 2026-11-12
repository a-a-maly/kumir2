#include "vm.hpp"

namespace VM
{

void KumirVM::do_stringscall(uint16_t alg)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	valuesStack_.pop(); // Args count
	switch (alg) {
	/* алг лит верхний регистр(лит строка) */
	case 0x0000: {
		const String x = valuesStack_.pop().toString();
		const String y = Kumir::StringUtils::toUpperCase(x);
		Variable res(y);
		valuesStack_.push(res);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг лит нижний регистр(лит строка) */
	case 0x0001: {
		const String x = valuesStack_.pop().toString();
		const String y = Kumir::StringUtils::toLowerCase(x);
		Variable res(y);
		valuesStack_.push(res);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг цел позиция после(цел от, лит фрагмент, лит строка) */
	case 0x0002:
	case 0x0007: {
		const String s = valuesStack_.pop().toString();
		const String sub = valuesStack_.pop().toString();
		const int from = valuesStack_.pop().toInt();
		const int y = Kumir::StringUtils::find(from + 1, sub, s);
		Variable res(y);
		valuesStack_.push(res);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг цел позиция(лит фрагмент, лит строка) */
	case 0x0003:
	case 0x0008: {
		const String s = valuesStack_.pop().toString();
		const String sub = valuesStack_.pop().toString();
		const int y = Kumir::StringUtils::find(1, sub, s);
		Variable res(y);
		valuesStack_.push(res);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вставить(лит фрагмент, аргрез лит строка, цел позиция) */
	case 0x0004: {
		const int from = valuesStack_.pop().toInt();
		Variable sr = valuesStack_.pop().toReference();
		String s = sr.value().toString();
		const String sub = valuesStack_.pop().toString();
		Kumir::StringUtils::insert(sub, s, from);
		sr.setValue(AnyValue(s));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг заменить(аргрез лит строка, лит старый фрагмент, лит новый фрагмент, лог каждый) */
	case 0x0005: {
		const bool all = valuesStack_.pop().toBool();
		const String newSub = valuesStack_.pop().toString();
		const String oldSub = valuesStack_.pop().toString();
		Variable sr = valuesStack_.pop().toReference();
		String s = sr.value().toString();
		Kumir::StringUtils::replace(s, oldSub, newSub, all);
		sr.setValue(AnyValue(s));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг удалить(аргрез лит строка, цел начало, цел количество) */
	case 0x0006: {
		const int count = valuesStack_.pop().toInt();
		const int from = valuesStack_.pop().toInt();
		Variable sr = valuesStack_.pop().toReference();
		String s = sr.value().toString();
		Kumir::StringUtils::remove(s, from, count);
		sr.setValue(AnyValue(s));
		error_ = Kumir::Core::getError();
		break;
	}

	default: {
		error_ = Kumir::Core::fromUtf8("Вызов неизвестного алгоримта, возможно из более новой версии Кумир");
	}
	}
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

}
