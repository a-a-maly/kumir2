/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

#include "complexnumbersmodule.h"
#include <kumir2-libs/stdlib/kumirstdlib.hpp>
#include <math.h>

namespace ActorComplexNumbers {

ComplexNumbersModule::ComplexNumbersModule(
	ExtensionSystem::KPlugin *parent
) :
	ComplexNumbersModuleBase(parent)
{
	/* TODO 
	implement class Constructor
	*/
}

QList<ExtensionSystem::CommandLineParameter> ComplexNumbersModule::acceptableCommandLineParameters()
{
	return QList<ExtensionSystem::CommandLineParameter>();
}

void ComplexNumbersModule::reloadSettings(
	ExtensionSystem::SettingsPtr,
	const QStringList &
) {
}

void ComplexNumbersModule::changeGlobalState(
	ExtensionSystem::GlobalState,
	ExtensionSystem::GlobalState
) {
}

void ComplexNumbersModule::reset()
{
	/* TODO
	This method is called when actor resets its state before program starts.
	*/
}

void ComplexNumbersModule::terminateEvaluation()
{
}

void ComplexNumbersModule::setAnimationEnabled(bool enabled)
{
	/* TODO
	Set internal flag for animation
	*/
}

bool ComplexNumbersModule::check(double &x)
{
	if (std::isnan(x)) {
		setError(QString::fromUtf8("Недопустимое значение"));
        x = 0.0;
        return false;
	}
    return true;
}

bool ComplexNumbersModule::check(Complex &x)
{
	if (std::isnan(x.re) || std::isnan(x.im)) {
		setError(QString::fromUtf8("Недопустимое значение"));
        x.re = x.im = 0.0;
        return false;
	}
    return true;
}

qreal ComplexNumbersModule::runRe(const Complex& x)
{
	return x.re;
}

qreal ComplexNumbersModule::runIm(const Complex& x)
{
	return x.im;
}

Complex ComplexNumbersModule::runConj(const Complex& x)
{
	return Complex{x.re, -x.im};
}


qreal ComplexNumbersModule::runCabs(const Complex& x)
{
    double res = std::abs(fromComplex(x));
    check(res);
	return res;
}

qreal ComplexNumbersModule::runCarg(const Complex& x)
{
    double res = std::arg(fromComplex(x));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCpolar(qreal r, qreal arg)
{
    dcomplex z = std::polar(r, arg);
	Complex res = toComplex(z);
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCsqrt(const Complex &x)
{
	Complex res = toComplex(sqrt(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCexp(const Complex &x)
{
	Complex res = toComplex(exp(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCln(const Complex &x)
{
	Complex res = toComplex(log(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runClg(const Complex &x)
{
	Complex res = toComplex(log10(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCsin(const Complex &x)
{
	Complex res = toComplex(sin(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCcos(const Complex &x)
{
	Complex res = toComplex(cos(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCtan(const Complex &x)
{
	Complex res = toComplex(tan(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCasin(const Complex &x)
{
	Complex res = toComplex(asin(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCacos(const Complex &x)
{
	Complex res = toComplex(acos(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runCatan(const Complex &x)
{
	Complex res = toComplex(atan(fromComplex(x)));
    check(res);
	return res;
}

Complex ComplexNumbersModule::runOperatorPLUS(const Complex &x, const Complex &y)
{
	Complex res {x.re + y.re, x.im + y.im};
    check(res);
	return res;
}

Complex ComplexNumbersModule::runOperatorMINUS(const Complex& x, const Complex& y)
{
	Complex res {x.re - y.re, x.im - y.im};
    check(res);
	return res;
}

Complex ComplexNumbersModule::runOperatorASTERISK(const Complex& _x, const Complex& _y)
{
    dcomplex x = fromComplex(_x), y = fromComplex(_y);
    Complex res = toComplex(x * y);
    check(res);
    return res;
}

Complex ComplexNumbersModule::runOperatorSLASH(const Complex& _x, const Complex& _y)
{
	if (_y.re==0 && _y.im==0) {
		setError(QString::fromUtf8("Деление на комплексный нуль"));
		return Complex{0.0, 0.0};
	}

    dcomplex x = fromComplex(_x), y = fromComplex(_y);
    Complex res = toComplex(x / y);
    check(res);
    return res;
}

Complex ComplexNumbersModule::runOperatorPOWER(const Complex& _x, const Complex& _y)
{
    dcomplex x = fromComplex(_x), y = fromComplex(_y);
    Complex res = toComplex(pow(x, y));
    check(res);
    return res;
}


Complex ComplexNumbersModule::runOperatorASSIGN(int x)
{
	Complex res {(double)x, 0.0};
	return res;
}

Complex ComplexNumbersModule::runOperatorASSIGN(qreal x)
{
	Complex res {x, 0.0};
	return res;
}

Complex ComplexNumbersModule::runOperatorINPUT(const QString& x, bool& ok)
{
	static const QRegExp rxPlusMinus("[+-]");
	int p = x.indexOf(rxPlusMinus);
	QString re = "0.0", im = "0.0";
	if (p==-1) {
		if (x.trimmed().endsWith(QChar('i'), Qt::CaseInsensitive))
			im = x.trimmed();
		else
			re = x.trimmed();
	} else {
		QString a = x.left(p).trimmed();
		QString b = x.mid(p+1).trimmed();
		if (a.endsWith(QChar('i'), Qt::CaseInsensitive)) {
			im = a;
			re = x[p]+b;
		} else {
			im = x[p]+b;
			re = a;
		}
	}

	im.remove(QChar('i'), Qt::CaseInsensitive);
	im = im.trimmed();
	if (im.endsWith("*")) {
		im = im.left(im.length()-1);
		im = im.trimmed();
	}
	if (im.isEmpty()) {
		im = "1.0";
	}

	Complex result;
	bool ok1 = false, ok2 = false;
	result.re = re.toDouble(&ok1);
	result.im = im.toDouble(&ok2);
	ok = ok1 && ok2;
	return result;
}

QString ComplexNumbersModule::runOperatorOUTPUT(const Complex& x)
{
	QString re = QString::fromStdWString(Kumir::Converter::sprintfReal(x.re, 16));
	QString im = QString::fromStdWString(Kumir::Converter::sprintfReal(x.im, 16));

	QString result = re + (im.startsWith('-') ? "" : "+") + im + "*i";
	return result;
}

} // namespace ActorComplexNumbers
