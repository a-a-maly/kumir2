/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

#ifndef COMPLEXNUMBERSMODULE_H
#define COMPLEXNUMBERSMODULE_H

#include <kumir2-libs/extensionsystem/kplugin.h>
#include "complexnumbersmodulebase.h"
#include <complex>

namespace ActorComplexNumbers
{

class ComplexNumbersModule : public ComplexNumbersModuleBase
{
	Q_OBJECT
public:
	static QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters();

	// Constructor
	ComplexNumbersModule(ExtensionSystem::KPlugin *parent);

	void reloadSettings(
		ExtensionSystem::SettingsPtr,
		const QStringList &
	);

	void changeGlobalState(
		ExtensionSystem::GlobalState,
		ExtensionSystem::GlobalState
	);

public slots:
	// Reset actor state before program starts
	void reset();
	void terminateEvaluation();
	// Set animation flag
	void setAnimationEnabled(bool enabled);

	// Actor methods
	qreal runRe(const Complex &x);
	qreal runIm(const Complex &x);
	Complex runConj(const Complex &x);
	Complex runOperatorPLUS(const Complex &x, const Complex &y);
	Complex runOperatorMINUS(const Complex &x, const Complex &y);
	Complex runOperatorASTERISK(const Complex &x, const Complex &y);
	Complex runOperatorSLASH(const Complex &x, const Complex &y);
	Complex runOperatorASSIGN(const int x);
	Complex runOperatorASSIGN(const qreal x);
	Complex runOperatorINPUT(const QString &x, bool &ok);
	QString runOperatorOUTPUT(const Complex &x);
}; // ComplexNumbersModule

typedef std::complex<double> dcomplex;

static inline Complex toComplex(dcomplex x)
{
    return Complex{x.real(), x.imag()};
}

static inline dcomplex fromComplex(Complex x)
{
    return dcomplex(x.re, x.im);
}

} // ActorComplexNumbers

#endif // COMPLEXNUMBERSMODULE_H
