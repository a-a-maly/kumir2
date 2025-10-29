#ifndef NEWKURSDIALOG_H
#define NEWKURSDIALOG_H
#include <QDialog>

namespace Ui
{
	class newKursDialog;
}

class newKursDialog : public QDialog
{
	Q_OBJECT

public:
	explicit newKursDialog(QWidget *parent = 0);
	~newKursDialog();

	QString name() const;


public slots:
	void showFileSelect();

private:
	Ui::newKursDialog *ui;

};

#endif // NEWKURSDIALOG_H
