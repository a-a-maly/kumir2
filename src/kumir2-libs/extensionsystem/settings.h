#ifndef EXTENSIONSYSTEM_SETTINGS_H
#define EXTENSIONSYSTEM_SETTINGS_H

#include <QtGlobal>
#include <QVariant>
#include <QString>
#include <QMutex>
#include <QSettings>
#include <QScopedPointer>
#include <QSharedPointer>

#ifdef EXTENSIONSYSTEM_LIBRARY
#define EXTENSIONSYSTEM_EXPORT Q_DECL_EXPORT
#else
#define EXTENSIONSYSTEM_EXPORT Q_DECL_IMPORT
#endif

namespace ExtensionSystem {

class EXTENSIONSYSTEM_EXPORT Settings
{
	friend class PluginManager;
	friend struct PluginManagerImpl;
	friend class KPlugin;
public:

	QVariant value(const QString & key, const QVariant & default_ = QVariant()) const;
	void setValue(const QString & key, const QVariant & value_);
	QString locationDirectory() const;
	QString settingsFilePath() const;
	void flush();

	~Settings();

protected:
	explicit Settings(const QString & pluginName_);
	void changeWorkingDirectory(const QString & workDirPath);

private:
	QString pluginName_;
	QString workDirPath_;
	QScopedPointer<QMutex> mutex_;
	QScopedPointer<QSettings> qsettings_;
	QString settingsFile_;
};

typedef QSharedPointer<Settings> SettingsPtr;
EXTENSIONSYSTEM_EXPORT QString defaultSettingsScope();

} // namespace ExtensionSystem

#endif // EXTENSIONSYSTEM_SETTINGS_H
