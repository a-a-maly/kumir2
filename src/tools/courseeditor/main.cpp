#include <QApplication>
#include <QDebug>
#include <QDirIterator>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindowTask w;
#if 1
	QDirIterator it(":/", QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
	while (it.hasNext()) {
		qDebug() << it.next();
	}
#endif

	a.setWindowIcon(QIcon(":/taskEdit.ico"));
	w.setWindowIcon(QIcon(":/taskEdit.ico"));
	w.setup();
	w.show();
	return a.exec();
}
