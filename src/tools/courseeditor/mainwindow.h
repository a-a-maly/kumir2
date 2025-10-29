/****************************************************************************
**
** Copyright (C) 2004-2010 NIISI RAS. All rights reserved.
**
** This file is part of the KuMir.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QFile>
#include <QFileInfo>
#include <QModelIndex>
class QSettings;
class QLineEdit;

#include "course_model.h"
#include "taskControlInterface.h"
class EditDialog;
class newKursDialog;

class CSInterface;
namespace Ui
{
class MainWindowTask;
}

class MainWindowTask : public QMainWindow
{
	Q_OBJECT
public:
	MainWindowTask(QWidget *parent = 0);
	~MainWindowTask();

	void setInterface(CSInterface *csInterface)
	{
		_interface = csInterface;
	}

	void setCS(QString cs)
	{
		CS = cs;
	}

	void setup();
	QString getFileName(QString fileName);

public slots:
	void aboutToQuit();
	void loadCourse();
	void returnTested();
	void saveCourse();
	void saveCourseFile();
	void saveBaseKurs();
	void showText(const QModelIndex &index);
	void startEdit(const QModelIndex &index);
	void loadHtml(QString fileName);
	void startTask();
	void checkTask();
	void Close();
	void customContextMenuRequested(QPoint  pos);
	void addTask();
	void addDeepTask();
	void deleteTask();

	void saveKursAs();//TEACHER
	void editTask();//Teacher
	void setEditTaskEnabled(bool flag);
	void moveUp();
	void moveDown();
	void unLockEditFields();
	void lockEditFields();
	void remSelIsp();
	void newKurs();
	void endRootEdit();
	void cancelRootEdit();
	void addIsp();
	void showFields();
	void fieldClick();
	void remField();
	void addField();
	void setPrg();
	void setType();
	void prgLineChange();
	void editFile();
	void makeSection();
	void openRecent();
	void createMoveMenu();
	void move();

protected:
	void changeEvent(QEvent *e);
	void closeEvent(QCloseEvent *event);

private:
	//Поиск id среди списка индексов
	bool checkInList(int id, QModelIndexList list) const;

	void enableMkSect(bool flag);
	void markProgChange();
	void lockKursFile(const QString fileName);
	QString getFileTypes();
	void createRescentMenu();
	void save2Tree(); //Save to XML tree, but not to file
	void setUpDown(QModelIndex index);
	QString loadScript(QString file_name);
	QString loadTestAlg(QString file_name);
	void loadCourseData(const QString filename);
	void loadMarks(const QString fileName);
	void refreshIspsNEnv();

	Ui::MainWindowTask *ui;
	QString curDir;
	courseModel *course;
	QModelIndex curTaskIdx;
	CSInterface *_interface;
	QString CS;
	bool onTask;
	courseChanges changes;
	QString cursFile;
	QList<int> progChange;
	QFile cursWorkFile; //.work.xml
	QMenu customMenu;
	bool isTeacher;
	EditDialog *editDialog;
	newKursDialog *newDialog;
	QSettings *settings;
	QLineEdit *editRoot;
	QFileInfo baseKursFile; //4 mode
	bool changed;
	QStringList lastFiles;
	QFile lockFile; //Lock main xml file

 public: // ???
	KumZadanie task;
};

#endif // MAINWINDOW_H
