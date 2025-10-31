#ifndef COURSE_MODEL_H
#define COURSE_MODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QFont>
#include <QDomDocument>
#include <QIcon>
#include <QDebug>


class KumTask
{
public:
	KumTask(int id, QString title, QString desc = QString(), int chapter_id = 0)
	{
		Id = id;
		Title = title;
		Desc = desc;
		Chapter_id = chapter_id;

	}

	int getId() const
	{
		return Id;
	}

	QString getTitle() const
	{
		return Title;
	}

	QStringList getIsps() const
	{
		return ispsNeeded;
	}

	QStringList getEnvFiles() const
	{
		return envFiles;
	}

	int Chapter() const 
	{
		return Chapter_id;
	}

	QString Description() const
	{
		return Desc;
	}

	void appendEnvFile(const QString &envFile)
	{
		envFiles.append(envFile);
	}

	int next() const
	{
		return nextId;
	}

	void setNext(int id)
	{
		nextId = id;
	}

	void addSubTask(KumTask *task)
	{
		subTasks.append(task);
	}

	int subTasksCount() const
	{
		return subTasks.count();
	}

private:
	int Id;
	QString Title;
	QStringList ispsNeeded;
	QStringList envFiles;
	QString Desc;
	int Chapter_id;
	QList<int> dependes;
	int nextId;
	QString controlScript;
	QList<KumTask *> subTasks;
};

class courseChanges
{
public:
	courseChanges() {};
	void cleanChanges()
	{
		marksChanged.clear();
	}

	void setMark(int testId, int newMark)
	{
		marksChanged.insert(testId, newMark);
	}

	QMap<int, int> marksChanged;
};


class courseModel: public QAbstractItemModel
{
	Q_OBJECT
public:
	courseModel() : QAbstractItemModel()
	{
		itemFont = QFont("Helvetica [Cronyx]");
#ifdef Q_OS_WIN32
		markIcons.append(QIcon(":/x.png"));
		markIcons.append(QIcon(":/1.png"));
		markIcons.append(QIcon(":/2.png"));
		markIcons.append(QIcon(":/3.svg"));
		markIcons.append(QIcon(":/4.svg"));
		markIcons.append(QIcon(":/5.svg"));
		markIcons.append(QIcon(":/6.svg"));
		markIcons.append(QIcon(":/7.png"));
		markIcons.append(QIcon(":/8.png"));
		markIcons.append(QIcon(":/9.png"));
		markIcons.append(QIcon(":/10.png"));
		markIcons.append(QIcon(":/m.png"));
		return;
#endif
		markIcons.append(QIcon(":/x.svg"));
		markIcons.append(QIcon(":/1.svg"));
		markIcons.append(QIcon(":/2.svg"));
		markIcons.append(QIcon(":/3.svg"));
		markIcons.append(QIcon(":/4.svg"));
		markIcons.append(QIcon(":/5.svg"));
		markIcons.append(QIcon(":/6.svg"));
		markIcons.append(QIcon(":/7.svg"));
		markIcons.append(QIcon(":/8.svg"));
		markIcons.append(QIcon(":/9.svg"));
		markIcons.append(QIcon(":/10.svg"));
		markIcons.append(QIcon(":/m.png"));
		isTeacher = false;
	}

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(
		int section,
		Qt::Orientation orientation,
		int role = Qt::DisplayRole
	) const;
	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &child) const;
	int columnCount(const QModelIndex &parent)const ;
	Qt::ItemFlags flags(const QModelIndex &index) const;

public:
	int loadCourse(QString fileName);//CED file
	QString getTaskText(QModelIndex index);
	QString getTaskCheck(QModelIndex index);

	QString courceDescr()
	{
		QDomElement titleEl = root.firstChildElement("DESC");
		if (titleEl.isNull()) {
			return "";
		}

		return titleEl.text();
	}

	QModelIndexList indexes()
	{
		return persistentIndexList();
	}

	void setIspEnvs(QModelIndex index, QString isp, QStringList Envs);
	void setIsps(QModelIndex index, QStringList isp);
	void setUserText(QModelIndex index, const QString &text)
	{
		QDomNode el = nodeById(index.internalId(), root);

		QDomElement userTextEl = el.firstChildElement("USER_PRG");
		if (userTextEl.isNull()) { //USER PRG пока нет - создаем
			userTextEl = courceXml.createElement("USER_PRG");
			el.appendChild(userTextEl);
		}
		//заполняем
		QDomAttr userPrg = courceXml.createAttribute("prg");
		userPrg.setValue(text);
		userTextEl.setAttributeNode(userPrg);
		qDebug() << "SET USER PRG" << index.internalId() << " test " << text;
	}
	void setUserText(int id, const QString &text)
	{
		QDomNode el = nodeById(id, root);

		QDomElement userTextEl = el.firstChildElement("USER_PRG");
		if (userTextEl.isNull()) { //USER PRG пока нет - создаем
			userTextEl = courceXml.createElement("USER_PRG");
			el.appendChild(userTextEl);
		}
		//заполняем
		QDomAttr userPrg = courceXml.createAttribute("prg");
		userPrg.setValue(text);
		userTextEl.setAttributeNode(userPrg);
		qDebug() << "SET USER PRG" << id << " " << text;
	}
	void setUserTestedText(int id, const QString &text)
	{
		QDomNode el = nodeById(id, root);

		QDomElement userTextEl = el.firstChildElement("TESTED_PRG");
		if (userTextEl.isNull()) { //USER PRG пока нет - создаем
			userTextEl = courceXml.createElement("TESTED_PRG");
			el.appendChild(userTextEl);
		}
		//заполняем
		QDomAttr userPrg = courceXml.createAttribute("prg");
		userPrg.setValue(text);
		userTextEl.setAttributeNode(userPrg);
		qDebug() << "SET TEST PRG" << id;
	}

	QString getUserText(int curTaskId)
	{
		QDomNode  node = nodeById(curTaskId, root);
		QDomElement userTextEl = node.firstChildElement("USER_PRG");
		if (userTextEl.isNull()) {
			qDebug() << "Null user Prg" << curTaskId;
			return "";
		};
		QString userPrg = userTextEl.attribute("prg");
		//  qDebug()<<"User program>>>>>>>:"<<userPrg;
		return userPrg;
	}

	QString getUserTestedText(int curTaskId)
	{
		QDomNode  node = nodeById(curTaskId, root);
		QDomElement userTextEl = node.firstChildElement("TESTED_PRG");
		if (userTextEl.isNull()) {
			qDebug() << "Null user  tested Prg" << curTaskId;
			return "";
		};
		QString userPrg = userTextEl.attribute("prg");
		//  qDebug()<<"User Tested program program>>>>>>>:"<<userPrg;
		return userPrg;
	}

	QString getTitle(int curTaskId)
	{
		QDomNode  node = nodeById(curTaskId, root);

		return node.toElement().attribute("name", "");
	}

	void setTitle(int curTaskId, QString title)
	{
		QDomNode el = nodeById(curTaskId, root);

		el.toElement().setAttribute("name", title);

	}

	void setTag(int curTaskId, QString data, QString tag)
	{
		QDomNode  node = nodeById(curTaskId, root);
		if (node.isNull()) {
			qDebug() << "Set NODE NO NODE";
			return;
		};
		QDomElement readyEl = node.firstChildElement(tag);
		if (readyEl.isNull()) {
			qDebug() << "Create NEW" << tag;
			QDomElement markEl = courceXml.createElement(tag);
			node.appendChild(markEl);
			readyEl = node.firstChildElement(tag);

		};
		QDomText text = courceXml.createTextNode(data);
		for (int i = 0; i < readyEl.childNodes().count(); i++) {
			qDebug() << "SWAP" << tag;
			if (readyEl.childNodes().at(i).isText()) {
				QDomNode elText = readyEl.childNodes().at(i);
				readyEl.replaceChild(text, elText);
				break;
			}
		}
		if (readyEl.childNodes().count() == 0) {
			readyEl.appendChild(text);
		}

	}

	void setDesc(int curTaskId, QString desc)
	{
		setTag(curTaskId, desc, "DESC");

	}

	void setProgram(int curTaskId, QString prg)
	{
		setTag(curTaskId, prg, "PROGRAM");

	}

	QModelIndex getIndexById(int id)
	{
		QDomNode  node = nodeById(id, root);
		if (node.isNull()) {
			return index(0, 0, QModelIndex());
		}
		int col = node.columnNumber();
		int row = node.lineNumber();
		if (id != 0) {
			return index(row, col, getIndexById(idByNode(node.parentNode())));
		} else {
			return index(row, col, QModelIndex());
		}
	}

	QString csName(int index);
	QString progFile(int index);
	QStringList Modules(int index);
	QString Script(int index, QString isp);
	QStringList Fields(int index, QString isp);

	int taskMark(int id) const
	{
		return taskMark(nodeById(id, root));

	}

	int taskMark(QDomNode node) const
	{
		if (node.isNull()) {
			return 0;
		}
		QDomElement readyEl = node.firstChildElement("MARK");
		if (readyEl.isNull()) {
			return 0;
		}
		return readyEl.text().toInt();

	}

	void setMark(int id, int mark)
	{
		QDomNode  node = nodeById(id, root);
		if (node.isNull()) {
			return;
		}

		QDomElement readyEl = node.firstChildElement("MARK");
		if (readyEl.isNull()) {
			QDomElement markEl = courceXml.createElement("MARK");
			node.appendChild(markEl);
			readyEl = node.firstChildElement("MARK");

		}

		QDomText text = courceXml.createTextNode(QString::number(mark));
		for (int i = 0; i < readyEl.childNodes().count(); i++) {
			if (readyEl.childNodes().at(i).isText()) {
				QDomNode elText = readyEl.childNodes().at(i);
				readyEl.replaceChild(text, elText);
				break;
			}
		}

		//qDebug()<<"Task "<<id<<" mark"<<taskMark(id);
	}

	void removeModule(int id, QString modName);
	void addModule(QModelIndex index, QString modName);
	QDomElement ispNodeByName(QModelIndex task_index, QString ispName);
	void removeEvn(QDomElement ispElement, QString fieldStr);
	void addEnv(QDomElement ispElement, QString fieldStr);
	QStringList getScripts(int id);

	bool isTask(int id) const
	{
		QDomNode task = nodeById(id, root);
		if (task.toElement().attribute("root") == "true") {
			qDebug() << "Is Node!";
			return false;
		}
		qDebug() << "Is task!";
		return true;
	}

	void setTask(int id, bool flag)
	{
		qDebug() << "setTask!" << flag;
		QDomNode task = nodeById(id, root);
		if (flag) {
			task.toElement().setAttribute("root", "true");
		} else {
			task.toElement().setAttribute("root", "false");
		}
		isTask(id);

	}

	int getMaxId()
	{
		int max = 0;
		QDomNodeList list = courceXml.elementsByTagName("T");
		for (int i = 0; i < list.count(); i++) {

			int depId = list.at(i).toElement().attribute("id", "").toInt();
			if (depId > max) {
				max = depId;
			}
		}
		return max + 10;
	}

	int setChildsId(QDomNode par, int first_id)
	{
		QDomNodeList Childs = par.childNodes();
		int cur_off = 0;
		for (int i = 0; i < Childs.count(); i++) {
			cur_off++;
			if (Childs.at(i).nodeName() == "T") {
				Childs.at(i).toElement().setAttribute("id", first_id + cur_off);
				QDomNodeList dep_list = Childs.at(i).toElement().elementsByTagName("DEPEND");
				qDebug() << "Dep count" << dep_list.count();
				Childs.at(i).toElement().removeChild(Childs.at(i).firstChildElement("DEPEND"));
				for (int j = 0; j < dep_list.count(); j++) {

					qDebug() << "dep remove";
				};

				cur_off = cur_off + setChildsId(Childs.at(i), cur_off + 1);
			}
		}
		return cur_off;
	}

	void addSiblingTask(int id)
	{
		QDomNode task = nodeById(id, root);
		QDomNode copy = task.cloneNode();
		int copyid = getMaxId();
		copy.toElement().setAttribute("id", copyid);
		setChildsId(copy, copyid + 1);

		task.parentNode().toElement().insertAfter(copy, task);

		setMark(copyid, 0);
		cash.clear();
		// buildCash();
	}


	int addNewTask(int par_id, int sibl_id) //Добавляем раздел перед sibl
	{
		QDomNode par = nodeById(par_id, root);
		QDomDocument baseNode;
		baseNode.setContent(QString::fromUtf8("<T root=\"true\" id=\"2\" name=\"Новый раздел\">\n<DESC>Нет Описания</DESC>\n<CS>Кумир</CS>\n<MARK/>\n</T>\n"));
		QDomElement newNode = baseNode.firstChildElement();
		QDomNode copy = newNode.cloneNode(true);
		QDomNode impCopy = courceXml.importNode(copy, true);
		int copyid = getMaxId();

		impCopy.toElement().setAttribute("id", copyid);


		par.toElement().insertBefore(impCopy, nodeById(sibl_id, par));
		//setMark(copyid,0);

		cash.clear();
		buildCash();
		emit dataChanged(QModelIndex(), createIndex(rowCount() + 1, 1, copyid));
		return copyid;
	}

	void moveTask(int new_par_id, int node_id)
	{
		cash.clear();
		buildCash();
		qDebug() << "new_par_id" << new_par_id;
		QDomNode task = nodeById(node_id, root);
		QDomNode newPar = nodeById(new_par_id, root);
		QDomNode clone = task.cloneNode(true);
		newPar.insertBefore(clone, newPar.firstChild());
		task.parentNode().removeChild(task);
		cash.clear();
		buildCash();

		emit dataChanged(QModelIndex(), createIndex(rowCount() + 1, 1, node_id));
	}

	void addDeepTask(int id)
	{
		if (id == 0) {
			QDomDocument baseNode;
			baseNode.setContent(QString::fromUtf8("<T xml:id=\"2\" name=\"Новое задание\">\n<DESC>Нет Описания</DESC>\n<CS>Кумир</CS>\n <READY>false</READY>\n</T>\n"));
			QDomElement newNode = baseNode.firstChildElement();
			QDomNode copy = newNode.cloneNode(true);
			QDomNode impCopy = courceXml.importNode(copy, true);
			int copyid = getMaxId();

			impCopy.toElement().setAttribute("xml:id", copyid);


			root.toElement().insertAfter(impCopy, root.lastChild());
			setMark(copyid, 0);

			cash.clear();
			buildCash();
			emit dataChanged(QModelIndex(), createIndex(rowCount() + 1, 1, copyid));
			return;
		}

		QDomNode task = nodeById(id, root);
		QDomNode copy = task.cloneNode(false);
		QDomNodeList taskChilds = task.childNodes();
		int copyid = getMaxId();

		copy.toElement().setAttribute("id", copyid);

		for (int i = 0; i < taskChilds.count(); i++) {
			QDomNode child = task.childNodes().item(i);
			QDomNode chCopy = child.cloneNode();
			if (chCopy.nodeName() != "T") {
				copy.toElement().appendChild(chCopy);
			}
			// qDebug()<<"Node app"<<chCopy.nodeName();
		}
		task.toElement().insertBefore(copy, task.firstChild());
		setMark(copyid, 0);
		cash.clear();
		buildCash();
	}

	void removeNode(int id)
	{
		QDomNode task = nodeById(id, root);
		task.parentNode().removeChild(task);
		cash.clear();
		buildCash();
	}

	bool  taskAvailable(int id) const
	{
		return taskAvailable(nodeById(id, root));
	}

	bool taskAvailable(QDomNode task) const
	{

		if (task.isNull()) {
			return false;
		}
		QDomElement depEl = task.firstChildElement("DEPEND");
		while (!depEl.isNull()) {
			QDomElement idEl = depEl.firstChildElement("ID");
			QDomElement markEl = depEl.firstChildElement("MIN");
			if (idEl.isNull() || markEl.isNull()) {
				depEl = depEl.nextSiblingElement("DEPEND");
				continue;
			}
			int depId = idEl.text().toInt();
			QDomNode depNode = nodeById(depId, root); //Узел от которого зависим

			int needMark = markEl.text().toInt();
			//qDebug()<<"Need mark"<<needMark<<"Task Mark"<<taskMark(depId);
			if (taskMark(depNode) < needMark) {
				// qDebug()<<"task id:"<<id<<" unavailable";
				return false;
			}
			depEl = depEl.nextSiblingElement("DEPEND");
		}
		return true;
	}

	QDomDocument *document()
	{
		return &courceXml;
	}

	void setTeacher(bool flag)
	{
		isTeacher = flag;
	}

	bool hasUpSib(QModelIndex &index)
	{
		QDomNode el = nodeById(index.internalId(), root);
		return !el.previousSiblingElement("T").isNull();
	}

	bool hasDownSib(QModelIndex &index)
	{
		QDomNode el = nodeById(index.internalId(), root);
		return !el.nextSiblingElement("T").isNull();
	}

	QModelIndex moveUp(QModelIndex &index)
	{
		if (!hasUpSib(index)) {
			return index;
		}
		QDomNode el = nodeById(index.internalId(), root);
		QDomNode per = el.previousSiblingElement("T");
		el.parentNode().toElement().insertBefore(el, per);
		cash.clear();
		buildCash();
		return createMyIndex(index.row() - 1, index.column(), index.parent());
	}

	QModelIndex moveDown(QModelIndex &index)
	{
		if (!hasDownSib(index)) {
			return index;
		}
		QDomNode el = nodeById(index.internalId(), root);
		QDomNode per = el.nextSiblingElement("T");
		el.parentNode().toElement().insertAfter(el, per);
		cash.clear();
		buildCash();
		return createMyIndex(index.row() + 1, index.column(), index.parent());
	}

	QString rootText()
	{
		return root.attribute("name");
	}

	void setRootText(QString text)
	{
		root.setAttribute("name", text);
	}

	void buildCash()
	{
		QDomNodeList list = courceXml.elementsByTagName("T");
		for (int i = 0; i < list.count(); i++) {
			QString id = list.at(i).toElement().attribute("id");

			if (id == "") {
				id = list.at(i).toElement().attribute("xml:id");    //Другой вариант написания.
			}

			qDebug() << "Cash" << id;
			cash.insert(id.toInt(), list.at(i));
		}
	}

	QList<KumTask> childTasks(QModelIndex parent)
	{
		QList <KumTask> toRet;
		QDomNode el = nodeById(parent.internalId(), root);
		QDomNodeList childs = el.childNodes();
		for (int i = 0; i < childs.count(); i++) {
			if (childs.at(i).nodeName() == "T") {
				QString id = childs.at(i).toElement().attribute("id");

				if (id == "") {
					id = childs.at(i).toElement().attribute("xml:id");
				}
				toRet.append(KumTask(id.toInt(), childs.at(i).toElement().attribute("name", "")));
			}
		}
		return toRet;
	}

private:
	QIcon iconByMark(int mark, bool isFolder)const;
	QDomNode nodeByRowColumn(int row, int column, QDomNode *parent) const;
	QDomNode nodeById(int id, QDomNode parent) const;
	QModelIndex createMyIndex(int row, int column, QModelIndex parent) const;

	int idByNode(QDomNode node) const
	{
		bool ok;
		int id = node.toElement().attribute("id", "").toInt(&ok);
		if (!ok) {
			return -1;
		}
		return id;
	}

	int subTasks(QDomNode parent) const
	{
		QDomNodeList childs = parent.childNodes();
		int count = 0;
		for (int i = 0; i < childs.count(); i++) {
			if (childs.at(i).nodeName() == "T") {
				count++;

			}
		}
		// qDebug()<<"Childs"<<count;
		return count;
	}

	int domRow(QDomNode &child) const;
	QString courseFileName;
	int taskCount;
	QString courseName;
	QString courseDesc;
	QFont itemFont;
	QDomDocument courceXml;
	QDomElement root;
	QList<QIcon> markIcons;
	bool isTeacher;
	QHash<int, QDomNode> cash;
};

#endif // COURSE_MODEL_H
