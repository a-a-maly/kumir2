#include "suggestionswindow.h"
#include "editor.h"
#include "editorplane.h"
#include "textcursor.h"
#include "textdocument.h"
#include "clipboard.h"
#include "utils.h"
#include "macro.h"
#include "settingspage.h"
#include "editcommands.h"
#include <kumir2-libs/widgets/cyrillicmenu.h>
#include <kumir2/actorinterface.h>
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include "findreplace.h"
#include "macroeditor.h"
#include "macrolisteditor.h"
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2-libs/widgets/iconprovider.h>
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2/runinterface.h>
#include <kumir2/editor_instanceinterface.h>

#include <deque>

#include <QScrollBar>
#include <QLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTextCodec>
#include <QApplication>


namespace Editor
{

using namespace Shared;

QSize EditorInstance::minimumSizeHint() const
{
	int minW = 100;
	int minH = 0;
	if (horizontalScrollBar_->isVisible()) {
		minH = horizontalScrollBar_->height();
	}
	QFontMetrics fm(plane_->font());
	minH += fm.lineSpacing() + fm.height(); // at least 1 line of text
	if (verticalScrollBar_->isVisible()) {
		minH = qMax(minH, verticalScrollBar_->minimumSizeHint().height());
	}
	return QSize(minW, minH);
}


void EditorInstance::lock()
{
	cursor_->setEnabled(false);
	cut_->setEnabled(false);
	paste_->setEnabled(false);
	deleteLine_->setEnabled(false);
	deleteTail_->setEnabled(false);
	if (toggleComment_) {
		toggleComment_->setEnabled(false);
	}
	if (toggleBreakpoint_) {
		toggleBreakpoint_->setEnabled(false);
	}
	for (int i = 0; i < userMacros_.size(); i++) {
		QSharedPointer<Macro> macro = userMacros_[i];
		QAction *action = macro->action;
		action->setEnabled(false);
	}
	for (int i = 0; i < systemMacros_.size(); i++) {
		QSharedPointer<Macro> macro = systemMacros_[i];
		QAction *action = macro->action;
		action->setEnabled(false);
	}
}

void EditorInstance::unlock()
{
	cursor_->setEnabled(true);
	paste_->setEnabled(Clipboard::instance()->hasContent());
	cut_->setEnabled(true);
	deleteLine_->setEnabled(true);
	deleteTail_->setEnabled(true);
	if (toggleComment_) {
		toggleComment_->setEnabled(true);
	}
	if (toggleBreakpoint_) {
		toggleBreakpoint_->setEnabled(true);
	}
	for (int i = 0; i < userMacros_.size(); i++) {
		QSharedPointer<Macro> macro = userMacros_[i];
		QAction *action = macro->action;
		action->setEnabled(true);
	}
	for (int i = 0; i < systemMacros_.size(); i++) {
		QSharedPointer<Macro> macro = systemMacros_[i];
		QAction *action = macro->action;
		action->setEnabled(true);
	}
}

void EditorInstance::appendMarginText(int lineNo, const QString &text)
{
	if (lineNo >= 0 && lineNo < (int)doc_->linesCount()) {
		TextLine::Margin &margin = doc_->marginAt(lineNo);
		if (!margin.text.isEmpty()) {
			margin.text += "; ";
		}
		margin.text += text;
	}
	update();
}

void EditorInstance::loadDocument(QIODevice *device, const QString &fileNameSuffix,
	const QString &sourceEncoding, const QUrl &sourceUrl, QString *error)
{
	const QByteArray bytes = device->readAll();

	Shared::Analizer::SourceFileInterface::Data data;

	if (analizerPlugin_) {
		data = analizerPlugin_->sourceFileHandler()->fromBytes(bytes, sourceEncoding);
	} else {
		QTextCodec *codec = QTextCodec::codecForName(sourceEncoding.toLatin1());
		if (!codec) {
			codec = QTextCodec::codecForLocale();
		}
		QTextStream ts(bytes);
		ts.setCodec(codec);
		ts.setAutoDetectUnicode(true);
		data.hasHiddenText = false;
		data.visibleText = ts.readAll();
	}
	data.canonicalSourceLanguageName = fileNameSuffix;
	data.sourceUrl = sourceUrl;
	loadDocument(data, error);
}

void EditorInstance::loadDocument(const QString &fileName, QString *error)
{
	QFile f(fileName);
	if (f.open(QIODevice::ReadOnly)) {
		const QString localPath = QFileInfo(f).absoluteFilePath();
		const QString suffix = QFileInfo(f).suffix();
		const QUrl url = QUrl::fromLocalFile(localPath);
		loadDocument(&f, suffix, QString(), url, error);
		f.close();
	} else {
		if (error) {
			*error =
				tr("Can't open file %1 for reading").arg(fileName);
		}
	}
}

void EditorInstance::loadDocument(
	const Shared::Analizer::SourceFileInterface::Data &data,
	QString *error
)
{
	QList<Shared::AnalizerInterface *> analizers =
		ExtensionSystem::PluginManager::instance()
		->findPlugins<Shared::AnalizerInterface>();

	for (int i = 0; i < analizers.size(); i++) {
		if (analizers[i]->defaultDocumentFileNameSuffix() == data.canonicalSourceLanguageName) {
			analizerPlugin_ = analizers[i];
			delete analizerInstance_;
			analizerInstance_ = 0;
			analizerInstance_ = analizerPlugin_->createInstance();
			if (data.sourceUrl.isLocalFile()) {
				QString localPath = data.sourceUrl.toLocalFile();
				QString dirName = QFileInfo(localPath).absoluteDir().path();
				analizerInstance_->setSourceDirName(dirName);
			}
			break;
		}
	}

	if (plane_) {
		plane_->updateAnalizer();
	}

	if (analizerInstance_) {
		analizerInstance_->connectUpdateRequest(this, SLOT(updateFromAnalizer()));
	}
	documentUrl_ = data.sourceUrl;
	setKumFile(data);
}

void EditorInstance::setMarginText(int lineNo, const QString &text, const QColor &fgColor)
{
	if (lineNo >= 0 && lineNo < (int)doc_->linesCount()) {
		TextLine::Margin &margin = doc_->marginAt(lineNo);
		margin.text = text;
		margin.color = fgColor;
	}
	update();
}

void EditorInstance::clearMarginText()
{
	for (uint i = 0; i < doc_->linesCount(); i++) {
		TextLine::Margin &margin = doc_->marginAt(i);
		margin.text.clear();
	}
	update();
}

void EditorInstance::clearMarginText(int fromLine, int toLine)
{
	int start = qMin(qMax(0, fromLine), int(doc_->linesCount()) - 1);
	int end = qMin(qMax(0, toLine), int(doc_->linesCount()) - 1);
	if (toLine == -1) {
		end = doc_->linesCount() - 1;
	}
	for (int i = start; i <= end; i++) {
		TextLine::Margin &margin = doc_->marginAt(i);
		margin.text.clear();
	}
	update();
}

void EditorInstance::setLineHighlighted(int lineNo, const QColor &color, quint32 colStart, quint32 colEnd)
{
	plane_->setLineHighlighted(lineNo, color, colStart, colEnd);
}


void EditorInstance::disableInsertActions()
{
	foreach (QSharedPointer<Macro> m, userMacros_) {
		m->action->setEnabled(false);
	}
	foreach (QSharedPointer<Macro> m, systemMacros_) {
		m->action->setEnabled(false);
	}
}

void EditorInstance::enableInsertActions()
{
	foreach (QSharedPointer<Macro> m, userMacros_) {
		m->action->setEnabled(true);
	}
	foreach (QSharedPointer<Macro> m, systemMacros_) {
		m->action->setEnabled(true);
	}
}

void EditorInstance::timerEvent(QTimerEvent *e)
{
	if (e->timerId() == timerId_) {
		e->accept();
		bool isRussianLayout = Utils::isRussianLayout();
		bool isCapsLock = Utils::isCapsLock();
		bool isShiftPressed = Utils::shiftKeyPressed;
		bool isAltPressed = Utils::altKeyPressed;
		emit keyboardLayoutChanged(
			isRussianLayout ? QLocale::Russian : QLocale::English,
			isCapsLock,
			isShiftPressed,
			isAltPressed
		);
		const bool hasSelection = cursor_->hasSelection() || cursor_->hasRectSelection();
		cut_->setEnabled(hasSelection);
		copy_->setEnabled(hasSelection);
	} else if (e->timerId() == autoScrollTimerId_) {
		e->accept();
		if (autoScrollStateY_ == -1) {
			if (verticalScrollBar_->value() > 0) {
				verticalScrollBar_->setValue(verticalScrollBar_->value() - verticalScrollBar_->singleStep());
			}
		} else if (autoScrollStateY_ == 1) {
			if (verticalScrollBar_->value() < verticalScrollBar_->maximum()) {
				verticalScrollBar_->setValue(verticalScrollBar_->value() + verticalScrollBar_->singleStep());
			}
		}
		if (autoScrollStateX_ == -1) {
			if (horizontalScrollBar_->value() > 0) {
				horizontalScrollBar_->setValue(horizontalScrollBar_->value() - horizontalScrollBar_->singleStep());
			}
		} else if (autoScrollStateX_ == 1) {
			if (horizontalScrollBar_->value() < horizontalScrollBar_->maximum()) {
				horizontalScrollBar_->setValue(horizontalScrollBar_->value() + horizontalScrollBar_->singleStep());
			}
		}
	} else if (e->timerId() == clipboardCheckTimerId_) {
		e->accept();
		bool isLocked = ! cursor_->isEnabled();
		if (!isLocked) {
			bool hasContents = Clipboard::instance()->hasContent();
			paste_->setEnabled(hasContents);
		}
	}
}

void EditorInstance::handleAutoScrollChange(char a)
{
	autoScrollStateY_ = a;
}

void EditorInstance::handleAutoScrollChangeX(char a)
{
	autoScrollStateX_ = a;
}

void EditorInstance::updatePosition(int row, int col)
{
	emit cursorPositionChanged(row, col);
}

void EditorInstance::loadMacros()
{
	if (analizerPlugin_ == nullptr) {
		return;
	}
	using namespace Shared;
	using namespace ExtensionSystem;
	QString analizerName = analizerPlugin_->defaultDocumentFileNameSuffix();

	// System macros
	QString systemMacrosPath = plugin_->myResourcesDir().absoluteFilePath("macros-" + analizerName + ".xml");
	systemMacros_ = loadFromFile(systemMacrosPath);

	// Actor-specific macros

	const QList<const KPlugin *> actorPlugins =
		PluginManager::instance()->loadedConstPlugins("Actor*");

	foreach (const KPlugin *plugin, actorPlugins) {
		ActorInterface *actor = qobject_cast<ActorInterface *>(plugin);
		QString canonicalName = actorCanonicalName<QString>(actor->asciiModuleName());
		canonicalName.remove(" ");
		const QString actorMacrosFileName = plugin_->myResourcesDir().absoluteFilePath(
				"macros-" + analizerName + "-" + canonicalName + ".xml"
			);
		if (QFile::exists(actorMacrosFileName)) {
			systemMacros_.push_back(QSharedPointer<Macro>(new Macro()));
			systemMacros_ += loadFromFile(actorMacrosFileName);
		}
	}


	// Import macros
	if (analizerName == "kum") {
		const QList<const KPlugin *> actorPlugins =
			PluginManager::instance()->loadedConstPlugins("Actor*");

		std::deque<QString> availableActorNames;

		foreach (const KPlugin *plugin, actorPlugins) {
			ActorInterface *actor = qobject_cast<ActorInterface *>(plugin);
			if (actor && !actor->localizedModuleName(QLocale::Russian).startsWith("_")) {
				availableActorNames.push_back(Shared::actorCanonicalName(actor->localizedModuleName(QLocale::Russian)));
			}
		}

//        if (!availableActorNames.empty()) {
//            systemMacros_.push_back(Macro());
//            for (size_t i=0; i<qMin(size_t(9), availableActorNames.size()); i++) {
//                const QString & actorName = availableActorNames.at(i);
//                const QString insertText = tr("import %1").arg(actorName);
//                Macro macro;
//                macro.title = insertText;
//                macro.key = QString::number(i+1).at(0);
//                macro.commands.push_back(
//                            KeyCommand(KeyCommand::InsertImport, actorName)
//                            );
//                systemMacros_.push_back(macro);
//            }
//        }
	}


	// User macros
	QString fileName = analizerName.length() > 0
		? QString::fromLatin1(".user-macros-%1.xml").arg(analizerName)
		: QString::fromLatin1(".user-macros.xml");

	const QString dirName = mySettings()->locationDirectory();
	if (dirName.startsWith(QDir::homePath() + "/.")) {
		fileName.remove(0, 1);
	}
	QDir dir(dirName);
	const QString filePath = dir.absoluteFilePath(fileName);
	userMacros_ = loadFromFile(filePath);
}

ExtensionSystem::SettingsPtr EditorInstance::mySettings() const
{
	return plugin_->mySettings();
}

static bool operator<(QSharedPointer<Macro> a, QSharedPointer<Macro> b)
{
	return a->key.unicode() < b->key.unicode();
}

void EditorInstance::updateInsertMenu()
{
	loadMacros();
	insertMenu_->clear();
	const QString escComa = "Esc, ";
	QVector< QList<QSharedPointer<Macro> > > groups(2);
	for (int i = 0; i < systemMacros_.size(); ++i) {
		QSharedPointer<Macro> m = systemMacros_[i];
		int groupNo = m->showInLastBlock ? 1 : 0;
		groups[groupNo].append(m);
	}
	qSort(groups[1]);
	for (int z = 0; z < 2; ++z) {
		QList<QSharedPointer<Macro> > group = groups[z];
		if (1 == z && groups[1].size() > 0) {
			insertMenu_->addSeparator();
		}
		for (int i = 0; i < group.size(); i++) {
			QSharedPointer<Macro> m = group[i];
			if (m->title.isEmpty()) {
				// Separator
				m->action = insertMenu_->addSeparator();
			} else {
				m->action = new QAction(m->title, insertMenu_);
				insertMenu_->addAction(m->action);
				connect(m->action, SIGNAL(triggered()), this, SLOT(playMacro()));
				if (!m->key.isNull()) {
					const QKeySequence ks2(escComa + QString(m->key));
					m->action->setProperty("fakeShortcut", ks2.toString());
				} else if (uint(m->extKey) != 0u) {
					QString repr;
					if (m->extKey == Qt::Key_Left) {
						repr = "Left";
					} else if (m->extKey == Qt::Key_Right) {
						repr = "Right";
					} else if (m->extKey == Qt::Key_Up) {
						repr = "Up";
					} else if (m->extKey == Qt::Key_Down) {
						repr = "Down";
					} else if (m->extKey == Qt::Key_Space) {
						repr = "Space";
					}
					const QKeySequence ks(escComa + repr);
					m->action->setShortcut(ks);
				}
			}
		}
	}
	if (!userMacros_.isEmpty()) {
		insertMenu_->addSeparator();
	}
	for (int i = 0; i < userMacros_.size(); i++) {
		QSharedPointer<Macro> m = userMacros_[i];
		m->action = new QAction(m->title, insertMenu_);
		if (!m->key.isNull()) {
			const QKeySequence ks(escComa + QString(Utils::latinKey(m->key)));
			const QKeySequence ks2(escComa + QString(m->key));
			const QList<QKeySequence> shortcuts = QList<QKeySequence>() << ks << ks2;
			m->action->setShortcuts(shortcuts);
		}
		insertMenu_->addAction(m->action);
		connect(m->action, SIGNAL(triggered()), this, SLOT(playMacro()));
	}
	editMacros_->setEnabled(userMacros_.size() > 0);
	Widgets::CyrillicMenu *insMenu = qobject_cast<Widgets::CyrillicMenu *>(insertMenu_);
	if (insMenu) {
		insMenu->updateUbuntuShortcutTexts();
	}
}

bool EditorInstance::tryEscKeyAction(const QString &text)
{
	if (text.length() != 1 && text.at(0).toLatin1()) {
		return false; // workarund required only for non-latin keys
	}
	const QList<QSharedPointer<Macro> > allMacros = systemMacros_ + userMacros_;
	const QChar ch = text.at(0).toUpper();
	const QChar altCh = Utils::cyrillicKey(ch).toUpper();
	foreach (QSharedPointer<Macro> m, allMacros) {
		bool keyMatch = m->key.toUpper() == ch;
		bool altKeyMatch = m->key.toUpper() == altCh;
		bool enabled = m->action && m->action->isEnabled();
		if ((keyMatch || altKeyMatch) && enabled) {
			m->action->trigger();
			return true;
		}
	}

	return false;
}

void EditorInstance::toggleBreakpoint()
{
	qint32 lineNo = cursor()->row();
	if (0 <= lineNo && lineNo < (int)document()->linesCount()) {
		TextLine &line = document()->at(lineNo);
		line.hasBreakpoint = !line.hasBreakpoint;
		plane_->update();
		if (line.hasBreakpoint) {
			emit breakpointCnagedOrInserted(
				line.breakpoint.enabled,
				lineNo,
				line.breakpoint.ignoreCount,
				line.breakpoint.condition
			);
		} else {
			emit breakpointRemoved(lineNo);
		}
	}
}

void EditorInstance::playMacro()
{
	QAction *a = qobject_cast<QAction *>(sender());
	Q_CHECK_PTR(a);
	QSharedPointer<Macro> m;
	bool found = false;
	foreach (QSharedPointer<Macro> mm, systemMacros_) {
		if (mm->action == a) {
			found = true;
			m = mm;
			break;
		}
	}
	if (!found) {
		foreach (QSharedPointer<Macro> mm, userMacros_) {
			if (mm->action == a) {
				found = true;
				m = mm;
				break;
			}
		}
	}
	if (found) {
		document()->undoStack()->beginMacro(m->title);
		for (int i = 0; i < m->commands.size(); i++) {
			cursor_->evaluateCommand(m->commands[i]);
		}
		document()->undoStack()->endMacro();
		plane_->updateScrollBars();
		plane_->ensureCursorVisible();
	}
}


void EditorInstance::focusInEvent(QFocusEvent *e)
{
	QWidget::focusInEvent(e);
	plane_->setFocus();
}



void EditorInstance::handleCompleteCompilationRequiest(
	const QStringList &visibleText,
	const QStringList &hiddenText,
	int hiddenBaseLine
)
{
	if (!analizerPlugin_) {
		return;
	}
	QString vt;
	for (int i = 0; i < visibleText.size(); i++) {
		vt += visibleText[i];
		if (i < visibleText.size() - 1) {
			vt += "\n";
		}
	}
	if (hiddenText.size() > 0) {
		vt += "\n";
	}
	for (int i = 0; i < hiddenText.size(); i++) {
		vt += hiddenText[i];
		if (i < hiddenText.size() - 1) {
			vt += "\n";
		}
	}
	analizerInstance_->setSourceText(vt);
	updateFromAnalizer();
}



void EditorInstance::updateFromAnalizer()
{
	QList<Shared::Analizer::LineProp> props = analizerInstance_->lineProperties();
	QList<QPoint> ranks = analizerInstance_->lineRanks();
	QList<Shared::Analizer::Error> errors = analizerInstance_->errors();
	std::vector<int> oldIndents(doc_->linesCount(), 0);
	for (int i = 0; i < (int)doc_->linesCount(); i++) {
		oldIndents[i] = doc_->indentAt(i);
	}
	for (unsigned int i = 0; i < doc_->linesCount(); i++) {
		int oldIndent = oldIndents[i];
		if (i < (unsigned int) ranks.size()) {
			doc_->setIndentRankAt(i, ranks[i]);
		}
		if (i < (unsigned int) props.size()) {
			doc_->setHighlightAt(i, props[i].toList());
		}
		doc_->at(i).multipleStatementsInLine = analizerInstance_->multipleStatementsInLine(i);
		doc_->marginAt(i).errors.clear();
		if (Shared::AnalizerInterface::HardIndents == analizerPlugin_->indentsBehaviour()) {
			int newIndent = doc_->indentAt(i);
			int diffIndent = newIndent - oldIndent;
			if (cursor_->row() == i) {
				int newCursorColumn = 0;
				if (doc_->at(i).text.isEmpty()) {
					newCursorColumn = 2 * newIndent;
				} else {
					int oldCursorColumn = cursor_->column();
					newCursorColumn = oldCursorColumn + 2 * diffIndent;
				}
				newCursorColumn = qMax(newCursorColumn, 0);
				cursor_->setColumn(newCursorColumn);
			}
		}
	}
	for (int i = 0; i < errors.size(); i++) {
		Shared::Analizer::Error err = errors[i];
		int lineNo = err.line;
		if (lineNo >= 0) {
			doc_->marginAt(lineNo).errors.append(err.message);
		}
	}
	plane_->update();
}

EditorInstance::EditorInstance(
	EditorPlugin *plugin,
	bool initiallyNotSaved,
	AnalizerInterface *analizerPlugin,
	Analizer::InstanceInterface *analizerInstance)
	: QWidget()
	, plugin_(plugin)
	, analizerPlugin_(analizerPlugin)
	, analizerInstance_(analizerInstance)
	, doc_(new TextDocument(this))
	, cursor_(new TextCursor(this))
	, plane_(new EditorPlane(this))
	, findReplace_(new FindReplace(plugin->myResourcesDir(), this))
	, autocompleteWidget_(new SuggestionsWindow(this))
	, autoScrollStateX_(0)
	, autoScrollStateY_(0)
	, notSaved_(initiallyNotSaved)
{
	setupUi();
	setupStyleSheets();
	createActions();
	updateInsertMenu();

	plane_->addContextMenuAction(cut_);
	plane_->addContextMenuAction(copy_);
	plane_->addContextMenuAction(paste_);

	if (plugin_->teacherMode_) {
		plane_->addContextMenuAction(toggleLock_);
	}

	cursor_->setTeacherMode(plugin_->teacherMode_);
	toggleLock_->setVisible(plugin_->teacherMode_);
	toggleLock_->setEnabled(plugin_->teacherMode_);

	createConnections();

	timerId_ = startTimer(50);
	autoScrollTimerId_ = startTimer(100);
	clipboardCheckTimerId_ = startTimer(250);

	updateSettings(QStringList());
	updatePosition(cursor_->row(), cursor_->column());

	plane_->updateScrollBars();

	if (analizerInstance_) {
		analizerInstance_->connectUpdateRequest(this, SLOT(updateFromAnalizer()));
	}
}

EditorInstance::~EditorInstance()
{
	delete doc_;
	doc_ = 0;
	delete analizerInstance_;
	analizerInstance_ = 0;

	if (plane_) {
		plane_->deleteLater();
	}
	killTimer(timerId_);
}


void EditorInstance::setupUi()
{
	horizontalScrollBar_ = new QScrollBar(Qt::Horizontal, this);
	verticalScrollBar_ = new QScrollBar(Qt::Vertical, this);
	QGridLayout *l = new QGridLayout();
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	setLayout(l);
	l->addWidget(plane_, 0, 0);
	l->addWidget(verticalScrollBar_, 0, 1);
	l->addWidget(horizontalScrollBar_, 1, 0);
	autocompleteWidget_->setVisible(false);
}

void EditorInstance::setupStyleSheets()
{
//    static const char * ScrollBarCSS = ""
//            "QScrollBar {"
//            "   width: 12px;"
//            "   background-color: transparent;"
//            "   padding-right: 4px;"
//            "   border: 0;"
//            "}"
//            "QScrollBar:handle {"
//            "   background-color: gray;"
//            "   border-radius: 4px;"
//            "}"
//            "QScrollBar:add-line {"
//            "   height: 0;"
//            "}"
//            "QScrollBar:sub-line {"
//            "   height: 0;"
//            "}"
//            ;
//    static const char * HorizontalScrollBarCSS = ""
//            "QScrollBar {"
//            "   height: 12px;"
//            "   background-color: transparent;"
//            "   padding-right: 4px;"
//            "   border: 0;"
//            "   border-right: 4px solid #FF8080;"
//            "}"
//            "QScrollBar:handle {"
//            "   background-color: gray;"
//            "   border-radius: 4px;"
//            "}"
//            "QScrollBar:add-line {"
//            "   height: 0;"
//            "}"
//            "QScrollBar:sub-line {"
//            "   height: 0;"
//            "}"
//            ;
//    verticalScrollBar_->setStyleSheet(ScrollBarCSS);
//    horizontalScrollBar_->setStyleSheet(HorizontalScrollBarCSS);
}

void EditorInstance::createConnections()
{
	installEventFilter(this);
	plane_->installEventFilter(this);
	verticalScrollBar_->installEventFilter(this);
	qApp->installEventFilter(plane_);

	connect(plane_, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
	connect(cursor_, SIGNAL(positionChanged(int, int)),
		this, SLOT(updatePosition(int, int)));

	connect(plane_, SIGNAL(requestAutoScroll(char)),
		this, SLOT(handleAutoScrollChange(char)));

	connect(plane_, SIGNAL(requestAutoScrollX(char)),
		this, SLOT(handleAutoScrollChangeX(char)));

	connect(doc_, SIGNAL(completeCompilationRequest(QStringList, QStringList, int)),
		this, SLOT(handleCompleteCompilationRequiest(QStringList, QStringList, int)), Qt::DirectConnection);

	connect(doc_->undoStack(), SIGNAL(cleanChanged(bool)), this, SIGNAL(documentCleanChanged(bool)));

	connect(plane_, SIGNAL(urlsDragAndDropped(QList<QUrl>)), this, SIGNAL(urlsDragAndDropped(QList<QUrl>)));

	connect(doc_->undoStack(), SIGNAL(canRedoChanged(bool)), cursor_, SLOT(handleRedoChanged(bool)));
	connect(doc_->undoStack(), SIGNAL(canUndoChanged(bool)), cursor_, SLOT(handleUndoChanged(bool)));


	connect(plane_, SIGNAL(enableInsertActions()), this, SLOT(enableInsertActions()));
	connect(plane_, SIGNAL(disableInsertActions()), this, SLOT(disableInsertActions()));

	connect(horizontalScrollBar_, SIGNAL(valueChanged(int)), plane_, SLOT(update()));
	connect(verticalScrollBar_, SIGNAL(valueChanged(int)), plane_, SLOT(update()));

	connect(cursor_, SIGNAL(updateRequest()), plane_, SLOT(updateCursor()));
	connect(cursor_, SIGNAL(updateRequest(int, int)), plane_, SLOT(updateText(int, int)));

	connect(autocompleteWidget_.data(), SIGNAL(requestHelpForAlgorithm(QString, QString)),
		this, SIGNAL(requestHelpForAlgorithm(QString, QString)));
	connect(autocompleteWidget_.data(), SIGNAL(hidden()), plane_, SIGNAL(enableInsertActions()));
	connect(autocompleteWidget_.data(), SIGNAL(acceptedSuggestion(QString)),
		plane_, SLOT(finishAutoCompletion(QString)));

	connect(plane_, SIGNAL(breakpointCnagedOrInserted(bool, quint32, quint32, QString)),
		this, SIGNAL(breakpointCnagedOrInserted(bool, quint32, quint32, QString)));
	connect(plane_, SIGNAL(breakpointRemoved(quint32)),
		this, SIGNAL(breakpointRemoved(quint32)));

}

QScrollBar *EditorInstance::scrollBar(Qt::Orientation orientation)
{
	if (orientation == Qt::Horizontal) {
		return horizontalScrollBar_;
	} else {
		return verticalScrollBar_;
	}
}

QFont EditorInstance::editorFont() const
{
	return plane_->font();
}

Shared::Analizer::ApiHelpItem EditorInstance::contextHelpItem() const
{
	Shared::Analizer::ApiHelpItem result;

	if (analizerInstance_ && analizerInstance_->helper()) {
		int row = cursor()->row();
		int col = cursor()->column();
		const QString &text = document()->textAt(row);
		if (Shared::AnalizerInterface::HardIndents == analizerPlugin_->indentsBehaviour()) {
			col -= document()->indentAt(row) * 2;
		}
		result = analizerInstance_->helper()->itemUnderCursor(text, row, col, true);
	}

	return result;
}

QAction *EditorInstance::toggleBreakpointAction() const
{
	if (!toggleBreakpoint_ && hasBreakpointSupport()) {
		toggleBreakpoint_ = new QAction(plane_);
		toggleBreakpoint_->setText(tr("Toggle line breakpoint"));
		toggleBreakpoint_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
		toggleBreakpoint_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
		QObject::connect(toggleBreakpoint_, SIGNAL(triggered()),
			this, SLOT(toggleBreakpoint()));
	}
	return toggleBreakpoint_;
}

QList<Shared::Editor::Breakpoint> EditorInstance::breakpoints() const
{
	QList<Shared::Editor::Breakpoint> result;
	for (int i = 0; i < (int)document()->linesCount(); i++) {
		const TextLine &line = document()->at(i);
		if (line.hasBreakpoint) {
			Shared::Editor::Breakpoint bp = line.breakpoint;
			bp.lineNo = i;
			bp.fileName = "";
			result.push_back(bp);
		}
	}
	return result;
}

void EditorInstance::forceCompleteCompilation()
{
	document()->forceCompleteRecompilation(QPoint(0, 0));
}

void EditorInstance::paintEvent(QPaintEvent *e)
{
	QPainter p(this);
	p.setPen(Qt::NoPen);
	p.setBrush(palette().brush(QPalette::Window));
	p.drawRect(0, 0, width(), height());
	p.end();
	QWidget::paintEvent(e);
//    const QBrush br = plane_->hasFocus()
//            ? palette().brush(QPalette::Highlight)
//            : palette().brush(QPalette::Window);
//    p.setPen(QPen(br, 3));
//    p.drawLine(width()-1, 0, width()-1, height()-1);
	e->accept();
}

bool EditorInstance::eventFilter(QObject *obj, QEvent *evt)
{
	if (obj == verticalScrollBar_ && evt->type() == QEvent::Paint) {
		QPainter p(verticalScrollBar_);
		const QBrush br = plane_->hasFocus()
			? palette().brush(QPalette::Highlight)
			: palette().brush(QPalette::Window);
		p.setPen(QPen(br, 3));
		p.drawLine(0, 0,
			verticalScrollBar_->width() - 1, 0);
		p.drawLine(0, verticalScrollBar_->height() - 1,
			verticalScrollBar_->width() - 1, verticalScrollBar_->height() - 1);
		p.end();
	} else if (obj == plane_) {
		if (evt->type() == QEvent::FocusIn || evt->type() == QEvent::FocusOut) {
			verticalScrollBar_->repaint();
		}
	}
	return false;
}

void EditorInstance::createActions()
{
	const QString qtcreatorIconsPath =
		ExtensionSystem::PluginManager::instance()->sharePath()
		+ "/icons/from_qtcreator/";

	selectAll_ = new QAction(plane_);
	selectAll_->setText(QObject::tr("Select all text in editor"));
	selectAll_->setShortcut(QKeySequence(QKeySequence::SelectAll));
	selectAll_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(selectAll_, SIGNAL(triggered()), plane_, SLOT(selectAll()));

	copy_ = new QAction(plane_);
	copy_->setObjectName("edit-copy");
	copy_->setText(QObject::tr("Copy selection to clipboard"));
//    copy_->setIcon(QIcon(qtcreatorIconsPath+"editcopy.png"));
	copy_->setIcon(Widgets::IconProvider::self()->iconForName("edit-copy"));
	copy_->setShortcut(QKeySequence(QKeySequence::Copy));
	copy_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(copy_, SIGNAL(triggered()), plane_, SLOT(copy()));

	cut_ = new QAction(plane_);
	cut_->setObjectName("edit-cut");
	cut_->setText(QObject::tr("Cut selection to clipboard"));
//    cut_->setIcon(QIcon(qtcreatorIconsPath+"editcut.png"));
	cut_->setIcon(Widgets::IconProvider::self()->iconForName("edit-cut"));
	cut_->setShortcut(QKeySequence(QKeySequence::Cut));
	cut_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(cut_, SIGNAL(triggered()), plane_, SLOT(cut()));

	paste_ = new QAction(plane_);
	paste_->setObjectName("edit-paste");
	paste_->setText(QObject::tr("Paste from clipboard"));
//    paste_->setIcon(QIcon(qtcreatorIconsPath+"editpaste.png"));
	paste_->setIcon(Widgets::IconProvider::self()->iconForName("edit-paste"));
	paste_->setShortcut(QKeySequence(QKeySequence::Paste));
	paste_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(paste_, SIGNAL(triggered()), plane_, SLOT(paste()));

	find_ = new QAction(plane_);
	find_->setText(QObject::tr("Find..."));
	find_->setShortcut(QKeySequence(QKeySequence::Find));
	find_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(find_, SIGNAL(triggered()),
		findReplace_, SLOT(showFind()));

	replace_ = new QAction(plane_);
	replace_->setText(QObject::tr("Replace..."));
	replace_->setShortcut(QKeySequence(QKeySequence::Replace));
	replace_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(replace_, SIGNAL(triggered()),
		findReplace_, SLOT(showReplace()));



	deleteLine_ = new QAction(plane_);
	deleteLine_->setText(QObject::tr("Delete line under cursor"));
	deleteLine_->setShortcut(QKeySequence("Ctrl+Y"));
	deleteLine_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(deleteLine_, SIGNAL(triggered()), plane_, SLOT(removeLine()));

	deleteTail_ = new QAction(plane_);
	deleteTail_->setText(QObject::tr("Delete text from cursor to end of line"));
	deleteTail_->setShortcut(QKeySequence("Ctrl+K"));
	deleteTail_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(deleteTail_, SIGNAL(triggered()), plane_, SLOT(removeLineTail()));

	undo_ = new QAction(plane_);
	undo_->setEnabled(false);
	undo_->setObjectName("edit-undo");
	undo_->setText(QObject::tr("Undo last action"));
//    undo_->setIcon(QIcon(qtcreatorIconsPath+"undo.png"));
	undo_->setIcon(Widgets::IconProvider::self()->iconForName("edit-undo"));
	undo_->setShortcut(QKeySequence::Undo);
	undo_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(cursor_, SIGNAL(undoAvailable(bool)), undo_, SLOT(setEnabled(bool)));
	QObject::connect(undo_, SIGNAL(triggered()), this, SLOT(undo()));

	redo_ = new QAction(plane_);
	redo_->setEnabled(false);
	redo_->setObjectName("edit-redo");
	redo_->setText(QObject::tr("Redo last undoed action"));
//    redo_->setIcon(QIcon(qtcreatorIconsPath+"redo.png"));
	redo_->setIcon(Widgets::IconProvider::self()->iconForName("edit-redo"));
	redo_->setShortcut(QKeySequence("Ctrl+Shift+Z"));
	redo_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(cursor_, SIGNAL(redoAvailable(bool)), redo_, SLOT(setEnabled(bool)));
	QObject::connect(redo_, SIGNAL(triggered()), this, SLOT(redo()));

	toggleComment_ = new QAction(plane_);
	toggleComment_->setText(QObject::tr("(Un)Comment lines"));
	toggleComment_->setShortcut(QKeySequence("Ctrl+/"));
	toggleComment_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(toggleComment_, SIGNAL(triggered()),
		cursor_, SLOT(toggleComment()));

	toggleLock_ = new QAction(plane_);
	toggleLock_->setText(QObject::tr("(Un)Lock lines from editing"));
	toggleLock_->setShortcut(QKeySequence("Ctrl+L"));
	toggleLock_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	QObject::connect(toggleLock_, SIGNAL(triggered()),
		cursor_, SLOT(toggleLock()));

	recordMacro_ = new QAction(plane_);
	recordMacro_->setEnabled(true);
	recordMacro_->setCheckable(true);
	recordMacro_->setText(tr("Record keyboard sequence"));
	recordMacro_->setShortcut(QKeySequence("Ctrl+M"));
	recordMacro_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(recordMacro_, SIGNAL(triggered(bool)), this, SLOT(toggleRecordMacro(bool)));

	editMacros_ = new QAction(plane_);
	editMacros_->setText(tr("Edit saved keyboard sequences..."));
	connect(editMacros_, SIGNAL(triggered()), this, SLOT(editMacros()));


	separatorAction_ = new QAction(this);
	separatorAction_->setSeparator(true);

	editMenu_ = new QMenu(tr("Edit"), 0);
	editMenu_->setProperty("menuRole", "edit");
	editMenu_->addAction(undo_);
	editMenu_->addAction(redo_);
	editMenu_->addSeparator();
	editMenu_->addAction(selectAll_);
	editMenu_->addSeparator();
	editMenu_->addAction(cut_);
	editMenu_->addAction(copy_);
	editMenu_->addAction(paste_);
	editMenu_->addSeparator();
	editMenu_->addAction(find_);
	editMenu_->addAction(replace_);
	editMenu_->addSeparator();
	editMenu_->addAction(deleteLine_);
	editMenu_->addAction(deleteTail_);


	editMenu_->addSeparator();
	editMenu_->addAction(toggleComment_);
	editMenu_->addAction(toggleLock_);


	editMenu_->addSeparator();
	editMenu_->addAction(recordMacro_);
	editMenu_->addAction(editMacros_);

	insertMenu_ = new Widgets::CyrillicMenu(tr("Insert"), 0);
	insertMenu_->setProperty("menuRole", "insert");

	toggleBreakpoint_ = 0;

}

const TextCursor *EditorInstance::cursor() const
{
	return cursor_;
}

const TextDocument *EditorInstance::document() const
{
	return doc_;
}

TextCursor *EditorInstance::cursor()
{
	return cursor_;
}

TextDocument *EditorInstance::document()
{
	return doc_;
}

Shared::Analizer::InstanceInterface *EditorInstance::analizer()
{
	return analizerInstance_;
}


QList<QAction *> EditorInstance::toolBarActions() const
{
	QList<QAction *> result;
	result << cut_;
	result << copy_;
	result << paste_;
	result << separatorAction_;
	result << undo_;
	result << redo_;

	return result;
}

void EditorInstance::undo()
{
	cursor_->undo();
}

void EditorInstance::redo()
{
	cursor_->redo();
}

void EditorInstance::toggleRecordMacro(bool on)
{
	if (on) {
		cursor_->startRecordMacro();
	} else {
		QSharedPointer<Macro> macro = cursor_->endRecordMacro();
		if (macro->commands.size() > 0) {
			MacroEditor *editor = new MacroEditor(this);
			editor->setWindowTitle(tr("New keyboard sequence..."));
			QList<QSharedPointer<Macro> > allMacros = systemMacros_ + userMacros_;
			QString usedLetters;
			QStringList usedNames;
			foreach (QSharedPointer<Macro> m, allMacros) {
				if (!m->key.isNull()) {
					usedLetters.push_back(m->key);
					usedNames.push_back(m->title);
				}
			}
			editor->setUsedSymbols(usedLetters, usedNames);
			editor->setMacro(macro);
			if (editor->exec() == QDialog::Accepted) {
				userMacros_.push_back(macro);
				plugin_->updateUserMacros(analizerPlugin_ ? analizerPlugin_->defaultDocumentFileNameSuffix() : QString(), userMacros_, true);
			}
			editor->deleteLater();
		}
	}
	emit recordMacroChanged(on);
}

void EditorInstance::editMacros()
{
	MacroListEditor *editor = new MacroListEditor(plugin_->myResourcesDir(), this);
	editor->initialize(userMacros_,
		systemMacros_);
	editor->exec();
	userMacros_ = editor->result();
	plugin_->updateUserMacros(analizerPlugin_ ? analizerPlugin_->defaultDocumentFileNameSuffix() : QString(), userMacros_, true);
	editor->deleteLater();
}

QList<QMenu *> EditorInstance::menus() const
{
	QList<QMenu *> result;
	result << editMenu_;
	bool nonEmptyInsertMenu = false;
	for (int i = 0; i < insertMenu_->children().size(); i++) {
		QObject *child = insertMenu_->children().at(i);
		const QString clazz = child->metaObject()->className();
		if (clazz == "QAction" && child != insertMenu_->menuAction()) {
			nonEmptyInsertMenu = true;
			break;
		}
	}
	if (nonEmptyInsertMenu) {
		result << insertMenu_;
	}
	return result;
}



void EditorInstance::setKumFile(const Shared::Analizer::SourceFileInterface::Data &data)
{
	notSaved_ = true;
	doc_->setKumFile(data, plugin_->teacherMode_);
	if (analizerInstance_) {
		toggleComment_->setVisible(true);
		toggleComment_->setEnabled(true);
		QString plainText = data.visibleText;
		if (data.hasHiddenText) {
			plainText += "\n" + data.hiddenText;
		}
		analizerInstance_->setSourceText(plainText);
		updateFromAnalizer();
	} else {
		toggleComment_->setVisible(false);
		toggleComment_->setEnabled(false);
	}
	updateInsertMenu();
	plane_->setLineHighlighted(-1, QColor(), 0, 0);
	plane_->updateScrollBars();
	plane_->update();
	checkForClean();
	setNotModified();
}

void  EditorInstance::setPlainText(const QString &data)
{
	doc_->setPlainText(data);
	if (analizerInstance_) {
		analizerInstance_->setSourceText(data);
		updateFromAnalizer();
	}
	plane_->setLineHighlighted(-1, QColor(), 0, 0);
	plane_->update();
	checkForClean();
}

Shared::Analizer::SourceFileInterface::Data EditorInstance::documentContents() const
{
	Shared::Analizer::SourceFileInterface::Data data = doc_->toKumFile();
	data.sourceUrl = documentUrl_;
	return data;
}

bool EditorInstance::hasBreakpointSupport() const
{
	Shared::RunInterface *runner =
		ExtensionSystem::PluginManager::instance()->findPlugin<Shared::RunInterface>();
	return nullptr != analizerInstance_ && nullptr != runner && runner->hasBreakpointsSupport();
}

void EditorInstance::saveDocument(const QString &fileName, QString *error)
{
	QFile f(fileName);
	if (f.open(QIODevice::WriteOnly)) {
		saveDocument(&f, error);
		f.close();
		documentUrl_ = QUrl::fromLocalFile(fileName);
	} else {
		if (error) {
			*error = tr("Can't open file %1 for writing").arg(fileName);
		}
	}
}

void EditorInstance::saveDocument(QIODevice *device, QString *error)
{
	if (analizerPlugin_) {
		QByteArray bytes = analizerPlugin_->sourceFileHandler()->toBytes(documentContents());
		device->write(bytes);
	} else {
		QTextStream ts(device);
#ifdef Q_OS_WIN32
		ts.setCodec("CP1251");
#else
		ts.setCodec("UTF-8");
		ts.setGenerateByteOrderMark(true);
#endif
		ts << documentContents().visibleText;
		ts.flush();
	}
	notSaved_ = false;
	checkForClean();
	doc_->undoStack()->setClean();
}

quint32 EditorInstance::currentLineNumber() const
{
	return cursor_->row();
}

quint32 EditorInstance::errorLinesCount() const
{
	QSet<int> lines;
	if (analizerInstance_) {
		QList<Shared::Analizer::Error> errors = analizerInstance_->errors();
		foreach (const Shared::Analizer::Error &e, errors) {
			if (e.line >= 0 && e.line < (int)doc_->linesCount()) {
				if (plugin_->teacherMode_ || !doc_->at(e.line).hidden) {
					lines.insert(e.line);
				}
			}
		}
	}
	return lines.size();
}

void EditorInstance::highlightLineGreen(int lineNo, quint32 colStart, quint32 colEnd)
{
	const QColor bgColor = palette().color(QPalette::Base);
	int darkness = bgColor.red() + bgColor.green() + bgColor.blue();
	QColor color;
	if (darkness / 3 <= 127) {
		color = QColor("palegreen");
	} else {
		color = QColor(Qt::darkGreen);
	}
	setLineHighlighted(lineNo, color, colStart, colEnd);
}

void EditorInstance::highlightLineRed(int lineNo, quint32 colStart, quint32 colEnd)
{
	const QColor bgColor = palette().color(QPalette::Base);
	int darkness = bgColor.red() + bgColor.green() + bgColor.blue();
	QColor color;
	if (darkness / 3 <= 127) {
		color = QColor("lightcoral");
	} else {
		color = QColor(Qt::red);
	}
	setLineHighlighted(lineNo, color, colStart, colEnd);
}

void EditorInstance::unhighlightLine()
{
	setLineHighlighted(-1, QColor::Invalid, 0u, 0u);
}

void EditorInstance::ensureAnalized()
{
	doc_->forceCompleteRecompilation(QPoint(cursor_->column(), cursor_->row()));
}

bool EditorInstance::isTeacherMode() const
{
	return plugin_->teacherMode_;
}

void EditorInstance::updateSettings(const QStringList &keys)
{
	plane_->updateSettings(keys);
	plane_->update();
	if (autocompleteWidget_) {
		autocompleteWidget_->setFont(plane_->font());
	}
}

void EditorInstance::changeGlobalState(quint32 prevv, quint32 currentt)
{
	ExtensionSystem::GlobalState prev = ExtensionSystem::GlobalState(prevv);
	ExtensionSystem::GlobalState current = ExtensionSystem::GlobalState(currentt);

	using Shared::PluginInterface;

	if (current == PluginInterface::GS_Unlocked || current == PluginInterface::GS_Running) {
		unhighlightLine();
	}
	if (prev == PluginInterface::GS_Observe && current != PluginInterface::GS_Observe) {
		clearMarginText();
	}
	if (current == PluginInterface::GS_Unlocked || current == PluginInterface::GS_Observe) {
		unlock();
	} else {
		lock();
	}
}

void EditorInstance::setNotModified()
{
	doc_->undoStack()->setClean();
	notSaved_ = false;
	emit documentCleanChanged(true);
}

bool EditorInstance::isModified() const
{
	return notSaved_ || ! doc_->undoStack()->isClean();
}

void EditorInstance::checkForClean()
{
	emit documentCleanChanged(!isModified());
}

bool EditorInstance::forceNotSavedFlag() const
{
	return notSaved_;
}

void EditorInstance::setForceNotSavedFlag(bool v)
{
	notSaved_ = v;
	checkForClean();
}

} // namespace Editor


