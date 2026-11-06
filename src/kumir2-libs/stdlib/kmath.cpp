#include "kumirstdlib.hpp"

#if defined(WIN32) || defined(_WIN32)
# define NOMINMAX
# include <Windows.h>
#else
# include <sys/time.h>
#endif

#include <assert.h>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <float.h>
#include <string.h>

#include <time.h>
#if !defined(_WIN32) || defined(__MINGW32__) 
#include <sys/time.h>
#endif

#if !defined(APPLE) && !defined(__MINGW32__)
#include <random>
static std::random_device rd;
#endif

namespace Kumir
{

void Random::init()
{
#if defined(APPLE)
	unsigned int seed = 0xDEADBEEF;
	FILE *urandom = fopen("/dev/urandom", "rb");
	fread(&seed, 1, sizeof(seed), urandom);
	fclose(urandom);
	srand(seed);
#elif defined(__MINGW32__)
	struct timeval tv;
	gettimeofday(&tv, 0);
	unsigned int seed = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	srand(seed);
#endif
}

inline unsigned int Random::get_max()
{
#if defined(APPLE) || defined(__MINGW32__)
	return RAND_MAX;
#else
	return std::random_device::max() - std::random_device::min();
#endif
}


unsigned int Random::get_sample()
{
#if defined(APPLE) || defined(__MINGW32__)
	return rand();
#else
	return rd() - rd.min();
#endif
}


int Random::irand(int a, int b)
{
	if (a >= b) {
		if (a > b) {
			Core::abort(Core::fromUtf8("Неверный диапазон чисел"));
			return 0;
		}
		return a;
	}

	unsigned int d = b - a, rd = get_max();
	if ((d + 1 == 0) || (d > rd)) {
		Core::abort(Core::fromUtf8("Слишком широкий диапазон чисел"));
		return 0;
	}

	d += 1;
	unsigned int rq = rd / d + (rd % d + 1) / d;
	assert (0 < rq);

	for (;;) {
		unsigned int v = get_sample(), res = v / rq;
		if (res < d) {
			return a + res;
		}
	}
}

int Random::irnd(int x)
{
	return irand(1, x);
}


real Random::rrand(real a, real b)
{
	if (!(a < b)) {
		if (!(a <= b)) {
			Core::abort(Core::fromUtf8("Неверный диапазон чисел"));
			return 0.0;
		}
		return a;
	}

	real d = b - a;
	if (!std::isfinite(d)) {
		Core::abort(Core::fromUtf8("Слишком широкий диапазон чисел"));
		return 0.0;
	}

	real rd = get_max() + 1.0;
	real v = get_sample() + 0.5;
	real res = v / rd * d;
	if (d < res) {
		res = d;
	}
	return a + res;
}

real Random::rrnd(real x)
{
	return rrand(0, x);
}


int System::time()
{
	int hours = 0, mins = 0, secs = 0, msecs = 0;

#if defined(WIN32) || defined(_WIN32)
	SYSTEMTIME st;
	memset(&st, 0, sizeof(SYSTEMTIME));
	GetSystemTime(&st);
	hours = st.wHour;
	mins  = st.wMinute;
	secs  = st.wSecond;
	msecs = st.wMilliseconds;
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	tzset();
	struct tm dummy;
	time_t epoch = ::time(NULL);
	struct tm *loc = localtime_r(&epoch, &dummy);
	hours = loc->tm_hour;
	mins  = loc->tm_min;
	secs  = loc->tm_sec;
	msecs = tv.tv_usec / 1000;
#endif

	int result = ((hours * 60 + mins) * 60 + secs) * 1000 + msecs;
	return result % 86400000;
}

// Check for integer overflow
// CLang completely removes the standard check technique due to optimization.
// Use another method: cast to 64 bit

bool Math::checkSumm(int32_t l, int32_t r)
{
	int64_t res = (int64_t) l + (int64_t) r;
	bool result = (INT32_MIN <= res && res <= INT32_MAX);
	return result;
}

bool Math::checkDiff(int32_t l, int32_t r)
{
	int64_t res = (int64_t) l - (int64_t) r;
	bool result = (INT32_MIN <= res && res <= INT32_MAX);
	return result;
}

bool Math::checkProd(int32_t l, int32_t r)
{
	int64_t res = (int64_t) l * (int64_t) r;
	bool result = (INT32_MIN <= res && res <= INT32_MAX);
	return result;
}

bool Math::isCorrectDouble(double x)
{
	// !!! WARNING !!
	// this works ONLY for IEEE754-compatible representation!
	union DoubleU64 {
		double d;
		uint64_t u;
	} v;
	v.d = x;
	uint32_t expMask  = 0x7FF;
	uint32_t exponent = (v.u >> 52) & expMask;
	return exponent != expMask;
}

real Math::maxreal()
{
	return 1.7976931348623157e+308;
}

real Math::abs(real x)
{
	return ::fabs(x);
}

int Math::iabs(int x)
{
	unsigned int y = (unsigned int) x;
	if (y == 0x80000000U) {
		Core::abort(L"Целочисленное переполнение");
		return 0;
	}

	return x >= 0 ? x : -x;
}

int Math::intt(real x)
{
	double y = ::floor((double) x);
	if (!(INT32_MIN <= y && y <= INT32_MAX)) {
		Core::abort(L"Целочисленное переполнение");
		return 0;
	}
	return (int) y;
}

real Math::arcsin(real x)
{
	if (-1.0 <= x && x <= 1.0) {
		return ::asin(x);
	} else {
		Core::abort(L"Неверный аргумент обратной тригонометрической функции");
		return 0.0;
	}
}

real Math::arccos(real x)
{
	if (-1.0 <= x && x <= 1.0) {
		return ::acos(x);
	} else {
		Core::abort(L"Неверный аргумент обратной тригонометрической функции");
		return 0.0;
	}
}

real Math::arctg(real x)
{
	real res = ::atan(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент обратной тригонометрической функции");
		return 0.0;
	}
}

real Math::arcctg(real x)
{
	real res = ::atan2(1.0, x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент обратной тригонометрической функции");
		return 0.0;
	}
}


real Math::sin(real x)
{
	real res = ::sin(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент тригонометрической функции");
		return 0.0;
	}
}

real Math::cos(real x)
{
	real res = ::cos(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент тригонометрической функции");
		return 0.0;
	}
}

real Math::tg(real x)
{
	real res = ::tan(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент тригонометрической функции");
		return 0.0;
	}
}

real Math::ctg(real x)
{
	real res = ::cos(x) / ::sin(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент тригонометрической функции");
		return 0.0;
	}
}

real Math::exp(real x)
{
	real res = ::exp(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент экспоненты");
		return 0.0;
	}
}

real Math::ln(real x)
{
	if (!(x > 0.0)) {
		Core::abort(Core::fromUtf8("Логарифм от не положительного числа"));
		return 0.0;
	}
	real res = ::log(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент логарифма");
		return 0.0;
	}
}

real Math::lg(real x)
{
	if (!(x > 0.0)) {
		Core::abort(Core::fromUtf8("Логарифм от не положительного числа"));
		return 0.0;
	}
	real res = ::log10(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Неверный аргумент логарифма");
		return 0.0;
	}
}

real Math::sqrt(real x)
{
	if (!(x > 0.0)) {
		if (x != 0.0)
			Core::abort(L"Нельзя извлечь квадратный корень из отрицательного числа");
		return 0.0;
	}
	real res = ::sqrt(x);
	if (isCorrectReal(res)) {
		return res;
	} else {
		Core::abort(L"Квадратный корень из бесконечности слишком велик");
		return 0.0;
	}
}

real Math::pow(real a, real b)
{
	real res = ::pow(a, b);
	if (!Math::isCorrectReal(res)) {
		Core::abort(L"Ошибка возведения в степень");
		return 0.0;
	}
	return res;
}

int Math::ipow(int a, int b)
{
	static const String e_unwhole = L"Результат - не целое число";
	static const String e_ovfl = L"Целочисленное переполнение";
	static const String e_divz = L"Деление на ноль";

	if (b <= 1) {
		if (b == 1) {
			return a;
		}
		if (b == 0) {
			return 1;
		}
		if (a != 1 && a != -1) {
			Core::abort(a == 0 ? e_divz : e_unwhole);
			return 0;
		}
	}

	if (a == 0 || a == 1) {
		return a;
	}

	if (a == -1) {
		return (b & 1) ? -1 : 1;
	}

	if (a == 2) {
		if (31 <= b) {
			Core::abort(e_ovfl);
			return 0;
		}
		return 1 << b;
	}

	if (a == -2) {
		if (31 < b) {
			Core::abort(e_ovfl);
			return 0;
		}
		unsigned int res = 1U << b;
		return (b & 1) ? 0 - res : res;
	}

	int64_t y = a, res = 1;
	bool ok = true;
	while (0 < b && ok) {
		if (b & 1) {
			res *= y;
			ok = ok && (INT32_MIN <= res && res <= INT32_MAX);
		}
		b /= 2;
		if (b == 0) {
			break;
		}
		y *= y;
		ok = ok && (INT32_MIN <= y && y <= INT32_MAX);
	}

	if (!ok) {
		Core::abort(e_ovfl);
		return 0;
	}

	return (int) res;
}

int Math::div(int a, int b)
{
	if (b <= 0) {
		Core::abort(L"Деление на ненатуральное число");
		return 0;
	}

	int q = a / b, r = a % b;
	if (r < 0) {
		q--;
	}
	return q;
}

int Math::mod(int a, int b)
{
	if (b <= 0) {
		Core::abort(L"Деление на ненатуральное число");
		return 0;
	}

	int r = a % b;
	if (r < 0) {
		r += b;
	}
	return r;
}


bool Converter::validDecimal(const String &astring)
{
	static const String validSymbols = Core::fromAscii("0123456789");
	for (size_t i = 0; i < astring.length(); i++) {
		if (i == 0 && (astring[i] == '-' || astring[i] == '+')) {
			continue;
		}
		if (validSymbols.find(astring[i]) == String::npos) {
			return false;
		}
	}
	return true;
}

real Converter::fromDecimal(const String &astring)
{
	real result = 0;
	real power = 1;
	size_t index;
	real digit;
	static const String digits = Core::fromAscii("0123456789");
	for (int i = astring.length() - 1; i >= 0; i--) {
		if (i == 0 && astring[i] == '-') {
			result = -1 * result;
			break;
		}
		if (i == 0 && astring[i] == '+') {
			break;
		}
		index = digits.find(astring[i]);
		if (index == String::npos) {
			return 0.0;
		}
		digit = static_cast<real>(index);
		result += power * digit;
		power *= 10;
	}
	return result;
}

int Converter::parseInt(String word, unsigned int base, ParseError &error)
{
	error = NoError;
	size_t l = word.length(), pos = 0;

	if (l == 0) {
		error = EmptyWord;
		return 0;
	}

	bool negative = false;

	if (word.at(pos) == Char('-')) {
		negative = true;
		pos += 1;
	} else if (word.at(pos) == Char('+')) {
		pos += 1;
	}

	if (base == 0) {
		if (pos < l && word.at(pos) == Char('$')) {
			base = 16;
			pos += 1;
		} else {
			base = 10;
		}
	}
	if (base <= 1 || base > 36) {
		fprintf(stderr, "Bad conversion base %u, setting to 10 \n", base);
		base = 10;
	}

	if (l == pos) {
		error = EmptyWord;
		return 0;
	}

	unsigned int maxabs = (1U << 31) - (negative ? 0 : 1);
	unsigned int maxabsb = maxabs / base;
	unsigned int result = 0;
	bool overflow = false;

	for (size_t i = pos; i < l; i++) {
		Char ch = word.at(i);
		unsigned int digit = base;

		if ('0' <= ch && ch <= '9') {
			digit = (unsigned int)(ch) - '0';
		} else if ('A' <= ch && ch <= 'Z') {
			digit = 10 + (unsigned int)(ch) - 'A';
		} else if ('a' <= ch && ch <= 'z') {
			digit = 10 + (unsigned int)(ch) - 'a';
		}

		if (base <= digit) {
			error = BadSymbol;
			return 0;
		}

		overflow = overflow || (maxabsb < result);
		result = result * base + digit;
		overflow = overflow || (result < digit);
	}

	overflow = overflow || (maxabs < result);

	if (overflow) {
		error = Overflow;
		return 0;
	}

	return negative ? -result : result;
}

bool Converter::isCorrectIntegerConstant(const String &v)
{
	ParseError e = NoError;
	parseInt(v, 0, e);
	return e == NoError;
}

real Converter::parseReal(String word, Char dot, ParseError &error)
{
	error = NoError;
	if (word.length() == 0) {
		error = EmptyWord;
		return 0;
	}
	size_t pos = 0;
	bool negative = false;
	if (word.at(pos) == Char('-')) {
		negative = true;
		pos += 1;
	} else if (word.at(pos) == Char('+')) {
		pos += 1;
	}
	real mantissa = 0.0;
	real exponenta = 0.0;
	real fraction = 0.0;
	String sIntegral, sFractional, sExponenta;
	sIntegral.reserve(30);
	sFractional.reserve(30);
	sExponenta.reserve(30);
	bool dotFound = false;
	bool eFound = false;
	static const String E = Core::fromUtf8("eEеЕ"); // includes cyrillic 'e'
	bool hasE = false;
	for (size_t i = 0; i < E.length(); i++) {
		if (!hasE) {
			hasE = word.find(E[i]) != String::npos;
		}
		if (hasE) {
			break;
		}
	}
	for (size_t i = pos; i < word.length(); i++) {
		Char ch = word.at(i);
		if (!dotFound) {
			// parse integral part of mantissa
			if (ch == dot || E.find_first_of(ch) != String::npos) {
				if (ch == dot) {
					dotFound = true;
				} else {
					dotFound = true;
					eFound = true;
				}

			} else {
				sIntegral.push_back(ch);
			}
		} else if (!eFound) {
			// parse fractional part of mantissa
			if (E.find_first_of(ch) != String::npos) {
				if (sFractional.length() > 0) {
					if (!validDecimal(sFractional)) {
						error = hasE ? WrongExpForm : WrongReal;
						return 0.0;
					}
					fraction = fromDecimal(sFractional);
				}
				if (fraction < 0) {
					error = hasE ? WrongExpForm : WrongReal;
					return 0.0;
				}
				eFound = true;
			} else {
				sFractional.push_back(ch);
			}
		} else {
			// parse exponenta
			sExponenta.push_back(ch);
		}
	}
	if (hasE && sExponenta.length() == 0) {
		error = WrongExpForm;
		return 0.0;
	}
	if (sIntegral.length() + sFractional.length() == 0) {
		error = sExponenta.length() > 0 ? WrongExpForm : WrongReal;
		return 0.0;
	}
	int fractionalLength = sFractional.length();
	for (int i = sFractional.length() - 1; i >= 0; i--) {
		Char ch = sFractional.at(i);
		if (ch == Char('0')) {
			fractionalLength -= 1;
		} else {
			break;
		}
	}
	if (!validDecimal(sIntegral) || !validDecimal(sFractional) || !validDecimal(sExponenta)) {
		error = WrongReal;
		return 0.0;
	}

	if (sExponenta.length() > 0 && sExponenta.find(Char('.')) != String::npos) {
		error = WrongExpForm;
		return 0.0;
	}

	fraction = fromDecimal(sFractional.substr(0, fractionalLength));
	for (int i = 0; i < fractionalLength; i++) {
		fraction /= 10.0;
	}
	mantissa = fromDecimal(sIntegral);
	if (mantissa < 0) {
		// Extra '-' at start
		error = WrongReal;
		return 0.0;
	}
	mantissa += fraction;
	exponenta = fromDecimal(sExponenta);
	real result = mantissa * ::pow(10, exponenta);
	if (negative && result != 0) {
		result *= -1;
	}
	if (!Math::isCorrectReal(result)) {
		error = Overflow;
	}
	return result;
}

String Converter::sprintfInt(int value, unsigned int base, unsigned int width, char al)
{
	String result;
	result.reserve(10);
	static const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	bool negative = value < 0;
	unsigned int va = negative ? -value : value;
#if 0
	fprintf(stderr, "%s:%d value=%d, base=%u, width = %u, char = %d \n",
		__FILE__, __LINE__, value, base, width, (int) al);
#endif
	if (base <= 1 || base > 36) {
		fprintf(stderr, "Bad conversion base %u, setting to 10 \n", base);
		base = 10;
	}

	do {
		result.push_back(digits[va % base]);
		va /= base;
	} while (va);

	if (negative) {
		result.push_back('-');
	}

	std::reverse(result.begin(), result.end());

	unsigned int len = result.length();
	if (width > len) {
		unsigned int leftSpaces = 0;
		unsigned int rightSpaces = 0;
		if (al == 'l') {
			rightSpaces = width - len;
		} else if (al == 'r') {
			leftSpaces = width - len;
		} else {
			leftSpaces = (width - len) / 2;
			rightSpaces = width - len - leftSpaces;
		}
		if (leftSpaces > 0) {
			result.insert(0, leftSpaces, ' ');
		}
		if (rightSpaces > 0) {
			result.append(rightSpaces, ' ');
		}
	}
	return result;
}

String Converter::sprintfReal(real value, unsigned int prec)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*g", prec, value);
    return String(buf, buf + strlen(buf));
}

String Converter::sprintfReal(
	real value, Char dot, bool expform,
	unsigned int width, int decimals, char al
)
{
	(void) dot;
	int sdecimals = decimals;
	if (sdecimals < 0) {
		double absVal = fabs(double(value));
		sdecimals = 13;
		if (0 < absVal && absVal <= DBL_MAX) {
			if (absVal < 1e-4 || absVal >= 1e6) {
				expform = true;
			}
			if (!expform) {
				sdecimals -= (int) ::floor(::log(absVal) / ::log(10.0));
			}
		}
	}

	std::stringstream stream;
	if (expform) {
		stream << std::scientific;
	} else {
		stream << std::fixed;
	}
	stream.precision(sdecimals < 0 ? 6 : sdecimals);
	stream << value;

	std::string rpart = stream.str();
	std::string expPart;

	if (expform) {
		size_t ePos = rpart.find('e');
		if (std::string::npos == ePos) {
			ePos = rpart.find('E');
		}
		expPart = rpart.substr(ePos);
		rpart.resize(ePos);
	}

	// Replace ',' with '.' (for some locales like Russian)
	size_t dotPos = rpart.find(',');
	if (std::string::npos != dotPos) {
		rpart.replace(dotPos, 1, ".");
	}
	if ((expform || 0 > decimals) && std::string::npos != rpart.find('.')) {
		while (rpart.size() > 1 && '0' == rpart.at(rpart.size() - 1)) {
			rpart.resize(rpart.length() - 1);
		}
		if ('.' == rpart.at(rpart.length() - 1)) {
			if (expform) {
				rpart.resize(rpart.length() - 1);
			} else {
				rpart.push_back('0');
			}
		}
	}
	std::string ascii = rpart + expPart;
	unsigned int len = ascii.size();
	if (width > len) {
		if ('r' == al) {
			ascii.insert(0, width - len, ' ');
		} else {
			ascii.append(width - len, ' ');
		}
	}

	EncodingError encodingError;
	String result = Coder::decode(ASCII, ascii, encodingError);
	return result;
}

bool Converter::stringToBoolDef(const String &str, bool def)
{
	String v = Core::toLowerCaseW(str);
	static const String trues[5] = {
		Core::fromAscii("true"),
		Core::fromAscii("yes"),
		Core::fromAscii("1"),
		Core::fromUtf8("да"),
		Core::fromUtf8("истина")
	};
	static const String falses[5] = {
		Core::fromAscii("false"),
		Core::fromAscii("no"),
		Core::fromAscii("0"),
		Core::fromUtf8("нет"),
		Core::fromUtf8("ложь")
	};

	for (size_t i = 0; i < 5; i++) {
		const String &t = trues[i];
		const String &f = falses[i];
		if (v == t) {
			return true;
		}
		if (v == f) {
			return false;
		}
	}
	return def;
}


} // namespace Kumir

