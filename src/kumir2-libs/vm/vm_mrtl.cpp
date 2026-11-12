#include "vm.hpp"

namespace VM
{

void KumirVM::do_stdcall(uint16_t alg)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	valuesStack_.pop(); // remove arguments count -- all is known
	switch (alg) {
	/* алг вещ abs(вещ x) */
	case 0x0000: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::abs(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг вещ arccos(вещ x) */
	case 0x0001: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::arccos(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ arcctg(вещ x) */
	case 0x0002: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::arcctg(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ arcsin(вещ x) */
	case 0x0003: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::arcsin(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ arctg(вещ x) */
	case 0x0004: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::arctg(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ cos(вещ x) */
	case 0x0005: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::cos(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг вещ ctg(вещ x) */
	case 0x0006: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::ctg(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг ждать(цел x) */
	case 0x0007: {
		int x = valuesStack_.pop().toInt();
		if (x < 0) {
			error_ = Kumir::Core::fromUtf8("Отрицательное время");
		} else {
			uint32_t msec = static_cast<uint32_t>(x);
			(*delay_)(msec);
		}
		break;
	}
	/* алг цел div(цел x, цел y) */
	case 0x0008: {
		int y = valuesStack_.pop().toInt();
		int x = valuesStack_.pop().toInt();
		int r = Kumir::Math::div(x, y);
		valuesStack_.push(Variable(r));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ exp(вещ x) */
	case 0x0009: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::exp(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг цел iabs(цел x) */
	case 0x000a: {
		int x = valuesStack_.pop().toInt();
		int y = Kumir::Math::iabs(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг цел imax(цел x, цел y) */
	case 0x000b: {
		int y = valuesStack_.pop().toInt();
		int x = valuesStack_.pop().toInt();
		int r = Kumir::Math::imax(x, y);
		valuesStack_.push(Variable(r));
		break;
	}
	/* алг цел imin(цел x, цел y) */
	case 0x000c: {
		int y = valuesStack_.pop().toInt();
		int x = valuesStack_.pop().toInt();
		int r = Kumir::Math::imin(x, y);
		valuesStack_.push(Variable(r));
		break;
	}
	/* алг цел int(вещ x) */
	case 0x000d: {
		real x = valuesStack_.pop().toReal();
		int y = Kumir::Math::intt(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг цел irand(цел x, цел y) */
	case 0x000e: {
		int y = valuesStack_.pop().toInt();
		int x = valuesStack_.pop().toInt();
		int r = Kumir::Random::irand(x, y);
		valuesStack_.push(Variable(r));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг цел irnd(цел x) */
	case 0x000f: {
		int x = valuesStack_.pop().toInt();
		int y = Kumir::Random::irnd(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг вещ lg(вещ x) */
	case 0x0010: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::lg(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ ln(вещ x) */
	case 0x0011: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::ln(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ max(вещ x, вещ y) */
	case 0x0012: {
		real  y = valuesStack_.pop().toReal();
		real  x = valuesStack_.pop().toReal();
		real  r = Kumir::Math::rmax(x, y);
		valuesStack_.push(Variable(r));
		break;
	}
	/* алг вещ min(вещ x, вещ y) */
	case 0x0013: {
		real  y = valuesStack_.pop().toReal();
		real  x = valuesStack_.pop().toReal();
		real  r = Kumir::Math::rmin(x, y);
		valuesStack_.push(Variable(r));
		break;
	}
	/* алг цел mod(цел x, цел y) */
	case 0x0014: {
		int y = valuesStack_.pop().toInt();
		int x = valuesStack_.pop().toInt();
		int r = Kumir::Math::mod(x, y);
		valuesStack_.push(Variable(r));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ rand(вещ x, вещ y) */
	case 0x0015: {
		real  y = valuesStack_.pop().toReal();
		real  x = valuesStack_.pop().toReal();
		real  r = Kumir::Random::rrand(x, y);
		valuesStack_.push(Variable(r));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ rnd(вещ x) */
	case 0x0016: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Random::rrnd(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг цел sign(вещ x) */
	case 0x0017: {
		real x = valuesStack_.pop().toReal();
		int y = Kumir::Math::sign(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг вещ sin(вещ x) */
	case 0x0018: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::sin(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг вещ sqrt(вещ x) */
	case 0x0019: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::sqrt(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ tg(вещ x) */
	case 0x001a: {
		real x = valuesStack_.pop().toReal();
		real y = Kumir::Math::tg(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг вещ МАКСВЕЩ */
	case 0x001b: {
		real r = Kumir::Math::maxreal();
		valuesStack_.push(Variable(r));
		break;
	}
	/* алг цел МАКСЦЕЛ */
	case 0x001c: {
		int r = INT32_MAX;
		valuesStack_.push(Variable(r));
		break;
	}
	/* алг лит вещ_в_лит(вещ x) */
	case 0x001d: {
		real x = valuesStack_.pop().toReal();
		const Kumir::String y = Kumir::Converter::realToString(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг цел время */
	case 0x001e: {
		int r = Kumir::System::time();
		valuesStack_.push(Variable(r));
		break;
	}
	/* алг цел длин(лит s) */
	case 0x001f: {
		const String &s = valuesStack_.top().toStringRef();
		size_t l = s.length();
		valuesStack_.pop();
		valuesStack_.push(Variable((int) l));
		break;
	}
	/* алг цел код(сим ch) */
	case 0x0020: {
		Char x = valuesStack_.pop().toChar();
		int y = Kumir::StringUtils::code(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг вещ лит_в_вещ(лит s, рез лог success) */
	case 0x0021: {
		Variable sf = valuesStack_.pop().toReference();
		const String x = valuesStack_.pop().toString();
		bool ok;
		real y = Kumir::Converter::stringToReal(x, ok);
		valuesStack_.push(Variable(y));
		sf.setValue(AnyValue(ok));
		break;
	}
	/* алг цел лит_в_цел(лит s, рез лог success) */
	case 0x0022: {
		Variable sf = valuesStack_.pop().toReference();
		const String x = valuesStack_.pop().toString();
		bool ok;
		int y = Kumir::Converter::stringToInt(x, ok);
		valuesStack_.push(Variable(y));
		sf.setValue(AnyValue(ok));
		break;
	}
	/* алг сим символ(цел n) */
	case 0x0023: {
		int x = valuesStack_.pop().toInt();
		Char y = Kumir::StringUtils::symbol(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг сим символ2(цел n) */
	case 0x0024: {
		int x = valuesStack_.pop().toInt();
		Char y = Kumir::StringUtils::unisymbol(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг лит цел_в_лит(цел n) */
	case 0x0025: {
		int x = valuesStack_.pop().toInt();
		String y = Kumir::Converter::intToString(x);
		valuesStack_.push(Variable(y));
		break;
	}
	/* алг цел юникод(сим ch) */
	case 0x0026: {
		Char x = valuesStack_.pop().toChar();
		int y = Kumir::StringUtils::unicode(x);
		valuesStack_.push(Variable(y));
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг цел Цел(лит строка, цел по умолчанию) */
	case 0x0027: {
		int def = valuesStack_.pop().toInt();
		String sv = valuesStack_.pop().toString();
		int v = Kumir::Converter::stringToIntDef(sv, def);
		valuesStack_.push(Variable(v));
		break;
	}
	/* алг вещ Вещ(лит строка, вещ по умолчанию) */
	case 0x0028: {
		Kumir::real def = valuesStack_.pop().toReal();
		String sv = valuesStack_.pop().toString();
		Kumir::real v = Kumir::Converter::stringToRealDef(sv, def);
		valuesStack_.push(Variable(v));
		break;
	}
	/* алг лог Лог(лит строка, лог по умолчанию) */
	case 0x0029: {
		bool def = valuesStack_.pop().toBool();
		String sv = valuesStack_.pop().toString();
		bool v = Kumir::Converter::stringToBoolDef(sv, def);
		valuesStack_.push(Variable(v));
		break;
	}
	/* алг @поставить оценку(цел значение) */
	case 0x002A: {
		int value = valuesStack_.pop().toInt();
		if (value < 0) {
			value = 0;
		}
		if (value > 10) {
			value = 10;
		}
		evaluationResult_ = static_cast<uint8_t>(value);
		break;
	}
	/* алг НАЗНАЧИТЬ ВВОД(лит имя файла) */
	case 0x0030: {
		const String x = valuesStack_.pop().toString();
		Kumir::Files::assignInStream(x);
		error_ = Kumir::Core::getError();
		break;
	}
	/* алг НАЗНАЧИТЬ ВЫВОД(лит имя файла) */
	case 0x0031: {
		const String x = valuesStack_.pop().toString();
		Kumir::Files::assignOutStream(x);
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
