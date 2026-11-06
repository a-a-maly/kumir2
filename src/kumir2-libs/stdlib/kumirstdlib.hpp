#ifndef KUMIRSTDLIB_H
#define KUMIRSTDLIB_H

#include "kstdlib_dll.h"
#include "encodings.hpp"

#include <stdio.h>
#include <deque>


namespace Kumir
{

typedef wchar_t Char;
typedef std::wstring String;
typedef double real;

inline String &operator+(String &s1, const /*utf8*/ char *s2)
{
	EncodingError encodingError;
	String us2 = Coder::decode(UTF8, std::string(s2), encodingError);
	s1.append(us2);
	return s1;
}

struct FileType {
	enum OpenMode { NotOpen, Read, Write, Append };
	enum SpecialType { RegularFile, Console };

	FileType()
	{
		valid = false;
		mode = NotOpen;
		type = RegularFile;
		autoClose = false;
		handle = 0;
	}

	void setName(const String &name)
	{
		fullPath = name;
	}

	String getName() const
	{
		return fullPath;
	}

	void setMode(OpenMode m)
	{
		mode = int(m);
	}

	OpenMode getMode() const
	{
		return OpenMode(mode);
	}

	SpecialType getType() const
	{
		return SpecialType(type);
	}

	void setType(SpecialType t)
	{
		type = int(t);
	}

	bool isValid() const
	{
		return valid;
	}

	void invalidate()
	{
		valid = false;
		handle = 0;
	}

	bool operator==(const FileType &other) const
	{
		return other.fullPath == fullPath;
	}

	bool operator!=(const FileType &other) const
	{
		return other.fullPath != fullPath;
	}

	String fullPath;
	int mode;
	int type;
	bool valid;
	bool autoClose;
	FILE *handle;
};

class KS_DLL Core
{
	static String &takeError();

public:
	static const String &getError();
	static void unsetError() { takeError().clear(); }

	static void init()
	{
		unsetError();
	}

	static void finalize() {}

	static String fromEncoding(Encoding enc, const std::string &s)
	{
		EncodingError encodingError;
		String result = Coder::decode(enc, s, encodingError);
		return result;
	}

	static std::string toEncoding(Encoding enc, const String &s)
	{
		EncodingError encodingError;
		std::string result = Coder::encode(enc, s, encodingError);
		return result;
	}

	static String fromAscii(const std::string &s)
	{
		return fromEncoding(ASCII, s);
	}

	static std::string toAscii(const String &s)
	{
		return toEncoding(ASCII, s);
	}

	static String fromUtf8(const std::string &s)
	{
		return fromEncoding(UTF8, s);
	}

	static std::string toUtf8(const String &s)
	{
		return toEncoding(UTF8, s);
	}

	static std::string toLowerCase(const std::string &s);
	static std::string toUpperCase(const std::string &s);

	static std::wstring toLowerCaseW(const std::wstring &s);
	static std::wstring toUpperCaseW(const std::wstring &s);

	static String joinStrings(const std::deque<String> &sl, Char sep);
	static std::deque<String> splitString(
		const String &s,
		const Char sep,
		bool skipEmptyParts
	);

	static Encoding getSystemEncoding();

	typedef void (*AbortHandlerType)();

	static AbortHandlerType getAbortHandler();
	static void setAbortHandler(AbortHandlerType ah);
	static void abort(const String &e);
};


class StringList: public std::deque<String>
{
public:
	StringList() {}
	StringList(const std::deque<String> &d) : std::deque<String> (d) {}
	String join(Char sep) const
	{
		return Core::joinStrings(*this, sep);
	}
};


class KS_DLL Math
{
private:
	static bool isCorrectDouble(double x);

public:
	static void init() {}
	static void finalize() {}

	static bool checkSumm(int32_t l, int32_t r);
	static bool checkDiff(int32_t l, int32_t r);
	static bool checkProd(int32_t l, int32_t r);

	static bool isCorrectReal(real val) { return isCorrectDouble(val); }
	static real maxreal();

	static int imax(int x, int y) { return x > y ? x : y; }
	static int imin(int x, int y) { return x < y ? x : y; }

	static real rmax(real x, real y) { return x > y ? x : y; }
	static real rmin(real x, real y) { return x < y ? x : y; }

	static real abs(real x);
	static int iabs(int x);
	static int intt(real x);

	static real arcsin(real x);
	static real arccos(real x);
	static real arctg(real x);
	static real arcctg(real x);

	static real sin(real x);
	static real cos(real x);
	static real tg(real x);
	static real ctg(real x);

	static real exp(real x);
	static real ln(real x);
	static real lg(real x);

	static real sqrt(real x);
	static real pow(real a, real b);
	static int ipow(int a, int b);

	static int div(int a, int b);
	static int mod(int a, int b);

	template <typename T> static int sign(T x)
	{
		if (x > 0) {
			return 1;
		} else if (x < 0) {
			return -1;
		} else {
			return 0;
		}
	}
};

class KS_DLL Random
{
private:
	static unsigned int get_max();
	static unsigned int get_sample();

public:
	static void init();
	static void finalize() {}

	static int irand(int a, int b);
	static int irnd(int x);
	static real rrand(real a, real b);
	static real rrnd(real x);
};


class KS_DLL Converter
{
private:
	static bool validDecimal(const String &astring);
	static real fromDecimal(const String &astring);

public:
	enum ParseError {
		NoError,
		EmptyWord,
		WrongHex,
		WrongReal,
		WrongExpForm,
		BadSymbol,
		Overflow
	};

	static void init() {}
	static void finalize() {}

	static int parseInt(String word, unsigned int base, ParseError &error);
	static real parseReal(String word, Char dot, ParseError &error);

	static bool isCorrectIntegerConstant(const String &v);

	static String sprintfInt(int value, unsigned int base, unsigned int width, char al);

	static String sprintfReal(
		real value, Char dot, bool expform,
		unsigned int width, int decimals, char al
	);

	static String sprintfReal(real value, unsigned int precision = 16);

	static int stringToInt(const String &str, bool &ok)
	{
		ParseError error = NoError;
		int result = parseInt(str, 0, error);
		ok = (error == NoError);
		return result;
	}

	static real stringToReal(const String &str, bool &ok)
	{
		ParseError error = NoError;
		real result = parseReal(str, '.', error);
		ok = (error == NoError);
		return result;
	}

	static int stringToIntDef(const String &str, int def)
	{
		bool ok = false;
		int val = stringToInt(str, ok);
		return ok ? val : def;
	}

	static real stringToRealDef(const String &str, real def)
	{
		bool ok = false;
		real val = stringToReal(str, ok);
		return ok ? val : def;
	}

	static bool stringToBoolDef(const String &str, bool def);

	static String realToString(real value)
	{
		return sprintfReal(value, '.', false, 0, -1, 'l');
	}

	static String intToString(int value)
	{
		return sprintfInt(value, 10, 0, 'l');
	}

};

class KS_DLL StringUtils
{
public:
	static void init() {}
	static void finalize() {}

	static int code(Char ch);
	static Char symbol(int code);

	static int unicode(Char ch)
	{
		return static_cast<int>(ch);
	}

	static Char unisymbol(int code);

	template <class S, typename C>
	inline static void trim(S &s)
	{
		size_t a = 0, l = s.length(), b = l;
		for (; a < b; a++) {
			if (s[a] != C(' ') && s[a] != C('\t') && s[a] != C('\n'))
				break;
		}

		for (; b > a; b--) {
			if (s[b - 1] != C(' ') && s[b - 1] != C('\t') && s[b - 1] != C('\n'))
				break;
		}

		if (a) {
			for (size_t i = a; i < b; i++)
				s[i - a] = s[i];
		}

		s.erase(b - a, l - (b - a));
	}

	static String toLowerCase(const String &s)
	{
		return Core::toLowerCaseW(s);
	}

	static String toUpperCase(const String &s)
	{
		return Core::toUpperCaseW(s);
	}

	static int find(int from, const String &substr, const String &s);
	static void insert(const String &substr, String &s, int pos);
	static void remove(String &s, int pos, int count);
	static void replace(
		String &s, const String &oldSubstr,
		const String &newSubstr,
		bool all
	);

};

class KS_DLL AbstractInputBuffer
{
protected:
	~AbstractInputBuffer();

public:
	virtual bool readRawChar(Char &ch) = 0;
	virtual void pushLastCharBack() = 0;
	virtual void clear() = 0;
};

class KS_DLL AbstractOutputBuffer
{
protected:
	~AbstractOutputBuffer();

public:
	virtual void writeRawString(const String &) = 0;
};

class KS_DLL Files
{
	static bool isOpenedFiles();

public:
	AbstractInputBuffer *getConsoleInputBuffer();
	static void setConsoleInputBuffer(AbstractInputBuffer *b);

	AbstractOutputBuffer *getConsoleOutputBuffer();
	static void setConsoleOutputBuffer(AbstractOutputBuffer *b);

	static void init();
	static void finalize();

	static void setFileEncoding(const String &enc);

	static String getAbsolutePath(const String &fileName);
	static String CurrentWorkingDirectory();


	static String getNormalizedPath(const String &path, Char sep);
	static bool canOpenForRead(const String &path);
	static bool canOpenForWrite(const String &path);

	static bool exist(const String &fileName);
	static bool isDirectory(const String &fileName);
	static bool unlink(const String &fileName);
	static bool rmdir(const String &fileName);
	static bool mkdir(const String &fileName);

	static FileType getConsoleBuffer();

	static FileType open(
		const String &shortName,
		FileType::OpenMode mode,
		bool remember,
		FILE **fh
	);

	static void close(const FileType &key);

	static void reset(FileType &key);
	static bool eof(const FileType &key);
	static bool hasData(const FileType &key);

	static bool overloadedStdIn();
	static bool overloadedStdOut();

	static FILE *getAssignedIn();
	static FILE *getAssignedOut();

	static void assignInStream (String fileName);
	static void assignOutStream(String fileName);
};


class KS_DLL IO
{
public:
	enum StreamType { File, InternalBuffer, ExternalBuffer };

	class KS_DLL OutputStream
	{
	public:
		OutputStream()
		{
			streamType_ = InternalBuffer;
			file = 0;
			encoding = UTF8;
			buffer.reserve(10);
			externalBuffer_ = 0;
		}

		OutputStream(FILE *f, Encoding enc);

		OutputStream(AbstractOutputBuffer *buffer)
		{
			streamType_ = ExternalBuffer;
			file = 0;
			encoding = UTF8;
			externalBuffer_ = buffer;
		}

		StreamType type() const { return streamType_; }
		const String &getBuffer() const { return buffer; }

		void writeRawString(const String &s);

	private:
		StreamType streamType_;
		FILE *file;
		Encoding encoding;
		String buffer;
		AbstractOutputBuffer *externalBuffer_;
	};


	class KS_DLL InputStream
	{
	public:
		InputStream()
		{
			streamType_ = InternalBuffer;
			file_ = 0;
			encoding_ = UTF8;
			errStart_ = 0;
			errLength_ = 0;
			currentPosition_ = 0;
			externalBuffer_ = 0;
		}

		InputStream(const String &b)
		{
			streamType_ = InternalBuffer;
			file_ = 0;
			encoding_ = UTF8;
			errStart_ = 0;
			errLength_ = 0;
			buffer_ = b;
			currentPosition_ = 0;
			externalBuffer_ = 0;
		}

		InputStream(AbstractInputBuffer *buffer)
		{
			streamType_ = ExternalBuffer;
			file_ = 0;
			encoding_ = UTF8;
			errStart_ = 0;
			errLength_ = 0;
			currentPosition_ = 0;
			externalBuffer_ = buffer;
		}

		InputStream(FILE *f, Encoding enc);

		StreamType type() const { return streamType_; }
		long currentPosition() const { return currentPosition_; }

		void getError(String &text, int &start, int &len) const
		{
			text = error_;
			start = errStart_;
			len = errLength_;
		}

		void setError(const String &err)
		{
			if (type() != InternalBuffer) {
				Core::abort(err);
			} else {
				error_ = err;
			}
		}

		bool hasError() const
		{
			if (type() != InternalBuffer) {
				return Core::getError().length() > 0;
			} else {
				return error_.length() > 0;
			}
		}

		bool readRawChar(Char &x);
		void pushLastCharBack();
		String readUntil(const String &delimeters);
		void skipDelimiters(const String &delim = String());

	private:
		void markPossibleErrorStart()
		{
			errStart_ = currentPosition_;
			errLength_ = 0;
			error_.clear();
		}

		StreamType streamType_;
		FILE *file_;
		long currentPosition_;
		Encoding encoding_;
		String buffer_;
		String error_;
		int errStart_;
		int errLength_;
		//char lastCharBuffer_[3];
		Char lastChar_;
		int8_t lastCharLength_;
		bool lastCharHere_;
		AbstractInputBuffer *externalBuffer_;
	}; // end inner class InputStream


private:
	// Format parsing function
	static StringList splitIntoLexemsByDelimeter(
		const String &s,
		Char delim = Char(':')
	);

	static String readWord(InputStream &is);

public:

	// Generic functions to be in use while input from GUI
	static String readString(InputStream &is);
	static String readLine(InputStream &is);
	static Char readChar(InputStream &is);
	static int readInteger(InputStream &is);
	static real readReal(InputStream &is);
	static bool readBool(InputStream &is);

	static void writeString(OutputStream &os, const String &str, int width);
	static void writeChar(OutputStream &os, const Char &chr, int width);
	static void writeInteger(OutputStream &os, int value, int width);
	static void writeReal(OutputStream &os, real value, int width, int decimals);
	static void writeBool(OutputStream &os, bool value, int width);

	// Actual functions to be in use while input from stream

	static void setLocaleEncoding(Encoding enc);
	static InputStream makeInputStream(FileType fileNo, bool fromStdIn);
	static OutputStream makeOutputStream(FileType fileNo, bool toStdOut);

	static String readString(FileType fileNo = FileType(), bool fromStdIn = true);
	static String readLine(FileType fileNo = FileType(), bool fromStdIn = true);
	static Char readChar(FileType fileNo = FileType(), bool fromStdIn = true);
	static int readInteger(FileType fileNo = FileType(), bool fromStdIn = true);
	static real readReal(FileType fileNo = FileType(), bool fromStdIn = true);
	static bool readBool(FileType fileNo = FileType(), bool fromStdIn = true);

	static void writeString(int width, const String &value, FileType fileNo = FileType(), bool toStdOut = true);
	static void writeChar(int width, Char value, FileType fileNo = FileType(), bool toStdOut = true);
	static void writeInteger(int width, int value, FileType fileNo = FileType(), bool toStdOut = true);
	static void writeReal(int width, int decimals, real value, FileType fileNo = FileType(), bool toStdOut = true);
	static void writeBool(int width, bool value, FileType fileNo = FileType(), bool toStdOut = true);

}; // end class IO


class KS_DLL System
{
public:
	static void init() {}
	static void finalize() {}

	static int time();
};

inline void initStandardLibrary()
{
	Core::init();
	Math::init();
	Random::init();
	Converter::init();
	StringUtils::init();
	Files::init();
	System::init();
}

inline void finalizeStandardLibrary()
{
	Core::finalize();
	Math::finalize();
	Random::finalize();
	Converter::finalize();
	StringUtils::finalize();
	Files::finalize();
	System::finalize();
}


} // namespace Kumir

#endif // KUMIRSTDLIB_H
