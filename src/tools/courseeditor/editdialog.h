#ifndef EDITDIALOG_H
#define EDITDIALOG_H

#include <QDialog>
class QListWidgetItem;

namespace Ui
{
class EditDialog;
}

class EditDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EditDialog(QWidget *parent = 0);
	~EditDialog();

	void setTitle(QString title);
	void setDesc(QString dec);
	void setProgram(QString prg);
	void setUseIsps(QString isp);
	void setEnvs(QStringList);


	QString getTitle() const;
	QString getDesc() const;
	QString getProgram() const;
	QStringList getUseIsps() const;
	QStringList getEnvs() const;

	void setCurDir(QString dir)
	{
		curDir = dir;
	}

public slots:
	void setPrg();
	void removeEnv();
	void addEnv();

private:
	Ui::EditDialog *ui;
	QString curDir;
	static QStringList items2StringList(QList<QListWidgetItem *> inp);
};

#endif // EDITDIALOG_H
