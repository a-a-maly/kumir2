#include "utils.h"

#include <QApplication>
#include <QLocale>

#ifdef Q_OS_WIN32
#   include <windows.h>
#endif

#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
#   include <QX11Info>
#   include <X11/XKBlib.h>
#   include <X11/Xatom.h>
#endif

#ifdef Q_OS_MACX
#   include "mac-util.h"
#endif

namespace Editor
{

#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
static Display *x11Display()
{
#if QT_VERSION >= 0x050200
	if (!QX11Info::isPlatformX11()) {
		return 0;
	}
#endif
	return QX11Info::display();
}

#if QT_VERSION >= 0x050000
static QString x11LayoutName(Display *d)
{
	XkbStateRec xkbState;
	if (XkbGetState(d, XkbUseCoreKbd, &xkbState) != Success) {
		return QString();
	}
	int group = xkbState.group;

	// solution found at: http://www.linux.org.ru/forum/development/12852504
	Atom real_prop_type = None;
	int fmt = 0;
	unsigned long nitems = 0, extra_bytes = 0;
	unsigned char *prop_data = NULL;
	Atom rules_atom = XInternAtom(d, "_XKB_RULES_NAMES", False);
	int status = XGetWindowProperty(d, DefaultRootWindow(d), rules_atom, 0L, 1024,
		False, XA_STRING, &real_prop_type, &fmt, &nitems, &extra_bytes, &prop_data);
	QStringList names;
	if (status == Success && prop_data && real_prop_type == XA_STRING && fmt == 8) {
		const char *data = (const char *) prop_data;
		for (const char *p = data; p - data < (long)nitems; p += strlen(p) + 1) {
			names.append(QString::fromLatin1(p));
		}
	}
	if (prop_data) {
		XFree(prop_data);
	}
	if (names.count() > 2) {
		QStringList layouts = names[2].split(",");
		if (group < layouts.count()) {
			return layouts[group];
		}
	}

	QString result;
	XkbDescPtr desc = XkbAllocKeyboard();
	if (!desc) {
		return result;
	}
	if (XkbGetNames(d, XkbGroupNamesMask, desc) == Success && desc->names
		&& group < XkbNumKbdGroups && desc->names->groups[group] != None
	) {
		char *name = XGetAtomName(d, desc->names->groups[group]);
		if (name) {
			result = QString::fromLatin1(name);
			XFree(name);
		}
	}
	XkbFreeKeyboard(desc, 0, True);
	return result;
}
#endif
#endif // X11

bool Utils::isRussianLayout()
{
	bool result = false;
#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
#if QT_VERSION < 0x050000
	// Qt4 implementation
	QString keybLang = QApplication::keyboardInputLocale().name();
	result = keybLang.contains("ru");
#else
	// Qt5 implementation
	QInputMethod *inputMethod = QApplication::inputMethod();
	QString keybLang = inputMethod ? inputMethod->locale().name() : QString();
	result = keybLang.contains("ru");
	Display *d = x11Display();
	if (d && !result) {
		keybLang = x11LayoutName(d);
		result = keybLang.contains("ru") || keybLang.contains("Russian");
	}
#endif
#endif // X11

#ifdef Q_OS_WIN32
	HKL l = GetKeyboardLayout(0);
	result = unsigned(l) == 0x4190419;
#endif
#ifdef Q_OS_MACX
	result = EditorMacUtil::isRussianLayout();
#endif
	return result;
}

bool Utils::isCapsLock()
{
	bool result = false;
#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
	Display *d = x11Display();
	if (d) {
		unsigned n = 0;
		if (XkbGetIndicatorState(d, XkbUseCoreKbd, &n) == Success) {
			result = (n & 0x01) == 1;
		}
	}
#endif
#ifdef Q_OS_WIN32
	result = GetKeyState(VK_CAPITAL) == 1;
#endif
#ifdef Q_OS_MACX
	result = EditorMacUtil::isCapsLock();
#endif
	return result;
}

bool Utils::altKeyPressed = false;
bool Utils::shiftKeyPressed = false;

QString Utils::textByKey(Qt::Key key,
	const QString &sourceKeyText,
	bool shiftPressed,
	bool /*teacherMode*/
)
{

	Q_UNUSED(shiftPressed);
	QString keyText = sourceKeyText;
	if (altKeyPressed && key == Qt::Key_1) {
		keyText = "|";
	} else if (/*teacherMode && */altKeyPressed && key == Qt::Key_2) {
		keyText = "@";
	} else if (/*teacherMode && */altKeyPressed && key == Qt::Key_3) {
		keyText = "#";
	} else if (/*teacherMode && */altKeyPressed && key == Qt::Key_5) {
		keyText = "%";
	} else if (altKeyPressed && key == Qt::Key_Equal) {
		keyText = ":=";
	} else if (altKeyPressed && isRussianLayout()) {
		if (keyText.toLower() == QString::fromUtf8("й")) {
			keyText = "q";
		} else if (keyText.toLower() == QString::fromUtf8("ц")) {
			keyText = "w";
		} else if (keyText.toLower() == QString::fromUtf8("у")) {
			keyText = "e";
		} else if (keyText.toLower() == QString::fromUtf8("к")) {
			keyText = "r";
		} else if (keyText.toLower() == QString::fromUtf8("е")) {
			keyText = "t";
		} else if (keyText.toLower() == QString::fromUtf8("н")) {
			keyText = "y";
		} else if (keyText.toLower() == QString::fromUtf8("г")) {
			keyText = "u";
		} else if (keyText.toLower() == QString::fromUtf8("ш")) {
			keyText = "i";
		} else if (keyText.toLower() == QString::fromUtf8("щ")) {
			keyText = "o";
		} else if (keyText.toLower() == QString::fromUtf8("з")) {
			keyText = "p";
		} else if (keyText.toLower() == QString::fromUtf8("х")) {
			keyText = shiftPressed ? "{" : "[";
		} else if (keyText.toLower() == QString::fromUtf8("ъ")) {
			keyText = shiftPressed ? "}" : "]";
		}

		else if (keyText.toLower() == QString::fromUtf8("ф")) {
			keyText = "a";
		} else if (keyText.toLower() == QString::fromUtf8("ы")) {
			keyText = "s";
		} else if (keyText.toLower() == QString::fromUtf8("в")) {
			keyText = "d";
		} else if (keyText.toLower() == QString::fromUtf8("а")) {
			keyText = "f";
		} else if (keyText.toLower() == QString::fromUtf8("п")) {
			keyText = "g";
		} else if (keyText.toLower() == QString::fromUtf8("р")) {
			keyText = "h";
		} else if (keyText.toLower() == QString::fromUtf8("о")) {
			keyText = "j";
		} else if (keyText.toLower() == QString::fromUtf8("л")) {
			keyText = "k";
		} else if (keyText.toLower() == QString::fromUtf8("д")) {
			keyText = "l";
		} else if (keyText.toLower() == QString::fromUtf8("ж")) {
			keyText = shiftPressed ? ":" : ";";
		} else if (keyText.toLower() == QString::fromUtf8("э")) {
			keyText = shiftPressed ? "\"" : "'";
		}

		else if (keyText.toLower() == QString::fromUtf8("я")) {
			keyText = "z";
		} else if (keyText.toLower() == QString::fromUtf8("ч")) {
			keyText = "x";
		} else if (keyText.toLower() == QString::fromUtf8("с")) {
			keyText = "c";
		} else if (keyText.toLower() == QString::fromUtf8("м")) {
			keyText = "v";
		} else if (keyText.toLower() == QString::fromUtf8("и")) {
			keyText = "b";
		} else if (keyText.toLower() == QString::fromUtf8("т")) {
			keyText = "n";
		} else if (keyText.toLower() == QString::fromUtf8("ь")) {
			keyText = "m";
		} else if (keyText.toLower() == QString::fromUtf8("б")) {
			keyText = shiftPressed ? "<" : ",";
		} else if (keyText.toLower() == QString::fromUtf8("ю")) {
			keyText = shiftPressed ? ">" : ".";
		} else if (keyText.toLower() == QString::fromUtf8("ё")) {
			keyText = shiftPressed ? "~" : "`";
		} else if (keyText == "." || keyText == ",") {
			keyText = shiftPressed ? "?" : "/";
		}

		if ((shiftPressed && !isCapsLock()) || (!shiftPressed && isCapsLock())) {
			keyText = keyText.toUpper();
		}
	} else if (altKeyPressed && !isRussianLayout()) {
		if (key == Qt::Key_Q) {
			keyText = QString::fromUtf8("й");
		} else if (key == Qt::Key_W) {
			keyText = QString::fromUtf8("ц");
		} else if (key == Qt::Key_E) {
			keyText = QString::fromUtf8("у");
		} else if (key == Qt::Key_R) {
			keyText = QString::fromUtf8("к");
		} else if (key == Qt::Key_T) {
			keyText = QString::fromUtf8("е");
		} else if (key == Qt::Key_Y) {
			keyText = QString::fromUtf8("н");
		} else if (key == Qt::Key_U) {
			keyText = QString::fromUtf8("г");
		} else if (key == Qt::Key_I) {
			keyText = QString::fromUtf8("ш");
		} else if (key == Qt::Key_O) {
			keyText = QString::fromUtf8("щ");
		} else if (key == Qt::Key_P) {
			keyText = QString::fromUtf8("з");
		} else if (key == Qt::Key_BraceLeft || key == Qt::Key_BracketLeft) {
			keyText = QString::fromUtf8("х");
		} else if (key == Qt::Key_BraceRight || key == Qt::Key_BracketRight) {
			keyText = QString::fromUtf8("ъ");
		} else if (key == Qt::Key_A) {
			keyText = QString::fromUtf8("ф");
		} else if (key == Qt::Key_S) {
			keyText = QString::fromUtf8("ы");
		} else if (key == Qt::Key_D) {
			keyText = QString::fromUtf8("в");
		} else if (key == Qt::Key_F) {
			keyText = QString::fromUtf8("а");
		} else if (key == Qt::Key_G) {
			keyText = QString::fromUtf8("п");
		} else if (key == Qt::Key_H) {
			keyText = QString::fromUtf8("р");
		} else if (key == Qt::Key_J) {
			keyText = QString::fromUtf8("о");
		} else if (key == Qt::Key_K) {
			keyText = QString::fromUtf8("л");
		} else if (key == Qt::Key_L) {
			keyText = QString::fromUtf8("д");
		} else if (key == Qt::Key_Semicolon || key == Qt::Key_Colon) {
			keyText = QString::fromUtf8("ж");
		} else if (key == Qt::Key_QuoteDbl || key == Qt::Key_Apostrophe) {
			keyText = QString::fromUtf8("э");
		} else if (key == Qt::Key_Z) {
			keyText = QString::fromUtf8("я");
		} else if (key == Qt::Key_X) {
			keyText = QString::fromUtf8("ч");
		} else if (key == Qt::Key_C) {
			keyText = QString::fromUtf8("с");
		} else if (key == Qt::Key_V) {
			keyText = QString::fromUtf8("м");
		} else if (key == Qt::Key_B) {
			keyText = QString::fromUtf8("и");
		} else if (key == Qt::Key_N) {
			keyText = QString::fromUtf8("т");
		} else if (key == Qt::Key_M) {
			keyText = QString::fromUtf8("ь");
		} else if (key == Qt::Key_Comma || key == Qt::Key_Less) {
			keyText = QString::fromUtf8("б");
		} else if (key == Qt::Key_Period || key == Qt::Key_Greater) {
			keyText = QString::fromUtf8("ю");
		} else if (key == Qt::Key_QuoteLeft || key == Qt::Key_AsciiTilde) {
			keyText = QString::fromUtf8("ё");
		} else if (key == Qt::Key_Slash || key == Qt::Key_Question) {
			keyText = shiftPressed ? "," : ".";
		}

		if ((shiftPressed && !isCapsLock()) || (!shiftPressed && isCapsLock())) {
			keyText = keyText.toUpper();
		}

	}
	return keyText;
}

QChar Utils::latinKey(const QChar &cyrillicKey)
{
	if (cyrillicKey.toLower() == QString::fromUtf8("й")[0]) {
		return 'q';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ц")[0]) {
		return 'w';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("у")[0]) {
		return 'e';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("к")[0]) {
		return 'r';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("е")[0]) {
		return 't';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("н")[0]) {
		return 'y';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("г")[0]) {
		return 'u';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ш")[0]) {
		return 'i';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("щ")[0]) {
		return 'o';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("з")[0]) {
		return 'p';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("х")[0]) {
		return '[';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ъ")[0]) {
		return ']';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ф")[0]) {
		return 'a';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ы")[0]) {
		return 's';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("в")[0]) {
		return 'd';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("а")[0]) {
		return 'f';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("п")[0]) {
		return 'g';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("р")[0]) {
		return 'h';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("о")[0]) {
		return 'j';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("л")[0]) {
		return 'k';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("д")[0]) {
		return 'l';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ж")[0]) {
		return ';';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("э")[0]) {
		return '\'';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("я")[0]) {
		return 'z';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ч")[0]) {
		return 'x';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("с")[0]) {
		return 'c';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("м")[0]) {
		return 'v';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("и")[0]) {
		return 'b';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("т")[0]) {
		return 'n';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ь")[0]) {
		return 'm';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("б")[0]) {
		return ',';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ю")[0]) {
		return '.';
	} else if (cyrillicKey.toLower() == QString::fromUtf8("ё")[0]) {
		return '`';
	} else {
		return cyrillicKey.toLower();
	}
}

QChar Utils::cyrillicKey(const QChar &latinKey)
{
	if (latinKey.toLower() == QString::fromLatin1("q")[0]) {
		return QString::fromUtf8("й")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("w")[0]) {
		return QString::fromUtf8("ц")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("e")[0]) {
		return QString::fromUtf8("у")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("r")[0]) {
		return QString::fromUtf8("к")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("t")[0]) {
		return QString::fromUtf8("е")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("y")[0]) {
		return QString::fromUtf8("н")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("u")[0]) {
		return QString::fromUtf8("г")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("i")[0]) {
		return QString::fromUtf8("ш")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("o")[0]) {
		return QString::fromUtf8("щ")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("p")[0]) {
		return QString::fromUtf8("з")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("[")[0]) {
		return QString::fromUtf8("х")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("]")[0]) {
		return QString::fromUtf8("ъ")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("a")[0]) {
		return QString::fromUtf8("ф")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("s")[0]) {
		return QString::fromUtf8("ы")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("d")[0]) {
		return QString::fromUtf8("в")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("f")[0]) {
		return QString::fromUtf8("а")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("g")[0]) {
		return QString::fromUtf8("п")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("h")[0]) {
		return QString::fromUtf8("р")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("j")[0]) {
		return QString::fromUtf8("о")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("k")[0]) {
		return QString::fromUtf8("л")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("l")[0]) {
		return QString::fromUtf8("д")[0];
	} else if (latinKey.toLower() == QString::fromLatin1(";")[0]) {
		return QString::fromUtf8("ж")[0];
	} else if (latinKey.toLower() == QString::fromUtf8("э")[0]) {
		return '\'';
	} else if (latinKey.toLower() == QString::fromLatin1("z")[0]) {
		return QString::fromUtf8("я")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("x")[0]) {
		return QString::fromUtf8("ч")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("c")[0]) {
		return QString::fromUtf8("с")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("v")[0]) {
		return QString::fromUtf8("м")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("b")[0]) {
		return QString::fromUtf8("и")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("n")[0]) {
		return QString::fromUtf8("т")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("m")[0]) {
		return QString::fromUtf8("ь")[0];
	} else if (latinKey.toLower() == QString::fromLatin1(",")[0]) {
		return QString::fromUtf8("б")[0];
	} else if (latinKey.toLower() == QString::fromLatin1(".")[0]) {
		return QString::fromUtf8("ю")[0];
	} else if (latinKey.toLower() == QString::fromLatin1("`")[0]) {
		return QString::fromUtf8("ё")[0];
	} else {
		return latinKey.toLower();
	}
}

} // namespace Editor
