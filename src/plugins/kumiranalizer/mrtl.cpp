#include "mrtl.h"

namespace StdLibModules
{
	RTL::FunctionList RTL::functionList() const
	{
		// fprintf(stderr, "RTL::functionList \n");

		FunctionList result;
		Function func;
		func.accessType = PublicFunction;

		/* алг вещ abs(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0000;
		func.asciiName = "abs";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ arccos(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0001;
		func.asciiName = "arccos";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ arcctg(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0002;
		func.asciiName = "arcctg";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ arcsin(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0003;
		func.asciiName = "arcsin";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ arctg(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0004;
		func.asciiName = "arctg";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ cos(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0005;
		func.asciiName = "cos";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ ctg(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0006;
		func.asciiName = "ctg";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг ждать(цел x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0007;
		func.asciiName = "wait";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("ждать");
		func.returnType = Void;
		func.arguments << Argument("ms", Int);
		result << func;

		/* алг цел div(цел x, цел y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0008;
		func.asciiName = "div";
		func.returnType = Int;
		func.arguments << Argument("x", Int) << Argument("y", Int);
		result << func;

		/* алг вещ exp(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0009;
		func.asciiName = "exp";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг цел iabs(цел x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x000a;
		func.asciiName = "iabs";
		func.returnType = Int;
		func.arguments << Argument("x", Int);
		result << func;

		/* алг цел imax(цел x, цел y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x000b;
		func.asciiName = "imax";
		func.returnType = Int;
		func.arguments << Argument("x", Int) << Argument("y", Int);
		result << func;

		/* алг цел imin(цел x, цел y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x000c;
		func.asciiName = "imin";
		func.returnType = Int;
		func.arguments << Argument("x", Int) << Argument("y", Int);
		result << func;

		/* алг цел int(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x000d;
		func.asciiName = "int";
		func.returnType = Int;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг цел irand(цел x, цел y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x000e;
		func.asciiName = "irand";
		func.returnType = Int;
		func.arguments << Argument("x", Int) << Argument("y", Int);
		result << func;

		/* алг цел irnd(цел x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x000f;
		func.asciiName = "irnd";
		func.returnType = Int;
		func.arguments << Argument("x", Int);
		result << func;

		/* алг вещ lg(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0010;
		func.asciiName = "lg";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ ln(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0011;
		func.asciiName = "ln";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ max(вещ x, вещ y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0012;
		func.asciiName = "max";
		func.returnType = Real;
		func.arguments << Argument("x", Real) << Argument("y", Real);
		result << func;

		/* алг вещ min(вещ x, вещ y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0013;
		func.asciiName = "min";
		func.returnType = Real;
		func.arguments << Argument("x", Real) << Argument("y", Real);
		result << func;

		/* алг цел mod(цел x, цел y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0014;
		func.asciiName = "mod";
		func.returnType = Int;
		func.arguments << Argument("x", Int) << Argument("y", Int);
		result << func;

		/* алг вещ rand(вещ x, вещ y) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0015;
		func.asciiName = "rand";
		func.returnType = Real;
		func.arguments << Argument("x", Real) << Argument("y", Real);
		result << func;

		/* алг вещ rnd(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0016;
		func.asciiName = "rnd";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг цел sign(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0017;
		func.asciiName = "sign";
		func.returnType = Int;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ sin(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0018;
		func.asciiName = "sin";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ sqrt(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0019;
		func.asciiName = "sqrt";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ tg(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x001a;
		func.asciiName = "tg";
		func.returnType = Real;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг вещ МАКСВЕЩ */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x001b;
		func.asciiName = "MAXREAL";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("МАКСВЕЩ");
		func.returnType = Real;
		result << func;

		/* алг вещ МАКСЦЕЛ */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x001c;
		func.asciiName = "MAXINT";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("МАКСЦЕЛ");
		func.returnType = Int;
		result << func;

		/* алг лит вещ_в_лит(вещ x) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x001d;
		func.asciiName = "string_of_real";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("вещ_в_лит");
		func.returnType = String;
		func.arguments << Argument("x", Real);
		result << func;

		/* алг цел время */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x001e;
		func.asciiName = "time";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("время");
		func.returnType = Int;
		result << func;

		/* алг цел длин(лит s) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x001f;
		func.asciiName = "length";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("длин");
		func.returnType = Int;
		func.arguments << Argument("s", String);
		result << func;

		/* алг цел код(сим ch) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0020;
		func.asciiName = "code";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("код");
		func.returnType = Int;
		func.arguments << Argument("ch", Char);
		result << func;

		/* алг вещ лит_в_вещ(лит s, рез лог success) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0021;
		func.asciiName = "string_to_real";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("лит_в_вещ");
		func.returnType = Real;
		func.arguments << Argument("s", String) << Argument("success", Bool);
		func.arguments.last().accessType = OutArgument;
		result << func;

		/* алг цел лит_в_цел(лит s, рез лог success) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0022;
		func.asciiName = "string_to_int";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("лит_в_цел");
		func.returnType = Int;
		func.arguments << Argument("s", String) << Argument("success", Bool);
		func.arguments.last().accessType = OutArgument;
		result << func;

		/* алг сим символ(цел n) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0023;
		func.asciiName = "symbol";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("символ");
		func.returnType = Char;
		func.arguments << Argument("n", Int);
		result << func;

		/* алг сим юнисимвол(цел n) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0024;
		func.asciiName = "unisymbol";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("юнисимвол");
		func.returnType = Char;
		func.arguments << Argument("n", Int);
		result << func;

		/* алг лит цел_в_лит(цел n) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0025;
		func.asciiName = "string_of_int";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("цел_в_лит");
		func.returnType = String;
		func.arguments << Argument("n", Int);
		result << func;

		/* алг цел юникод(сим ch) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0026;
		func.asciiName = "unicode";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("юникод");
		func.returnType = Int;
		func.arguments << Argument("ch", Char);
		result << func;

		/* алг цел Цел(лит строка, цел по умолчанию) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0027;
		func.asciiName = "Integer";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("Цел");
		func.returnType = Int;
		func.arguments << Argument("lexem", String);
		func.arguments << Argument("defvalue", Int);
		result << func;

		/* алг вещ Вещ(лит строка, вещ по умолчанию) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0028;
		func.asciiName = "Real";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("Вещ");
		func.returnType = Int;
		func.arguments << Argument("lexem", String);
		func.arguments << Argument("defvalue", Real);
		result << func;

		/* алг лог Лог(лит строка, лог по умолчанию) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x0029;
		func.asciiName = "Boolean";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("Лог");
		func.returnType = Int;
		func.arguments << Argument("lexem", String);
		func.arguments << Argument("defvalue", Bool);
		result << func;

		/* алг @поставить оценку(цел значение) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.id = 0x002A;
		func.asciiName = "@Evaluate";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("@поставить оценку");
		func.returnType = Void;
		func.arguments << Argument("value", Int);
		result << func;

		/* алг НАЗНАЧИТЬ ВВОД(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0030;
		func.asciiName = "ASSIGN INPUT";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("НАЗНАЧИТЬ ВВОД");
		func.returnType = Void;
		func.arguments << Argument("path", String);
		result << func;

		/* алг НАЗНАЧИТЬ ВЫВОД(лит имя файла) */
		func.localizedNames.clear();
		func.arguments.clear();
		func.returnTypeSpecification = RecordSpecification();
		func.id = 0x0031;
		func.asciiName = "ASSIGN OUTPUT";
		func.localizedNames[QLocale::Russian] = QString::fromUtf8("НАЗНАЧИТЬ ВЫВОД");
		func.returnType = Void;
		func.arguments << Argument("path", String);
		result << func;

		return result;
	}

}

