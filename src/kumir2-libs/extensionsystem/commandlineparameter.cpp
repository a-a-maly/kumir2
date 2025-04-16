#include "commandlineparameter.h"

#include <QStringList>

namespace ExtensionSystem
{


CommandLineParameter::CommandLineParameter(
	bool allowInGui,
	QChar shortName,
	const QString &longName,
	const QString &description,
	QVariant::Type acceptType,
	bool required
) :
	allowInGui_(allowInGui),
	shortName_(shortName),
	longName_(longName),
	value_(QVariant::Invalid),
	description_(description),
	acceptValue_(true),
	valueRequired_(required),
	acceptType_(acceptType)
{
}

CommandLineParameter::CommandLineParameter(
	bool allowInGui,
	QChar shortName,
	const QString &longName,
	const QString &description
) :
	allowInGui_(allowInGui),
	shortName_(shortName),
	longName_(longName),
	value_(false),
	description_(description),
	acceptValue_(false),
	valueRequired_(false),
	acceptType_(QVariant::Bool)
{
}

CommandLineParameter::CommandLineParameter(
	const CommandLineParameter &other
) : QObject(),
	allowInGui_(other.allowInGui_),
	shortName_(other.shortName_),
	longName_(other.longName_),
	value_(other.value_),
	shortDescription_(other.shortDescription_),
	description_(other.description_),
	acceptValue_(other.acceptValue_),
	valueRequired_(other.valueRequired_),
	acceptType_(other.acceptType_)
{
}

CommandLineParameter::CommandLineParameter(
	bool allowInGui,
	const QString &shortDescription,
	const QString &description,
	QVariant::Type acceptType,
	bool required
) :
	allowInGui_(allowInGui),
	shortName_(QChar()),
	longName_(QString()),
	value_(QVariant::Invalid),
	shortDescription_(shortDescription),
	description_(description),
	acceptValue_(true),
	valueRequired_(required),
	acceptType_(acceptType)
{
}

CommandLineParameter &CommandLineParameter::operator =(
	const CommandLineParameter &other
) {
	allowInGui_ = other.allowInGui_;
	shortName_ = other.shortName_;
	longName_ = other.longName_;
	value_ = other.value_;
	shortDescription_ = other.shortDescription_;
	description_ = other.description_;
	acceptValue_ = other.acceptValue_;
	valueRequired_ = other.valueRequired_;
	acceptType_ = other.acceptType_;
	return *this;
}

QString CommandLineParameter::toHelpLine() const
{
	QString firstColumn, secondColumn;
	if (acceptValue_) {
		QString type;
		if (acceptType_ == QVariant::Int) {
			type = tr("int");
		} else if (acceptType_ == QVariant::Double) {
			type = tr("real");
		} else if (acceptType_ == QVariant::Bool) {
			type = tr("bool");
		} else if (acceptType_ == QVariant::Char) {
			type = tr("char");
		} else if (acceptType_ == QVariant::String) {
			type = tr("string");
		}
		if (shortDescription_.length() > 0) {
			if (valueRequired_) {
				firstColumn = shortDescription_.leftJustified(30);
			} else {
				firstColumn = QString::fromLatin1("%1...%2")
					.arg(shortDescription_.arg(1)).arg(shortDescription_.arg("n"))
					.leftJustified(30);
			}
		} else {
			firstColumn = QString::fromLatin1("-%1=<%2>")
				.arg(shortName_).arg(type).leftJustified(10);
			secondColumn = QString::fromLatin1("--%1=<%2>")
				.arg(longName_).arg(type).leftJustified(20);
		}
	} else {
		firstColumn = QString::fromLatin1("-%1")
			.arg(shortName_).leftJustified(10);
		secondColumn = QString::fromLatin1("--%1")
			.arg(longName_).leftJustified(20);
	}
	QStringList lines;
	lines.push_back("  " + firstColumn + " " + secondColumn);
	QString Spacer;
	Spacer.fill(' ', 4 + firstColumn.length() + secondColumn.length());
	QStringList words = description_.split(' ');
	while (!words.isEmpty()) {
		const QString word = words.first();
		words.pop_front();
		if (lines.last().length() + word.length() + 1 <= 80) {
			lines.last() += " " + word;
		} else {
			lines.push_back(Spacer + word);
		}
	}
	return lines.join("\n");
}

bool CommandLineParameter::isValid() const
{
	return !valueRequired_ || value_.isValid();
}

void CommandLineParameter::fillValue(const QString &argument)
{
	if (acceptValue_ && shortDescription_.length() == 0) {
		QString svalue;
		if (argument.startsWith("-" + QString(shortName_))) {
			svalue = argument.mid(2);
		} else if (argument.startsWith("--" + longName_)) {
			svalue = argument.mid(2 + longName_.length());
		}
		if (svalue.startsWith("=")) {
			svalue = svalue.mid(1);
			if (acceptType_ == QVariant::String || acceptType_ == QVariant::Char) {
				if (svalue.length() >= 2) {
					if (svalue.startsWith('"') && svalue.endsWith('"')) {
						svalue = svalue.mid(1, svalue.length() - 2);
					} else if (svalue.startsWith('\'') && svalue.endsWith('\'')) {
						svalue = svalue.mid(1, svalue.length() - 2);
					}
				}
			}
			if (acceptType_ == QVariant::String) {
				value_ = svalue;
			} else if (acceptType_ == QVariant::Char && svalue.length() >= 1) {
				value_ = svalue.at(0);
			} else if (acceptType_ == QVariant::Int) {
				bool ok;
				int val = svalue.toInt(&ok);
				if (ok) {
					value_ = val;
				}
			} else if (acceptType_ == QVariant::Double) {
				bool ok;
				double val = svalue.toDouble(&ok);
				if (ok) {
					value_ = val;
				}
			} else if (acceptType_ == QVariant::Bool) {
				svalue = svalue.toLower();
				static const QStringList YES = QStringList()
					<< QString::fromLatin1("1")
					<< QString::fromLatin1("true")
					<< QString::fromLatin1("yes")
					<< QString::fromUtf8("да");
				static const QStringList NO = QStringList()
					<< QString::fromLatin1("0")
					<< QString::fromLatin1("false")
					<< QString::fromLatin1("no")
					<< QString::fromUtf8("нет");
				if (YES.contains(svalue) || NO.contains(svalue)) {
					bool val = YES.contains(svalue);
					value_ = val;
				}
			}
		}
	} else if (shortDescription_.length() > 0) {
		value_ = argument;  // unnamed parameter
	} else {
		value_ = true;
	}
}

CommandLine::CommandLine()
{
}

CommandLine::CommandLine(
	const QList<CommandLineParameter> &other
) : data_(other)
{
}

bool CommandLine::hasFlag(QChar shortName) const
{
	for (int i = 0; i < data_.size(); i++) {
		const CommandLineParameter &param = data_.at(i);
		if (!param.acceptValue_ && param.shortName_ == shortName) {
			return param.value_.toBool();
		}
	}
	return false;
}

bool CommandLine::hasFlag(const QString &longName) const
{
	for (int i = 0; i < data_.size(); i++) {
		const CommandLineParameter &param = data_.at(i);
		if (!param.acceptValue_ && param.longName_ == longName) {
			return param.value_.toBool();
		}
	}
	return false;
}

QVariant CommandLine::value(QChar shortName) const
{
	for (int i = 0; i < data_.size(); i++) {
		const CommandLineParameter &param = data_.at(i);
		if (param.shortName_ == shortName) {
			return param.value_;
		}
	}
	return QVariant::Invalid;
}

QVariant CommandLine::value(const QString &longName) const
{
	for (int i = 0; i < data_.size(); i++) {
		const CommandLineParameter &param = data_.at(i);
		if (param.longName_ == longName) {
			return param.value_;
		}
	}
	return QVariant::Invalid;
}

QVariant CommandLine::value(size_t index) const
{
	if (index < size()) {
		QList<const CommandLineParameter *> params = unnamedParameters();
		return params[index]->value_;
	}
	return QVariant::Invalid;
}

size_t CommandLine::size() const
{
	return unnamedParameters().size();
}

QList<const CommandLineParameter *> CommandLine::unnamedParameters() const
{
	QList<const CommandLineParameter *> result;
	for (int i = 0; i < data_.size(); i++) {
		if (data_[i].shortDescription_.length() > 0) {
			result.push_back(&(data_.at(i)));
		}
	}
	return result;
}

} // namespace ExtensionSystem
