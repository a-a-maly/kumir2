#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "interface.h"
#include "csInterface.h"
#include "editdialog.h"
#include "newkursdialog.h"
#include "kumzadanie.h"
#include "course_model.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QProcess>

#include <QDebug>
#include <QFileDialog>

MainWindowTask::MainWindowTask(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindowTask),
	task(new KumZadanie()),
	changes(new courseChanges)
{
	cursFile = "";
	course = NULL;
	curDir = "";
	progChange.clear();
	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));
	
}

MainWindowTask::~MainWindowTask()
{
	delete changes;
	changes = 0;
	delete task;
	task = 0;
	delete ui;
	ui = 0;
}

void MainWindowTask::setup()
{
	ui->setupUi(this);
	ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
	settings = new QSettings("NIISI RAS", "Kumir 1.9");
	setWindowIcon(QIcon(":/taskEdit.ico"));
	settings->setIniCodec("UTF-8");
	lastFiles = settings->value("RescentFiles").toStringList();
	customMenu.hide();
	connect(ui->loadCurs, SIGNAL(triggered()), this, SLOT(loadCourse()));
	connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveCourse()));
	connect(ui->treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(showText(QModelIndex)));
	connect(ui->treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(startEdit(QModelIndex)));
	connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(Close()));
	connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));

	customMenu.addAction(ui->actionAdd);
	customMenu.addAction(ui->actionRemove);
	customMenu.addAction(ui->actionEdit);
	customMenu.addAction(ui->addDeep);
	customMenu.addSeparator();
	customMenu.addAction(ui->actionup);
	customMenu.addAction(ui->actionDown);
	customMenu.addAction(ui->actionMakeSection);
	customMenu.addMenu(ui->menuMove);
	connect(ui->zadRb, SIGNAL(clicked(bool)), this, SLOT(setType()));
	connect(ui->nodeRb, SIGNAL(clicked(bool)), this, SLOT(setType()));

	connect(ui->actionup, SIGNAL(triggered()), this, SLOT(moveUp()));
	connect(ui->actionDown, SIGNAL(triggered()), this, SLOT(moveDown()));
	connect(ui->actionAdd, SIGNAL(triggered()), this, SLOT(addTask()));
	connect(ui->addDeep, SIGNAL(triggered()), this, SLOT(addDeepTask()));
	connect(ui->actionSaveK, SIGNAL(triggered()), this, SLOT(saveCourse()));
	connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveKursAs()));
	connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(deleteTask()));
	connect(ui->actionMakeSection, SIGNAL(triggered()), this, SLOT(makeSection()));
	newDialog = new newKursDialog();
	connect(ui->actionNewK, SIGNAL(triggered()), this, SLOT(newKurs()));
	editDialog = new EditDialog(this);
	connect(ui->actionEdit, SIGNAL(triggered()), this, SLOT(editTask()));
	setEditTaskEnabled(false);
	ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	editRoot = new QLineEdit(ui->treeView);
	editRoot->hide();
	connect(editRoot, SIGNAL(editingFinished()), this, SLOT(endRootEdit()));
	connect(ui->remIspButt, SIGNAL(pressed()), this, SLOT(remSelIsp()));
	connect(ui->addIspButt, SIGNAL(pressed()), this, SLOT(addIsp()));
	connect(ui->ispList, SIGNAL(clicked(QModelIndex)), this, SLOT(showFields()));
	connect(ui->fieldsList, SIGNAL(clicked(QModelIndex)), this, SLOT(fieldClick()));
	connect(ui->remFieldButt, SIGNAL(pressed()), this, SLOT(remField()));
	connect(ui->addFieldButt, SIGNAL(pressed()), this, SLOT(addField()));
	connect(ui->actionS, SIGNAL(triggered()), this, SLOT(saveBaseKurs()));
	connect(ui->selFileButt, SIGNAL(pressed()), this, SLOT(setPrg()));
	connect(ui->prgEdit, SIGNAL(textChanged(QString)), this, SLOT(prgLineChange()));
	connect(ui->editFButt, SIGNAL(pressed()), this, SLOT(editFile()));
	connect(ui->menuMove, SIGNAL(aboutToShow()), this, SLOT(createMoveMenu()));

	ui->ispSel->addItem("Robot");
	ui->ispSel->addItem(trUtf8("Водолей"));
	ui->actionup->setIcon(QIcon(":/arrow_up.png"));
	ui->actionDown->setIcon(QIcon(":/arrow_down.png"));
	changed = false;
	isTeacher = true;
	onTask = false;
	cursFile = "";
	createRescentMenu();
}

void MainWindowTask::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindowTask::lockKursFile(const QString fileName)
{
	if (lockFile.isOpen()) {
		lockFile.close();
	}
	lockFile.setFileName(fileName);
	if (!lockFile.open(QIODevice::ReadWrite)) {
		qDebug() << "Cant lock file!";
	}
}

void MainWindowTask::loadCourseData(const QString fileName)
{
	course = new courseModel();
	connect(course, SIGNAL(dataChanged(QModelIndex, QModelIndex)), ui->treeView, SLOT(dataChanged(QModelIndex, QModelIndex)));
	int tasks = course->loadCourse(fileName);

	lockKursFile(fileName);

	course->setTeacher(isTeacher);
	if (tasks == -1) {
		QMessageBox::information(0, "", trUtf8("Ошибка открытия файла: ") + fileName, 0, 0, 0);
		return;
	};
	ui->treeView->setModel(course);
	ui->treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	ui->treeView->setSelectionBehavior(QAbstractItemView::SelectItems);

	curTaskIdx = QModelIndex();
	onTask = false;
	ui->actionSave->setEnabled(true);

	changes->cleanChanges();
	cursFile = fileName;
	setEditTaskEnabled(true);
	ui->menuMove->setEnabled(true);
	if (lastFiles.indexOf(fileName) == -1) {
		lastFiles.prepend(fileName);
	}

	if (lastFiles.count() > 10) {
		lastFiles.takeLast();
	}
	createRescentMenu();
}

void MainWindowTask::loadMarks(const QString fileName)
{
	QDomDocument workXml;
	QFile f(fileName);
	if (!f.open(QIODevice::ReadOnly)) {
		QMessageBox::information(0, "", trUtf8("Ошибка открытия файла: ") + fileName, 0, 0, 0);
		return;

	}

	if (f.atEnd()) {
		QMessageBox::information(0, "", trUtf8("Ошибка открытия файла ,файл пуст: ") + fileName, 0, 0, 0);
		return;
	}

	cursWorkFile.setFileName(f.fileName());
	QString error;
	int str, pos;
	workXml.setContent(f.readAll(), true, &error, &str, &pos);
	qDebug() << "File parce:" << error << "str" << str << " pos" << pos;

	QDomElement root = workXml.documentElement();
	if (root.tagName() != "COURSE") {
		QMessageBox::information(0, "", trUtf8("Ошибка загрузки файла: ") + fileName, 0, 0, 0);
		return;
	}
	QDomElement fileEl = root.firstChildElement("FILE");
	loadCourseData(fileEl.attribute("fileName"));//Gruzim kurs

	QString fileN = fileEl.attribute("fileName");
	qDebug() << "KURS ZAGRUZILI";
	if (cursFile != fileEl.attribute("fileName")) {
		QMessageBox::information(0, "", trUtf8("Не наеден файл курса:") + fileEl.attribute("fileName"), 0, 0, 0);
		fileN = getFileName(fileEl.attribute("fileName"));
		loadCourseData(fileN);
		if (cursFile != fileN) {
			return;
		}
	}

	QFileInfo fi_kurs = QFileInfo(fileN);
	curDir = fi_kurs.absolutePath();
	QDomNodeList marksElList = root.elementsByTagName("MARK"); //Оценки
	qDebug() << "Loading marks " << marksElList.count();
	for (int i = 0; i < marksElList.count(); i++) {
		int taskId = marksElList.at(i).toElement().attribute("testId").toInt();
		int mark = marksElList.at(i).toElement().attribute("mark").toInt();
		qDebug() << "task:" << taskId << " mark:" << mark;
		course->setMark(taskId, mark);
		changes->setMark(taskId, mark);
	}

	qDebug() << "Loading user prgs...";
	QDomNodeList prgElList = root.elementsByTagName("USER_PRG"); //Программы
	for (int i = 0; i < prgElList.count(); i++) {
		int taskId = prgElList.at(i).toElement().attribute("testId").toInt();
		qDebug() << "Tassk id" << taskId;
		QString prg = prgElList.at(i).toElement().attribute("prg");

		if (progChange.indexOf(taskId) == -1) {

			progChange.append(taskId);
		};
		course->setUserText(taskId, prg);
		ui->treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	}
}

void MainWindowTask::loadCourse()
{
	//qDebug()<<"Fix 4 EDITOR!";

	editRoot->hide();
	QString dir = settings->value("Directories/Kurs", "").toString();
	QDir chD(dir);
	if (!chD.exists()) {
		dir = QDir::homePath();
	}
//     QFileDialog dialog(this,trUtf8("Открыть файл"),dir, "(*.kurs.xml)");
//     dialog.setAcceptMode(QFileDialog::AcceptOpen);
//     if(!dialog.exec())return;

	QString fname = QFileDialog::getOpenFileName(this, QString::fromUtf8("Открыть файл"), dir,  "(*.kurs.xml)");

	QFileInfo fi(fname);
	if (!fi.exists()) {
		return;
	}

	baseKursFile = fi;
	curDir = fi.absolutePath();

	settings->setValue("Directories/Kurs", curDir);
	qDebug() << "curDir" << curDir;
	QString fileName = fname;
	progChange.clear();
	setWindowTitle(trUtf8("Редактируем - ") + fi.baseName());
	cursWorkFile.setFileName("");
	loadCourseData(fileName);
	ui->descEdit->setText("");
	ui->prgEdit->setText("");
	ui->ispList->clear();
	ui->fieldsList->clear();

	lockEditFields();
	ui->actionS->setEnabled(true);
}

void MainWindowTask::remSelIsp()
{
	QList<QListWidgetItem *> items = ui->ispList->selectedItems();
	for (int i = 0; i < items.count(); i++) {
		ui->ispList->removeItemWidget(items.at(i));
		course->removeModule(curTaskIdx.internalId(), items.at(i)->text());
	};
	refreshIspsNEnv();

}

void MainWindowTask::setUpDown(QModelIndex index)
{
	if (!isTeacher) {
		ui->actionup->setEnabled(false);
		ui->actionDown->setEnabled(false);
	}
	return;
	if (course->hasUpSib(index)) {
		ui->actionup->setEnabled(true);
	} else {
		ui->actionup->setEnabled(false);
	}
	if (course->hasDownSib(index)) {
		ui->actionDown->setEnabled(true);
	} else {
		ui->actionDown->setEnabled(false);
	}
}

void MainWindowTask::moveUp()
{
	curTaskIdx = course->moveUp(curTaskIdx);
	QModelIndex par = curTaskIdx.parent();
	ui->treeView->setCurrentIndex(curTaskIdx);
	setUpDown(curTaskIdx);


	ui->treeView->collapse(par);
	ui->treeView->expand(par);
	saveBaseKurs();
}

void MainWindowTask:: moveDown()
{
	ui->treeView->setCurrentIndex(curTaskIdx);

	curTaskIdx = course->moveDown(curTaskIdx);
	ui->treeView->setCurrentIndex(curTaskIdx);
	setUpDown(curTaskIdx);
	//ui->treeView->dataChanged(curTaskIdx,ui->treeView->indexAbove(curTaskIdx));
	//showText(curTaskIdx);
	// ui->treeView->update(curTaskIdx);

	ui->treeView->collapse(curTaskIdx.parent());
	ui->treeView->expand(curTaskIdx.parent());
}

void MainWindowTask::unLockEditFields()
{
	ui->descEdit->setEnabled(true);
	ui->prgEdit->setEnabled(true);
	ui->ispList->setEnabled(true);
	ui->fieldsList->setEnabled(true);
	ui->addIspButt->setEnabled(true);
	ui->remIspButt->setEnabled(true);
	ui->selFileButt->setEnabled(true);
	ui->ispSel->setEnabled(true);
	ui->typeGroup->setEnabled(true);
}

void MainWindowTask::lockEditFields()
{
	ui->descEdit->setEnabled(false);
	ui->prgEdit->setEnabled(false);
	ui->ispList->setEnabled(false);
	ui->fieldsList->setEnabled(false);
	ui->addIspButt->setEnabled(false);
	ui->remIspButt->setEnabled(false);
	ui->selFileButt->setEnabled(false);
	ui->ispSel->setEnabled(false);
	ui->typeGroup->setEnabled(false);
}

void MainWindowTask::save2Tree()
{
	if (!curTaskIdx.isValid()) {
		return;
	}
	course->setDesc(curTaskIdx.internalId(), ui->descEdit->toPlainText().replace('\n', "<BR>"));
	qDebug() << "Desc:" + ui->descEdit->toPlainText();
	qDebug() << "Repl:" + ui->descEdit->toPlainText().replace('\n', "<BR>");

	if (curTaskIdx.internalId() > 0) {
		course->setProgram(curTaskIdx.internalId(), ui->prgEdit->text());
	}
	QFileInfo fi(curDir + '/' + ui->prgEdit->text());
	if (!fi.isReadable()) {
		QMessageBox::about(NULL, trUtf8("Предупреждение"), trUtf8("Нет файла:") + ui->prgEdit->text());
	};

	course->setTask(curTaskIdx.internalId(), ui->nodeRb->isChecked());
	if (ui->nodeRb->isChecked()) {
		course->setMark(curTaskIdx.internalId(), 11);
	}
	changed = true;
}

void MainWindowTask::showText(const QModelIndex &index)
{
	enableMkSect(false);

	if (index == curTaskIdx) {
		return;
	}

	if (curTaskIdx.isValid()) {
		save2Tree();
	}

	editRoot->hide();
	qDebug() << "TASK IDX:" << curTaskIdx.internalId();

	// if(curTaskIdx.isValid())save2Tree();

	if (index.internalId() > 0) {
		setEditTaskEnabled(true);

	} else {
		qDebug() << "root";
		setEditTaskEnabled(false);
		ui->nodeRb->setChecked(true);
		ui->zadRb->setEnabled(false);
	}

	setUpDown(index);
	if (index == curTaskIdx) {
		return;
	}

	unLockEditFields();

	onTask = false;
	ui->checkTask->setEnabled(false);
	QString taskText = course->getTaskText(index);

	ui->descEdit->setText(course->getTaskText(index));
	qDebug() << "TaskText:" << course->getTaskText(index);
	curTaskIdx = index;
	ui->zadRb->setEnabled(true);
	if (course->isTask(curTaskIdx.internalId()) && curTaskIdx.internalId() > 0) {

		QString progFile = course->progFile(curTaskIdx.internalId());
		ui->prgEdit->setText(progFile);
		ui->zadRb->setChecked(true);
		ui->zadRb->setEnabled(true);
		ui->nodeRb->setChecked(false);
		setType();
		refreshIspsNEnv();
	}

	if (!course->isTask(curTaskIdx.internalId())) {
		qDebug() << "!Task";
		ui->zadRb->setChecked(false);
		ui->nodeRb->setChecked(true);
		setType();
	}

	if (curTaskIdx.internalId() == 0) { //ROOT NODE
		ui->prgEdit->setEnabled(false);
		ui->ispList->setEnabled(false);
		ui->fieldsList->setEnabled(false);
		ui->selFileButt->setEnabled(false);
		ui->addIspButt->setEnabled(false);
		ui->ispSel->setEnabled(false);
		ui->zadRb->setEnabled(false);
	}
}

void MainWindowTask::refreshIspsNEnv()
{
	task->isps = course->Modules(curTaskIdx.internalId());
	qDebug() << "ISPS" << task->isps;
	ui->ispList->clear();
	ui->fieldsList->clear();
	ui->addFieldButt->setEnabled(false);
	for (int i = 0; i < task->isps.count(); i++) {
		ui->ispList->addItem(task->isps[i]);
		// task.Scripts.append(loadScript(course->Script(curTaskIdx.internalId(),task.isps[i])));
		QStringList t_fields = course->Fields(curTaskIdx.internalId(), task->isps[i]);
		qDebug() << "fields" << t_fields;
		task->fields.clear();
		for (int j = 0; j < t_fields.count(); j++) {

			task->fields.insertMulti(task->isps[i], curDir + '/' + t_fields[j]);
			qDebug() << curDir + '/' + t_fields[j];

		};
		qDebug() << "Fields!!!!" << task->fields;
	}

	if (task->isps.count() == 1) {
		ui->fieldsList->addItems(course->Fields(curTaskIdx.internalId(), task->isps[0]));
		ui->ispList->setCurrentItem(ui->ispList->item(0));
		ui->addFieldButt->setEnabled(true);
	}

	ui->remFieldButt->setEnabled(false);
}

void MainWindowTask::loadHtml(QString fileName)
{
	qDebug() << "LoadHtml" << fileName;
	if (fileName.isEmpty()) {
		return;
	}
	QFile inp(curDir + '/' + fileName);
	if (!inp.open(QIODevice::ReadOnly)) {
		QMessageBox::information(0, "", trUtf8("Ошибка чтения: ") + fileName, 0, 0, 0);
		return;
	};
	QString htmlData = QString::fromUtf8(inp.readAll());
	//ui->textBrowser->setHtml(htmlData);

	// ui->webView->setHtml(htmlData,QUrl("file://"+curDir+'/'+fileName));
	inp.close();

}

void MainWindowTask::startTask()
{
	qDebug() << "Dummy call MainWindowTask::startTask";
	return;

	// editRoot->hide();
	qDebug() << "StartTask";
	if (curTaskIdx.internalId() <= 0) {
		QMessageBox::about(NULL, trUtf8("Не выбрано задание"), trUtf8("Необходимо выбрать задание"));
		return;
	}
	if (course->csName(curTaskIdx.internalId()) != CS) {
		QMessageBox::about(NULL, trUtf8("Неправильное окружение"), trUtf8("Необходим ") + course->csName(curTaskIdx.internalId()));
		return;
	}

	QString progFile = course->progFile(curTaskIdx.internalId());
	_interface->setTesting(loadTestAlg(course->getTaskCheck(curTaskIdx)));
	task->isps = course->Modules(curTaskIdx.internalId());
	qDebug() << "ISPS" << task->isps;
	for (int i = 0; i < task->isps.count(); i++) {

		// task.Scripts.append(loadScript(course->Script(curTaskIdx.internalId(),task.isps[i])));
		QStringList t_fields = course->Fields(curTaskIdx.internalId(), task->isps[i]);
		qDebug() << "fields" << t_fields;
		task->fields.clear();
		for (int j = 0; j < t_fields.count(); j++) {

			task->fields.insertMulti(task->isps[i], curDir + '/' + t_fields[j]);
			qDebug() << curDir + '/' + t_fields[j];

		};
		qDebug() << "Fields!!!!" << task->fields;
	}
	qDebug() << "MODULES:" << course->Modules(curTaskIdx.internalId());
	if (!_interface->startNewTask(course->Modules(curTaskIdx.internalId()))) {
		QMessageBox::about(NULL, trUtf8("Невозможно выполнить задание"), trUtf8("Нет неоходимых исполнителей"));
	}
	if (course->getUserText(curTaskIdx.internalId()) != "") {
		_interface->setPreProgram(QVariant(course->getUserText(curTaskIdx.internalId())));
		ui->actionReset->setEnabled(true);
	} else if (!progFile.isEmpty()) {
		_interface->setPreProgram(QVariant(curDir + '/' + progFile));
	}

//qDebug()<<"Scripts "<<task.Scripts;
	ui->do_task->setEnabled(false);
	ui->checkTask->setEnabled(true);
	onTask = true;
//ui->loadCurs->setEnabled(false);
	qDebug() << "end load task";
	if (progChange.indexOf(curTaskIdx.internalId()) == -1) {
		progChange.append(curTaskIdx.internalId());
	}
}

void MainWindowTask::checkTask()
{
	qDebug() << "CheckTASK";
	if (!onTask) {
		qDebug() << "!onTASK";
		return;
	}

	markProgChange();
	if (!cursWorkFile.exists()) {

		QMessageBox::information(0, "", trUtf8("Необходимо сохранить работу "), 0, 0, 0);
		saveCourse();
	};
	course->setMark(curTaskIdx.internalId(), 0);
	_interface->startProgram(QVariant("TODO LOAD SCRIPT"));
	ui->treeView->setEnabled(false);
	ui->loadCurs->setEnabled(true);
}

QString MainWindowTask::loadScript(QString file_name)
{
	qDebug() << "Script file name:" << file_name;
	if (file_name.isEmpty()) {
		return "";
	}
	QFile file(curDir + "/" + file_name);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::about(NULL, trUtf8("Ошибка"), trUtf8("Невозможно открыть ") + curDir + "/" + file_name);
		return "";
	}
	return file.readAll();
}

void MainWindowTask::saveCourse()
{
	return;

}

void MainWindowTask::saveCourseFile()
{
	qDebug() << "Save cource file";
	QDomDocument saveXml;

	QDomElement crsEl = saveXml.createElement("COURSE");
	QDomElement fileEl = saveXml.createElement("FILE");
	QDomAttr crsAtt = saveXml.createAttribute("fileName");
	crsAtt.setValue(cursFile);
	fileEl.setAttributeNode(crsAtt);

	crsEl.appendChild(fileEl);
	saveXml.appendChild(crsEl);
	QDomElement mrksEl = saveXml.createElement("MARKS");

	//USER PROGRAMS n TESTED PROGRAMS
	for (int i = 0; i < progChange.count(); i++) {
		QDomElement prgEl = saveXml.createElement("USER_PRG");
		QDomAttr testIdprg = saveXml.createAttribute("testId");
		testIdprg.setValue(QString::number(progChange[i]));
		QDomAttr userPrg = saveXml.createAttribute("prg");
		userPrg.setValue(course->getUserText(progChange[i]));
		prgEl.setAttributeNode(testIdprg);
		prgEl.setAttributeNode(userPrg);
		crsEl.appendChild(prgEl);

		QDomElement prgElT = saveXml.createElement("TESTED_PRG");
		QDomAttr testIdprgT = saveXml.createAttribute("testId");
		testIdprgT.setValue(QString::number(progChange[i]));
		QDomAttr userPrgT = saveXml.createAttribute("prg");
		userPrgT.setValue(course->getUserTestedText(progChange[i]));
		prgElT.setAttributeNode(testIdprg);
		prgElT.setAttributeNode(userPrgT);
		crsEl.appendChild(prgElT);
	}
	//END USER PROGRAMS

	QMapIterator<int, int> i(changes->marksChanged);
	while (i.hasNext()) {
		i.next();
		QDomElement mrk = saveXml.createElement("MARK");
		QDomAttr testId = saveXml.createAttribute("testId");
		testId.setValue(QString::number(i.key()));
		QDomAttr mvalue = saveXml.createAttribute("mark");
		mvalue.setValue(QString::number(i.value()));
		mrk.setAttributeNode(testId);
		mrk.setAttributeNode(mvalue);
		mrksEl.appendChild(mrk);
	}
	crsEl.appendChild(mrksEl);

	if (!cursWorkFile.open(QIODevice::WriteOnly)) {
		QMessageBox::information(0, "", trUtf8("Ошибка записи: ") + cursWorkFile.fileName(), 0, 0, 0);
		return;
	}
	cursWorkFile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	cursWorkFile.write(saveXml.toByteArray());
	cursWorkFile.close();
}

void MainWindowTask::markProgChange()
{
	qDebug() << "Dummy call MainWindowTask::markProgChange()";
	return;
	course->setUserText(curTaskIdx, _interface->getText());
	if (progChange.indexOf(curTaskIdx.internalId()) == -1) {
		progChange.append(curTaskIdx.internalId());
	}
	if (!cursWorkFile.exists()) {
		qDebug() << "!cursWorkFile";
	} else {
		saveCourseFile();
	}

}

QString MainWindowTask::getFileName(QString fileName)
{
	QFileInfo finf(fileName);

	QFileDialog dialog(this, trUtf8("Открыть файл"), curDir, "(" + finf.fileName() + ")");
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	if (!dialog.exec()) {
		return "";
	}
	QFileInfo fi(dialog.selectedFiles().first());

	return dialog.selectedFiles().first();
}

void MainWindowTask::Close()
{
	qDebug() << cursFile;
	//if(cursFile!="")markProgChange();
	//saveBaseKurs();
	close();
}

void MainWindowTask::closeEvent(QCloseEvent *event)
{
	if (!course) {
		close();
		return;
	}

	qDebug() << "START CLOSE TASK WINDOW";
	if (changed) {
		QMessageBox::StandardButton ans;
		ans = QMessageBox::question(this, trUtf8("Курсы"), trUtf8("Вы хотите сохранить работу?"),
				QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
		if (ans == QMessageBox::No)

		{
			event->accept();
			if (course) {
				delete course;
			}
			course = NULL;
			return;
		};
		if (ans == QMessageBox::Cancel) {
			event->ignore();
			return;
		};

		saveBaseKurs();
	}

	qDebug() << "CLOSE TASK WINDOW";
	close();
}

void MainWindowTask::returnTested()
{
	_interface->setPreProgram(QVariant(course->getUserTestedText(curTaskIdx.internalId())));
}

QString MainWindowTask::loadTestAlg(QString file_name)
{
	if (file_name.isEmpty()) {
		return "";
	}
	QFile file(curDir + "/" + file_name);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {

		QMessageBox::about(NULL, trUtf8("Ошибка"), trUtf8("Невозможно открыть ") + curDir + "/" + file_name);
		return "";
	};
	QTextStream ts(&file);
	ts.setCodec("UTF-16LE");
	QString testalg = ts.readAll();
	file.close();

	qDebug() << "Test alg" << testalg;
	return testalg;
}

void MainWindowTask::aboutToQuit()
{
	close();
}

void MainWindowTask::enableMkSect(bool flag)
{
	qDebug() << "!Flag:" << !flag;

	ui->actionAdd->setEnabled(!flag);
	ui->actionDown->setEnabled(!flag);
	ui->actionup->setEnabled(!flag);
	ui->addDeep->setEnabled(!flag);
	ui->actionRemove->setEnabled(flag);
	ui->actionMakeSection->setEnabled(flag);
}

void MainWindowTask:: customContextMenuRequested(QPoint  pos)
{
	editRoot->hide();
	createMoveMenu();
	if (ui->treeView->selectionModel()->selectedIndexes().count() > 1) { //Many items selected.
		qDebug() <<  "enableMkSect(true);";
		enableMkSect(true);

	} else {
		enableMkSect(false);
		curTaskIdx = ui->treeView->currentIndex();
		ui->actionEdit->setEnabled(true);
		if (curTaskIdx.internalId() == 0) {
			setEditTaskEnabled(false);
			ui->actionEdit->setEnabled(true);

			ui->addDeep->setEnabled(false);
		} else {
			setEditTaskEnabled(true);
		}
	}

	if (!baseKursFile.isReadable()) {
		return;
	}

	pos.setY(pos.y() + ui->mainToolBar->height());
	customMenu.move(pos + this->pos() + ui->centralWidget->pos());
	customMenu.show();
	qDebug() << "Menu Request!!!";
}

void MainWindowTask::addTask()
{
	save2Tree();
	qDebug() << "Add task";
	QModelIndex par = curTaskIdx.parent();
	course->addSiblingTask(curTaskIdx.internalId());
	ui->treeView->collapse(par);
	ui->treeView->expand(par);
}

void MainWindowTask::addDeepTask()
{
	save2Tree();
	qDebug() << "Add deep task";
	QModelIndex par = curTaskIdx.parent();
	course->addDeepTask(curTaskIdx.internalId());
	ui->treeView->collapse(par);
	ui->treeView->expand(par);
//saveBaseKurs();
	if (curTaskIdx.internalId() == 0) {
		// loadCourseData(baseKursFile.absoluteFilePath());
	}
}

void MainWindowTask::deleteTask()
{
	QModelIndex par = curTaskIdx.parent();
//course->removeNode(curTaskIdx.internalId());
	QModelIndexList list2del = ui->treeView->selectionModel()->selectedIndexes(); //list of indexes to remove nodes;

	for (int i = 0; i < list2del.count(); i++) {
		qDebug() << course->getTitle(list2del.at(i).internalId());
		course->removeNode(list2del.at(i).internalId());
	}
	ui->treeView->collapse(par);
	ui->treeView->expand(par);
	ui->treeView->setCurrentIndex(QModelIndex());
	curTaskIdx = QModelIndex();
	lockEditFields();
}


void MainWindowTask::makeSection()
{
	QModelIndexList list2sec = ui->treeView->selectionModel()->selectedIndexes();
	QModelIndex rootIdx;
	for (int i = 0; i < list2sec.count(); i++) { //Ищем общий корень
		if (list2sec.at(i).internalId() == 0) {
			return;
		}
		QModelIndex  par = course->parent(list2sec.at(i));

		if (list2sec.indexOf(par) == -1) {
			qDebug() << "Par:" << course->getTitle(par.internalId());
			rootIdx = par;
		}
	};
	if (list2sec.isEmpty()) {
		return;
	}

	int newSecId = course->addNewTask(rootIdx.internalId(), list2sec.first().internalId());

	for (int i = list2sec.count() - 1; i > -1; i--) {
		QModelIndex  par = course->parent(list2sec.at(i));
		if (par == rootIdx) {
			qDebug() << "2Mov:" << course->getTitle(list2sec.at(i).internalId());
			course->moveTask(newSecId, list2sec.at(i).internalId());
		}
	};
	ui->treeView->collapse(rootIdx);
	ui->treeView->expand(rootIdx);
}

void MainWindowTask::saveBaseKurs()
{
	qDebug() << "Save Base Kurs";
	save2Tree();
	if (lockFile.isOpen()) {
		lockFile.close();
	}
	QFile cursKursFile(baseKursFile.absoluteFilePath());
	if (!cursKursFile.open(QIODevice::WriteOnly)) {
		QMessageBox::information(0, "", trUtf8("Ошибка записи: ") + cursKursFile.fileName(), 0, 0, 0);
		return;
	};

	cursKursFile.write(course->document()->toByteArray());
	cursKursFile.close();
	setWindowTitle(baseKursFile.baseName());
	lockKursFile(baseKursFile.absoluteFilePath());
	changed = false;
}

void MainWindowTask::saveKursAs()
{
	QString dir = curDir;
	QDir chD(curDir);
	if (!chD.exists()) {
		dir = QDir::homePath();
	}
	QFileDialog dialog(this, trUtf8("Сохранить файл курса"), dir, "(*.kurs.xml )");
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	if (!dialog.exec()) {
		return;
	}
	QString fileName = dialog.selectedFiles().first();
	if (!fileName.endsWith(".kurs.xml")) {
		fileName += ".kurs.xml";
	}
	QFileInfo fi(fileName);
	baseKursFile = fi;
	saveBaseKurs();
}

void MainWindowTask::editTask()
{
	QRect rect = ui->treeView->visualRect(curTaskIdx);
	editRoot->resize(rect.width(), rect.height());
	editRoot->setText(course->getTitle(curTaskIdx.internalId()));
	editRoot->move(rect.topLeft());
	editRoot->show();

	return;

	QModelIndex par = curTaskIdx.parent();

	editDialog->setTitle(course->getTitle(curTaskIdx.internalId()));
	editDialog->setDesc(course->getTaskText(curTaskIdx));
	editDialog->setProgram(course->progFile(curTaskIdx.internalId()));
	QStringList isps = course->Modules(curTaskIdx.internalId());

	if (isps.count() > 0) {
		editDialog->setUseIsps(isps.first());
	} else {
		editDialog->setUseIsps("");
	}

	if (course->Modules(curTaskIdx.internalId()).count() > 0) {
		editDialog->setEnvs(course->Fields(curTaskIdx.internalId(), isps.first()));
	} else {
		editDialog->setEnvs(QStringList());
	}

	editDialog->setCurDir(curDir);

	if (editDialog->exec()) {
		course->setUserText(curTaskIdx.internalId(), "");
		course->setTitle(curTaskIdx.internalId(), editDialog->getTitle());
		course->setDesc(curTaskIdx.internalId(), editDialog->getDesc());
		qDebug() << "Desc" << editDialog->getDesc();
		course->setProgram(curTaskIdx.internalId(), editDialog->getProgram());
		qDebug() << "EDIT ISPS" << editDialog->getUseIsps();


		course->setIsps(curTaskIdx, editDialog->getUseIsps());
		qDebug() << "PRG" << editDialog->getProgram();
		if (course->Modules(curTaskIdx.internalId()).count() > 0) {
			course->setIspEnvs(curTaskIdx, course->Modules(curTaskIdx.internalId()).first(), editDialog->getEnvs());
		}
		showText(curTaskIdx);
		ui->treeView->collapse(par);
		ui->treeView->expand(par);
		qDebug() << "Set task isps:" << course->Modules(curTaskIdx.internalId());
		qDebug() << "EDIT DIALOG EXEC OK";
		saveBaseKurs();
	}
}

void MainWindowTask::setEditTaskEnabled(bool flag)
{
	// ui->actionEdit->setEnabled(flag);
	ui->actionup->setEnabled(flag);
	ui->actionDown->setEnabled(flag);

	ui->actionSaveKas->setEnabled(flag);
	ui->actionAdd->setEnabled(flag);
	ui->actionRemove->setEnabled(flag);
	ui->addDeep->setEnabled(flag);
	ui->typeGroup->setEnabled(flag);
}

void MainWindowTask::newKurs()
{
	if (newDialog->exec()) {
		QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить курс..."), curDir, "*.kurs.xml");

		QFile newKurs(fileName);

		if (!newKurs.open(QIODevice::WriteOnly)) {
			QMessageBox::information(0, "", trUtf8("Ошибка записи: ") + fileName, 0, 0, 0);
			return;
		};
		QString toWr = "<?xml version='1.0' encoding='UTF-8'?>\n";
		newKurs.write(toWr.toUtf8());
		toWr = "<KURS xml:id=\"0\" xml:name=\"" + newDialog->name() + "\">\n";
		newKurs.write(toWr.toUtf8());

		toWr = QString::fromUtf8("<T xml:id=\"1\" xml:name=\"Новое задание\">\n<DESC>Нет Описания</DESC>\n<CS>Кумир</CS>\n <ISP xml:ispname=\"Robot\">\n</ISP>\n<READY>false</READY>\n</T>\n");
		newKurs.write(toWr.toUtf8());

		toWr = "</KURS>\n";
		newKurs.write(toWr.toUtf8());
		newKurs.close();
		loadCourseData(newKurs.fileName());
		baseKursFile = QFileInfo(newKurs);
		curDir = baseKursFile.absolutePath();
		setWindowTitle(trUtf8("Редактируем - ") + baseKursFile.baseName());
		setEditTaskEnabled(true);
		ui->actionS->setEnabled(true);
	}
}

void MainWindowTask::endRootEdit()
{
	course->setTitle(curTaskIdx.internalId(), editRoot->text());
	editRoot->hide();
}

void MainWindowTask::cancelRootEdit()
{
	editRoot->hide();
}

void MainWindowTask::addIsp()
{
	QString isp = ui->ispSel->currentText();
	qDebug() << "2 add" << isp;
	if (ui->ispList->findItems(isp, Qt::MatchExactly).count() == 0) {
		course->addModule(curTaskIdx, isp);
		refreshIspsNEnv();
	}
}

void MainWindowTask::showFields()
{
	qDebug() << "Show fields 4:" << ui->ispList->currentItem()->text();
	ui->fieldsList->clear();
	ui->fieldsList->addItems(course->Fields(curTaskIdx.internalId(), ui->ispList->currentItem()->text()));
	ui->addFieldButt->setEnabled(true);
}

void MainWindowTask::fieldClick()
{
	ui->remFieldButt->setEnabled(true);
}

void MainWindowTask::remField()
{
	QString isp = ui->ispList->currentItem()->text();
	course->removeEvn(course->ispNodeByName(curTaskIdx, isp), ui->fieldsList->currentItem()->text());
	refreshIspsNEnv();
}

QString MainWindowTask::getFileTypes()
{
	if (ui->ispList->currentItem()->text() == "Robot") {
		return "(*.fil)";
	}
	if (ui->ispList->currentItem()->text() == trUtf8("Водолей")) {
		return "(*.vod)";
	}
	return "(*.*)";
}

void MainWindowTask::addField()
{
	QString dir = curDir;
	QDir chD(curDir);
	QString fileTypes = getFileTypes();

	if (!chD.exists()) {
		dir = QDir::homePath();
	}

	QFileDialog dialog(this, trUtf8("Выбор файла обстановки"), dir, fileTypes);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	if (!dialog.exec()) {
		return;
	}
	QString isp = ui->ispList->currentItem()->text();
	for (int i = 0; i < dialog.selectedFiles().count(); i++) {
		QFileInfo fi(dialog.selectedFiles().at(i));

		course->addEnv(course->ispNodeByName(curTaskIdx, isp), chD.relativeFilePath(fi.absoluteFilePath()));
	};
	refreshIspsNEnv();
}

void MainWindowTask::setPrg()
{
	QString dir = curDir;
	QDir chD(curDir);
	if (!chD.exists()) {
		dir = QDir::homePath();
	}
	QFileDialog dialog(this, trUtf8("Выбор файла программы"), dir, "(*.kum )");
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	if (!dialog.exec()) {
		return;
	}
	QFileInfo fi(dialog.selectedFiles().first());
	ui->prgEdit->setText(chD.relativeFilePath(fi.absoluteFilePath()));
	course->setProgram(curTaskIdx.internalId(), ui->prgEdit->text());
}

void MainWindowTask::startEdit(const QModelIndex &index)
{
	showText(index);
	editTask();
}

void  MainWindowTask::setType()
{
	if (ui->zadRb->isChecked()) {
		ui->nodeRb->setChecked(false);
		ui->prgEdit->setEnabled(true);
		ui->ispList->setEnabled(true);
		ui->fieldsList->setEnabled(true);
		ui->addIspButt->setEnabled(true);
		ui->remIspButt->setEnabled(true);
		ui->selFileButt->setEnabled(true);
		ui->ispSel->setEnabled(true);
		ui->frame_2->show();
		ui->frame_7->show();
		ui->label_3->show();


	};
	if (ui->nodeRb->isChecked()) {
		if (ui->zadRb->isChecked()) {
			ui->zadRb->setChecked(false);
		}
		ui->prgEdit->setEnabled(false);
		ui->ispList->setEnabled(false);
		ui->fieldsList->setEnabled(false);
		ui->addIspButt->setEnabled(false);
		ui->remIspButt->setEnabled(false);
		ui->selFileButt->setEnabled(false);
		ui->ispSel->setEnabled(false);
		ui->frame_2->hide();
		ui->frame_7->hide();
		ui->label_3->hide();
	}

	save2Tree();
}

void MainWindowTask::prgLineChange()
{
	qDebug() << "Prg line change";
	QFileInfo fi(curDir + "/" + ui->prgEdit->text());
	qDebug() << fi.absoluteFilePath();
	if (fi.isReadable() && fi.isFile()) {

		ui->editFButt->setEnabled(true);
		course->setProgram(curTaskIdx.internalId(), ui->prgEdit->text());
	} else {
		ui->editFButt->setEnabled(false);
	}
}

void MainWindowTask::createRescentMenu()
{
	ui->rescentMenu->clear();

	if (lastFiles.count() == 0) {
		ui->rescentMenu->setEnabled(false);
	} else {
		ui->rescentMenu->setEnabled(true);
	}
	settings->setValue("RescentFiles", lastFiles);

	for (int i = 0; i < lastFiles.count(); i++) {
		QAction *action = ui->rescentMenu->addAction("&" + QString::number(i + 1) + " " + lastFiles[i], this, SLOT(openRecent()));
		Q_UNUSED(action);
	}
}

void MainWindowTask::openRecent()
{
	QAction *s = qobject_cast<QAction *>(sender());
	QString txt = s->text();
	txt.remove(0, 1);
	QStringList words = txt.split(" ");
	int no = words[0].toInt();
	QString fileName = lastFiles[no - 1];
	QFileInfo fi(fileName);
	cursWorkFile.setFileName("");
	loadCourseData(fileName);
	setWindowTitle(trUtf8("Редактируем - ") + fi.baseName());
	ui->descEdit->setText("");
	ui->prgEdit->setText("");
	ui->ispList->clear();
	ui->fieldsList->clear();

	lockEditFields();
	ui->actionS->setEnabled(true);
}

bool MainWindowTask::checkInList(int id, QModelIndexList list) const//Поиск id среди списка индексов
{
	for (int i = 0; i < list.count(); i++) { //Ищем общий корень
		if ((size_t) list.at(i).internalId() == (size_t) id) {
			return true;
		}
	}
	return false;
}

void MainWindowTask::createMoveMenu()
{
	ui->menuMove->clear();
	QModelIndexList list2sec = ui->treeView->selectionModel()->selectedIndexes();
	QModelIndex rootIdx;
	for (int i = 0; i < list2sec.count(); i++) { //Ищем общий корень
		if (list2sec.at(i).internalId() == 0) {
			return;
		}

		QModelIndex  par = course->parent(list2sec.at(i));

		if (list2sec.indexOf(par) == -1) {
			qDebug() << "Par:" << course->getTitle(par.internalId());
			rootIdx = par;
		}
	}

	QModelIndex  rt = course->parent(rootIdx);
	if (curTaskIdx == rt) {
		return;
	}

	QAction *action = ui->menuMove->addAction(course->getTitle(rt.internalId()), this, SLOT(move()));
	action->setData(QVariant(rt.internalId()));
	ui->menuMove->addSeparator();
	QList<KumTask> ct = course->childTasks(rt);
	for (int i = 0; i < ct.count(); i++) {
		int ctId = ct.at(i).getId();

		if (
			(size_t) ctId == (size_t) rootIdx.internalId() ||
			(size_t) ctId == (size_t) curTaskIdx.internalId() ||
			checkInList(ctId, list2sec)
		) {
			continue;
		}

		QAction *action = ui->menuMove->addAction(ct.at(i).getTitle(), this, SLOT(move()));
		action->setData(QVariant(ctId));
	}
}


void MainWindowTask::move()
{
	QAction *s = qobject_cast<QAction *>(sender());
	int id = s->data().toInt();

	QModelIndexList list2sec = ui->treeView->selectionModel()->selectedIndexes();
	if (list2sec.count() == 0) {
		return;
	}
	QModelIndex par = list2sec.first().parent();
	for (int i = list2sec.count() - 1; i > -1; i--) {
		course->moveTask(id, list2sec.at(i).internalId());
	};
	ui->treeView->collapse(par);
	ui->treeView->expand(par);
	ui->treeView->collapse(par.parent());
	ui->treeView->expand(par.parent());
}

void MainWindowTask::editFile()
{
	QProcess   *kum = new QProcess();
#ifdef Q_OS_MAC
	kum->start(QApplication::applicationDirPath() + "/../../../kumir.app/Contents/MacOS/kumir -t " + curDir + "/" + ui->prgEdit->text());
	qDebug() << QApplication::applicationDirPath() + "/../../../kumir.app/Contents/MacOS/kumir -t " + curDir + "/" + ui->prgEdit->text();
#else
#ifdef Q_OS_WIN
	kum->start("\"" + QApplication::applicationDirPath() + "\\kumir.exe\" -t \"" + curDir + "\\" + ui->prgEdit->text() + "\"");
#else
	kum->start(QApplication::applicationDirPath() + "/kumir -t " + curDir + "/" + ui->prgEdit->text());
#endif
#endif
	\
}

