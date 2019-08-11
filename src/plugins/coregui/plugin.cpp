#include "tabwidgetelement.h"
#include "switchworkspacedialog.h"
#include "plugin.h"
#include "mainwindow.h"
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2-libs/widgets/secondarywindow.h>
#include "debuggerview.h"
#include "ui_mainwindow.h"
#include "statusbar.h"
#include "tabwidget.h"
#include <kumir2-libs/widgets/iconprovider.h>
#include <kumir2-libs/widgets/actionproxy.h>

#include "guisettingspage.h"
#include "iosettingseditorpage.h"

#include "defaultstartpage.h"

#ifdef Q_OS_MACX
#include "mac-fixes.h"
#endif
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    #include <signal.h>
    #if defined(__FreeBSD__)
        typedef union sigval sigval_t;
    #endif /* __FreeBSD__ */
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

namespace CoreGUI {

using namespace Shared;

Plugin::Plugin() :
    KPlugin()
{
#if defined(Q_OS_WIN32)
    ipcShm_ = 0;
#endif
    mainWindow_ = 0;
    m_kumirStateLabel = 0;
    m_genericCounterLabel = 0;
    plugin_editor = 0;
    plugin_NativeGenerator = plugin_BytecodeGenerator = 0;
    plugin_browser = 0;
    plugin_kumirCodeRun = 0;
    helpWindow_ = 0;
    coursesWindow_ = 0;
    terminal_ = 0;
    kumirProgram_ = 0;
    _debugger = 0;
    sessionsDisableFlag_ = false;
    helpViewer_ = 0;
    courseManager_ = 0;
    guiSettingsPage_ = 0;
    ioSettingsPage_ = 0;
}

QString Plugin::InitialTextKey = "InitialText";
QString Plugin::SessionFilesListKey = "Session/Files";
QString Plugin::SessionTabIndexKey = "Session/TabIndex";
QString Plugin::RecentFileKey = "History/FileDialog";
QString Plugin::RecentFilesKey = "History/RecentFiles";
QString Plugin::MainWindowGeometryKey = "Geometry/MainWindow";
QString Plugin::MainWindowShowConsoleKey = "State/ConsoleToggle";
QString Plugin::MainWindowStateKey = "State/MainWindow";
QString Plugin::MainWindowSplitterStateKey = "State/MainWindowSplitter";
QString Plugin::DockVisibleKey = "DockWindow/Visible";
QString Plugin::DockFloatingKey = "DockWindow/Floating";
QString Plugin::DockGeometryKey = "DockWindow/Geometry";
QString Plugin::DockSideKey = "DockWindow/Side";
QString Plugin::UseSystemFontSizeKey = "Application/UseSystemFontSize";
bool Plugin::UseSystemFontSizeDefaultValue = true;
QString Plugin::OverrideFontSizeKey = "Application/OverideFontSize";
QString Plugin::PresentationModeMainFontSizeKey = "Presentation/MainFontSize";
QString Plugin::PresentationModeEditorFontSizeKey = "Presentation/EditorFontSize";
int Plugin::PresentationModeMainFontSizeDefaultValue = 14;
int Plugin::PresentationModeEditorFontSizeDefaultValue = 20;

Plugin* Plugin::instance_ = 0;

QList<CommandLineParameter> Plugin::acceptableCommandLineParameters() const
{
    QList<CommandLineParameter> result;
    result << CommandLineParameter(
                  true,
                  tr("PROGRAM.kum"),
                  tr("Source file name"),
                  QVariant::String,
                  false
                  );
    return result;
}

QString Plugin::initialize(const QStringList & parameters, const ExtensionSystem::CommandLine & cmd)
{

    instance_ = this;

    if (cmd.size() > 0 && cmd.value(size_t(0)).isValid()) {

        fileNameToOpenOnReady_ = cmd.value(size_t(0)).toString();

        if (! QFileInfo(fileNameToOpenOnReady_).isAbsolute()) {

            fileNameToOpenOnReady_ =
                    QDir::current().absoluteFilePath(fileNameToOpenOnReady_);

        }
    }

    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");

    qRegisterMetaType<Shared::Analizer::SourceFileInterface::Data>("KumirFile.Data");

    qRegisterMetaType<Shared::GuiInterface::ProgramSourceText::Language>
            ("Gui.ProgramSourceText.Language");

    qRegisterMetaType<Shared::GuiInterface::ProgramSourceText>
            ("Gui.ProgramSourceText");


    connect(this, SIGNAL(externalProcessCommandReceived(QString)),
            this, SLOT(handleExternalProcessCommand(QString)),
            Qt::QueuedConnection);



    const QStringList BlacklistedThemes = QStringList()
            << "iaorakde" << "iaoraqt" << "iaora";

    const QString currentStyleName = qApp->style()->objectName().toLower();

    if (BlacklistedThemes.contains(currentStyleName)) {

        qApp->setStyle("Cleanlooks");

    }

    sessionsDisableFlag_ = parameters.contains("nosessions",Qt::CaseInsensitive);

    m_kumirStateLabel = new QLabel();

    m_genericCounterLabel = new QLabel();

    qDebug() << "Creating main window";

    mainWindow_ = new MainWindow(this);
    mainWindow_->setWindowIcon(QApplication::windowIcon());
    qDebug() << "Main window created";


#ifdef Q_OS_MACX
   // void * mac_mainWindow = (class NSView*)(m_mainWindow->winId());
    //MacFixes::setLionFullscreenButton(mac_mainWindow);
#endif

    plugin_editor = PluginManager::instance()->findPlugin<EditorInterface>();



    plugin_BytecodeGenerator = qobject_cast<GeneratorInterface*>(myDependency("KumirCodeGenerator"));

    plugin_NativeGenerator = qobject_cast<GeneratorInterface*>(myDependency("KumirNativeGenerator"));

    plugin_browser = qobject_cast<BrowserInterface*>(myDependency("Browser"));

    if (!plugin_editor)
        return "Can't load editor plugin!";

    terminal_ = new Term(mainWindow_);
    terminal_->setTerminalFont(plugin_editor->defaultEditorFont());
    terminal_->setSettings(mySettings());

    mainWindow_->consolePlace_->addPersistentWidget(terminal_,
                                                              tr("Input/Output"));


    connect(terminal_, SIGNAL(showWindowRequest()),
            mainWindow_, SLOT(ensureBottomVisible()));

    connect(terminal_, SIGNAL(message(QString)),
            mainWindow_, SLOT(showMessage(QString)));

    static const int StatusBarIconSize = 16;

    static const QString MdiIconsSubdir =
            QString::fromLatin1("/icons/iconset/%1x%2/")
            .arg(StatusBarIconSize).arg(StatusBarIconSize);

    static const QString mdiIconsPath =
            ExtensionSystem::PluginManager::instance()
            ->findSystemResourcesDir(MdiIconsSubdir).absolutePath() + MdiIconsSubdir;

    static const QString showConsoleIconPath = mdiIconsPath + "/statusbar-terminal.png";
    static const QString saveConsoleIconPath = mdiIconsPath + "/statusbar-save.png";
    static const QString copyConsoleIconPath = mdiIconsPath + "/statusbar-copy.png";
    static const QString clearConsoleIconPath = mdiIconsPath + "/statusbar-clear.png";
    static const QString editConsoleIconPath = mdiIconsPath + "/statusbar-edit.png";

    const QIcon showConsoleIcon = Widgets::IconProvider::iconFromPath(showConsoleIconPath, 0.75);
    const QIcon saveConsoleIcon = Widgets::IconProvider::iconFromPath(saveConsoleIconPath, 0.75);
    const QIcon copyConsoleIcon = Widgets::IconProvider::iconFromPath(copyConsoleIconPath, 0.75);
    const QIcon clearConsoleIcon = Widgets::IconProvider::iconFromPath(clearConsoleIconPath, 0.75);
    const QIcon editConsoleIcon = Widgets::IconProvider::iconFromPath(editConsoleIconPath, 0.75);



    QToolButton * btnShowConsole = new QToolButton(mainWindow_);

    mainWindow_->ui->actionShow_Console_Pane->setIcon(showConsoleIcon);

    btnShowConsole->setDefaultAction(mainWindow_->ui->actionShow_Console_Pane);
    btnShowConsole->setIconSize(QSize(StatusBarIconSize, StatusBarIconSize));

    btnShowConsole->setToolTip(mainWindow_->ui->actionShow_Console_Pane->text());

    mainWindow_->statusBar_->addButtonToLeft(btnShowConsole);

    QToolButton * btnSaveTerm = new QToolButton(mainWindow_);

    btnSaveTerm->setPopupMode(QToolButton::InstantPopup);

    QMenu * menuSaveTerm = new QMenu(btnSaveTerm);

    btnSaveTerm->setToolTip(tr("Save console output"));
    btnSaveTerm->setMenu(menuSaveTerm);
    btnSaveTerm->setIcon(saveConsoleIcon);
    btnSaveTerm->setIconSize(QSize(StatusBarIconSize, StatusBarIconSize));
    menuSaveTerm->addAction(terminal_->actionSaveLast());
    menuSaveTerm->addAction(terminal_->actionSaveAll());

    mainWindow_->statusBar_->addButtonToLeft(btnSaveTerm);


    QToolButton * btnCopyTerm = new QToolButton(mainWindow_);
    btnCopyTerm->setPopupMode(QToolButton::InstantPopup);

    QMenu * menuCopyTerm = new QMenu(btnCopyTerm);
    btnCopyTerm->setToolTip(tr("Copy to clipboard console output"));
    btnCopyTerm->setMenu(menuCopyTerm);
    btnCopyTerm->setIcon(copyConsoleIcon);
    btnCopyTerm->setIconSize(QSize(StatusBarIconSize, StatusBarIconSize));
    menuCopyTerm->addAction(terminal_->actionCopyLast());
    menuCopyTerm->addAction(terminal_->actionCopyAll());
    mainWindow_->statusBar_->addButtonToLeft(btnCopyTerm);


    QToolButton * btnClearTerm = new QToolButton(mainWindow_);
    terminal_->actionClear()->setIcon(clearConsoleIcon);
    btnClearTerm->setDefaultAction(terminal_->actionClear());
    btnClearTerm->setIconSize(QSize(StatusBarIconSize, StatusBarIconSize));
    mainWindow_->statusBar_->addButtonToLeft(btnClearTerm);

    QToolButton * btnEditTerm = nullptr;

    if (!parameters.contains("notabs",Qt::CaseInsensitive)) {
        btnEditTerm = new QToolButton(mainWindow_);
        btnEditTerm->setIconSize(QSize(StatusBarIconSize, StatusBarIconSize));
        terminal_->actionEditLast()->setIcon(editConsoleIcon);
        btnEditTerm->setDefaultAction(terminal_->actionEditLast());
        mainWindow_->statusBar_->addButtonToLeft(btnEditTerm);
    }


#ifdef Q_OS_MAC
    static const char * statusBarButtonCSS =
            "QToolButton {"
            "   border: 0;"
            "}"
            "QToolButton:checked, QToolButton:pressed {"
            "   border: 1px solid gray;"
            "   border-radius: 5px;"
            "   background-color: lightgray;"
            "}"
            ;
    btnShowConsole->setStyleSheet(statusBarButtonCSS);
    btnSaveTerm->setStyleSheet(statusBarButtonCSS);
    btnClearTerm->setStyleSheet(statusBarButtonCSS);
    if (btnEditTerm) {
        btnEditTerm->setStyleSheet(statusBarButtonCSS);
    }
#endif


    kumirProgram_ = new KumirProgram(this);

    kumirProgram_->setTerminal(terminal_, 0);



    connect(kumirProgram_, SIGNAL(giveMeAProgram()), this, SLOT(prepareKumirProgramToRun()), Qt::DirectConnection);


    helpViewer_ = new DocBookViewer::DocBookView(mainWindow_);

    helpViewer_->updateSettings(mySettings(), "HelpViewer");

    static const QString helpPath =
           ExtensionSystem::PluginManager::instance()->sharePath() +
            "/userdocs/";


    const QString applicationLanucher = QDir::fromNativeSeparators(qApp->arguments().at(0));
    QString applicationName =
            applicationLanucher.startsWith(qApp->applicationDirPath())
            ? applicationLanucher.mid(qApp->applicationDirPath().length() + 1)
            : applicationLanucher;
#ifdef Q_OS_WIN32
    if (applicationName.endsWith(".exe")) {
        applicationName.remove(applicationName.length()-4, 4);
    }
#endif

#ifndef Q_OS_MACX
    QString indexFileName = applicationName + ".xml";
    indexFileName.replace("kumir2-", "index-");
#else
    QString indexFileName = "index-macx.xml";
#endif

    Shared::AnalizerInterface * analizerPlugin =
            ExtensionSystem::PluginManager::instance()
            ->findPlugin<Shared::AnalizerInterface>();

    if (analizerPlugin) {
        const QString apiHelpListingRole = analizerPlugin->languageName();
        helpViewer_->setRole(DocBookViewer::ProgramListing, apiHelpListingRole);
        helpViewer_->setRole(DocBookViewer::FuncSynopsys, apiHelpListingRole);
        helpViewer_->setRole(DocBookViewer::Section, apiHelpListingRole);
        helpViewer_->setRole(DocBookViewer::Type, apiHelpListingRole);
        helpViewer_->setRole(DocBookViewer::Code, apiHelpListingRole);

        if (analizerPlugin->languageQuickReferenceWidget()) {
            mainWindow_->ui->actionLanguage_Quick_Reference->setVisible(true);
            mainWindow_->ui->actionLanguage_Quick_Reference->setEnabled(true);
            quickRefWindow_ = Widgets::SecondaryWindow::createSecondaryWindow(
                        analizerPlugin->languageQuickReferenceWidget(),
                        tr("Quick Reference"),
                        QIcon(),
                        mainWindow_,
//                        mainWindow_->helpAndCoursesPlace_,
                        0,
                        "QuickReferenceWindow",
                        true
                        );
            connect(mainWindow_->ui->actionLanguage_Quick_Reference, SIGNAL(triggered()),
                    quickRefWindow_, SLOT(activate()));
            connect(analizerPlugin->languageQuickReferenceWidget(),
                    SIGNAL(openTopicInDocumentation(int,QString)),
                    this, SLOT(showHelpWindowFromQuickReference(int,QString)));
        }
    }

    indexFileName = helpPath + indexFileName;
    if (QFile::exists(indexFileName)) {
        helpViewer_->addDocument(QUrl::fromLocalFile(indexFileName));
    }



    helpWindow_ = Widgets::SecondaryWindow::createSecondaryWindow(
                helpViewer_,
                tr("Help"),
                QIcon(), // TODO help window icon
                mainWindow_,
                mainWindow_->helpAndCoursesPlace_,
                "HelpViewerWindow",
                true
                );


    connect(mainWindow_->ui->actionUsage, SIGNAL(triggered()),
            mainWindow_, SLOT(showHelp()));

    secondaryWindows_ << helpWindow_;



    courseManager_ = ExtensionSystem::PluginManager::instance()
            ->findPlugin<Shared::CoursesInterface>();


    if (courseManager_) {

        foreach (QMenu* menu, courseManager_->menus()) {

            menu->setObjectName("menu-CourseManager");
            mainWindow_->addMenuBeforeHelp(menu);
        }

        coursesWindow_ = Widgets::SecondaryWindow::createSecondaryWindow(
                    courseManager_->mainWindow(),
                    tr("Courses"),
                    QIcon(),
                    mainWindow_,
                    mainWindow_->helpAndCoursesPlace_,
                    "CoursesWindow",
                    true
                    );

        QAction * showCourses =
                mainWindow_->ui->menuWindow->addAction(
                    tr("Courses"),
                    coursesWindow_, SLOT(activate())
                    );

        showCourses->setObjectName("window-courses");


//        const QString courseIconFileName = ExtensionSystem::PluginManager::instance()->sharePath()+"/icons/course.png";

//        QIcon courseIcon(courseIconFileName);

        showCourses->setIcon(Widgets::IconProvider::self()->iconForName("window-cources"));
//        showCourses->setIcon(courseIcon);


        mainWindow_->gr_otherActions->addAction(showCourses);

        secondaryWindows_ << coursesWindow_;

    }


    KPlugin * kumirRunner = ExtensionSystem::PluginManager::instance()
            ->findKPlugin<RunInterface>();

    plugin_kumirCodeRun = qobject_cast<RunInterface*>(kumirRunner);

    connect(kumirRunner, SIGNAL(showActorWindowRequest(QByteArray)),
            this, SLOT(showActorWindow(QByteArray)));


    QList<ExtensionSystem::KPlugin*> actors = loadedPlugins("Actor*");

    actors += loadedPlugins("st_funct");

    QList<QUrl> actorHelpFiles;
    foreach (ExtensionSystem::KPlugin* o, actors) {
        ActorInterface * actor = qobject_cast<ActorInterface*>(o);
        const QString actorName = Shared::actorCanonicalName(actor->localizedModuleName(QLocale::Russian));
        const QString actorObjectName = Shared::actorCanonicalName(actor->asciiModuleName()).replace(" ", "-").toLower();
        l_plugin_actors << actor;
        QWidget * w = 0;
        const QString actorHelpFile = helpPath + o->pluginSpec().name + ".xml";
        if (!actor->localizedModuleName(QLocale::Russian).startsWith("_") && QFile(actorHelpFile).exists()) {
            actorHelpFiles.append(QUrl::fromLocalFile(actorHelpFile));
        }
        QList<QMenu*> actorMenus = actor->moduleMenus();
        foreach (QMenu* menu, actorMenus) {
            menu->setObjectName("menu-Actor" + QString::fromLatin1(Shared::actorCanonicalName(actor->asciiModuleName())));
            mainWindow_->addMenuBeforeHelp(menu);
        }
        if (actor->mainWidget()) {
            QWidget * actorWidget = actor->mainWidget();
            const QString iconFileName = ExtensionSystem::PluginManager::instance()->sharePath()+"/icons/actors/"+actor->mainIconName()+".png";
            const QString smallIconFileName = ExtensionSystem::PluginManager::instance()->sharePath()+"/icons/actors/"+actor->mainIconName()+"_22x22.png";
            QIcon mainIcon = QIcon(iconFileName);
            if (QFile::exists(smallIconFileName))
                mainIcon.addFile(smallIconFileName, QSize(22,22));

            const QSizePolicy actorSizePolicy = actorWidget->sizePolicy();
            const QSizePolicy::Policy horz = actorSizePolicy.horizontalPolicy();
            const QSizePolicy::Policy vert = actorSizePolicy.verticalPolicy();
            bool resizableX = horz != QSizePolicy::Fixed;
            bool resizableY = vert != QSizePolicy::Fixed;
            bool resizable = resizableX && resizableY;

            QIcon themeProvidedIcon = Widgets::IconProvider::self()->iconForName(
                        QString::fromLatin1("window-")+
                        QString::fromLatin1(actor->asciiModuleName().toLower())
                        );

            if (!themeProvidedIcon.isNull()) {
                mainIcon = themeProvidedIcon;
            }

            Widgets::SecondaryWindow * actorWindow =
                    Widgets::SecondaryWindow::createSecondaryWindow(
                        actorWidget,
                        actorName,
                        mainIcon,
                        mainWindow_,
                        mainWindow_->actorsPlace_,
                        o->pluginSpec().name,
                        resizable
                        );

            secondaryWindows_ << actorWindow;

            QAction * showActor =
                    mainWindow_->ui->menuWindow->addAction(
                        actorName,
                        actorWindow,
                        SLOT(activate())
                        );


            showActorActions_[actor->asciiModuleName()] = showActor;

            showActor->setObjectName("window-actor-" + actorObjectName);
            mainWindow_->gr_otherActions->addAction(showActor);
            if (!actor->mainIconName().isEmpty()) {
                showActor->setIcon(mainIcon);
            }

            if (actor->moduleMenus().size() > 0) {
                QMenu * actorMainMenu = actor->moduleMenus().first();
                Widgets::ActionProxy *showActorProxy =
                        new Widgets::ActionProxy(showActor, actorMainMenu);
                showActorProxy->setText(tr("Show actor window"));
                actorMainMenu->addSeparator();
                actorMainMenu->addAction(showActorProxy);
            }            
        }
        if (actor->pultWidget()) {
            const QString iconFileName = ExtensionSystem::PluginManager::instance()->sharePath()+"/icons/actors/"+actor->pultIconName()+".png";
            const QString smallIconFileName = ExtensionSystem::PluginManager::instance()->sharePath()+"/icons/actors/"+actor->pultIconName()+"_22x22.png";
            QIcon pultIcon = QIcon(iconFileName);
            if (QFile::exists(smallIconFileName))
                pultIcon.addFile(smallIconFileName, QSize(22,22));
            QIcon themeProvidedIcon = Widgets::IconProvider::self()->iconForName(
                        QString::fromLatin1("window-")+
                        QString::fromLatin1(actor->asciiModuleName().toLower())+
                        QString::fromLatin1("-control")
                        );

            if (!themeProvidedIcon.isNull()) {
                pultIcon = themeProvidedIcon;
            }

            Widgets::SecondaryWindow * pultWindow =
                    Widgets::SecondaryWindow::createSecondaryWindow(
                        actor->pultWidget(),
                        actorName + " - " + tr("Remote Control"),
                        pultIcon,
                        mainWindow_,
                        nullptr,
                        o->pluginSpec().name + "Pult",
                        false
                        );
            secondaryWindows_ << pultWindow;

            QAction * showPult =
                    mainWindow_->ui->menuWindow->addAction(
                        actorName + " - " + tr("Remote Control"),
                        pultWindow,
                        SLOT(activate())
                        );
            showPult->setObjectName("window-control-" + actorObjectName);

            mainWindow_->gr_otherActions->addAction(showPult);

            if (!actor->pultIconName().isEmpty()) {
                showPult->setIcon(pultIcon);
            }

            if (actor->moduleMenus().size() > 0) {
                QMenu * actorMainMenu = actor->moduleMenus().first();
                Widgets::ActionProxy *showRCProxy =
                        new Widgets::ActionProxy(showPult, actorMainMenu);
                showRCProxy->setText(tr("Show actor control"));
                actorMainMenu->addAction(showRCProxy);
            }
        }
    }
    helpViewer_->addDocuments(tr("Actor's References"), actorHelpFiles);


    if (!parameters.contains("nostartpage", Qt::CaseInsensitive)) {
        createStartPage();
    }

    if (parameters.contains("notabs", Qt::CaseInsensitive)) {

        mainWindow_->disableTabs();
    }

    connect(terminal_, SIGNAL(openTextEditor(QString,QString)),
            mainWindow_, SLOT(newText(QString,QString)), Qt::DirectConnection);


    _debugger = new DebuggerView(plugin_kumirCodeRun);


    Widgets::SecondaryWindow * debuggerWindow =
            Widgets::SecondaryWindow::createSecondaryWindow(
                _debugger,
                tr("Variables"),
                QIcon(), // TODO window icon
                mainWindow_,
                mainWindow_->debuggerPlace_,
                "DebuggerWindow",
                true
                );

    mainWindow_->debuggerWindow_ = debuggerWindow;

    secondaryWindows_ << debuggerWindow;


    connect(mainWindow_->ui->actionVariables, SIGNAL(triggered()),
            debuggerWindow, SLOT(activate()));


    const QString layoutChoice =
            mySettings()->value(GUISettingsPage::LayoutKey, GUISettingsPage::ColumnsFirstValue).toString();

    if (layoutChoice == GUISettingsPage::ColumnsFirstValue) {

        mainWindow_->switchToColumnFirstLayout();

    }
    else {

        mainWindow_->switchToRowFirstLayout();

    }

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACX)
    struct sigaction act;
    act.sa_sigaction = handleSIGUSR1;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    act.sa_mask = set;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &act, 0);
#endif
#if defined(Q_OS_WIN32)
    ipcShm_ = new QSharedMemory(QFileInfo(qApp->arguments().at(0)).fileName(), this);
    if (ipcShm_->create(2048, QSharedMemory::ReadWrite)) {
        ipcShm_->lock();
        memset(ipcShm_->data(), 0, ipcShm_->size());
        ipcShm_->unlock();
        startTimer(250);
    }
#endif

    mainWindow_->addPresentationModeItemToMenu();  // the last button in "Window" menu, after actor's windows

    return "";
} // End initialize

void Plugin::handleExternalProcessCommand(const QString &command)
{
    int spacePos = command.indexOf(' ');
    QString cmd;
    QString arg;
    if (spacePos == -1) {
        cmd = command.trimmed();
    }
    else {
        cmd = command.left(spacePos).trimmed();
        arg = command.mid(spacePos+1).trimmed();
    }
    if (cmd.toLower() == QString("open")) {
        mainWindow_->loadFromUrl(QUrl::fromLocalFile(arg), true);
    }
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACX)
void Plugin::handleSIGUSR1(int , siginfo_t * info, void * )
{
    sigval_t val = info->si_value;
    ::usleep(1000u);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(val.sival_int);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ::connect(sock, (const sockaddr*) &addr, sizeof(addr));
    QByteArray data;
    char buf[256];
    ssize_t sz;
    while (1) {
        sz = recv(sock, buf, 256, 0);
        if (sz <= 0) {
            break;
        }
        QByteArray block(buf, sz);
        data += block;
    }
    const QString message = QString::fromUtf8(data);
    emit instance_->externalProcessCommandReceived(message);
}
#endif

#if defined(Q_OS_WIN32)
void Plugin::timerEvent(QTimerEvent *e)
{
    e->accept();
    ipcShm_->lock();
    QString command = QString::fromUtf8((char*)ipcShm_->data());
    if (command.length() > 0 ) {
        qDebug() << "Have a message in SHM : " << command;
        emit externalProcessCommandReceived(command);
    }
    memset(ipcShm_->data(), 0, ipcShm_->size());
    ipcShm_->unlock();
}
#endif

void Plugin::updateAppFontSize(const int pointSize) {
    QFont f = qApp->font();
    f.setPointSize(pointSize);
    qApp->setFont(f);
    if (mainWindow_ && mainWindow_->tabWidget_)
        mainWindow_->tabWidget_->setFont(f);
    QEvent *e = new QEvent(QEvent::ApplicationFontChange);
    Q_FOREACH(Widgets::SecondaryWindow * sw, secondaryWindows_) {
        // Secondary windows are QObject, but not QWidget class instances,
        // so they are not notified by qApp->setFont(...).
        // The should be notified manually
        qApp->sendEvent(sw, e);
    }
    KPlugin *editorPlugin = myDependency("Editor");
    editorPlugin->updateSettings(QStringList());
    delete e;
}

void Plugin::updateSettings(const QStringList & keys)
{
    if (mySettings()) {
        if ( ! mainWindow_ || ! mainWindow_->isPresentationMode()) {
            bool overrideFontSize = ! mySettings()->value(UseSystemFontSizeKey, UseSystemFontSizeDefaultValue).toBool();
            if (overrideFontSize) {
                int currentFontSize = qApp->font().pointSize();
                int customFontSize = mySettings()->value(OverrideFontSizeKey,
                        PluginManager::instance()->initialApplicationFont().pointSize()).toInt();
                if (currentFontSize!=customFontSize) {
                    updateAppFontSize(customFontSize);
                }
            }
            else {
                int currentFontSize = qApp->font().pointSize();
                int initialFontSize = PluginManager::instance()->initialApplicationFont().pointSize();
                if (currentFontSize!=initialFontSize) {
                    updateAppFontSize(initialFontSize);
                }
            }
        }
    }
    foreach (Widgets::SecondaryWindow * window, secondaryWindows_) {
        const QString prefix = window->settingsKey() + "/";
        bool hasPrefix = false;
        foreach (const QString & key, keys) {
            if (key.startsWith(prefix)) {
                hasPrefix = true;
                break;
            }
        }
        if (keys.isEmpty() || hasPrefix) {
            QStringList windowKeys;
            foreach (const QString & key , keys) {
                if (key.startsWith(prefix)) {
                    windowKeys.push_back(key);
                }
            }
            window->updateSettings(mySettings(), windowKeys);
        }
    }
    if (mainWindow_)
        mainWindow_->updateSettings(mySettings(), keys);
    if (terminal_ && (
                keys.contains(IOSettingsEditorPage::UseFixedWidthKey) ||
                keys.contains(IOSettingsEditorPage::WidthSizeKey)
            ))
    {
        terminal_->setSettings(mySettings());
    }
    if (terminal_ && plugin_editor) {
        terminal_->setTerminalFont(plugin_editor->defaultEditorFont());
    }
}

QList<QWidget*> Plugin::settingsEditorPages() {
    if (!guiSettingsPage_) {
        guiSettingsPage_ = new GUISettingsPage(mySettings(), 0);
        connect(guiSettingsPage_, SIGNAL(settingsChanged(QStringList)),
                this, SLOT(updateSettings(QStringList)));
    }
    if (!ioSettingsPage_) {
        ioSettingsPage_ = new IOSettingsEditorPage(mySettings(), 0);
        connect(ioSettingsPage_, SIGNAL(settingsChanged(QStringList)),
                this, SLOT(updateSettings(QStringList)));
    }
    return QList<QWidget*>() << guiSettingsPage_ << ioSettingsPage_;
}

void Plugin::createPluginSpec()
{
    _pluginSpec.name = "CoreGUI";
    _pluginSpec.gui = true;
    _pluginSpec.dependencies.append("Analizer");
    _pluginSpec.dependencies.append("Editor");
    _pluginSpec.dependencies.append("Runner");
}

void Plugin::showActorWindow(const QByteArray &asciiName)
{
    if (showActorActions_.contains(asciiName)) {
        showActorActions_[asciiName]->trigger();
    }
}

void Plugin::changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState state)
{
    using namespace Shared;    
    if (state==PluginInterface::GS_Unlocked) {
//        m_kumirStateLabel->setText(tr("Editing"));
        mainWindow_->clearMessage();
        mainWindow_->setFocusOnCentralWidget();
        mainWindow_->unlockActions();
        _debugger->reset();
        _debugger->setDebuggerEnabled(false);
    }
    else if (state==PluginInterface::GS_Observe) {
//        m_kumirStateLabel->setText(tr("Observe"));
        mainWindow_->showMessage(kumirProgram_->endStatusText());
        mainWindow_->setFocusOnCentralWidget();
        mainWindow_->unlockActions();
//        debugger_->setDebuggerEnabled(kumirProgram_->endStatus() == KumirProgram::Exception);
        const RunInterface::RunMode runMode = kumirProgram_->runner()->currentRunMode();
        _debugger->setDebuggerEnabled(RunInterface::RM_ToEnd != runMode);
    }
    else if (state==PluginInterface::GS_Running) {
//        m_kumirStateLabel->setText(tr("Running"));
        mainWindow_->clearMessage();
        mainWindow_->lockActions();
        _debugger->setDebuggerEnabled(false);
    }
    else if (state==PluginInterface::GS_Pause) {
//        m_kumirStateLabel->setText(tr("Pause"));
        mainWindow_->lockActions();
        const RunInterface::RunMode runMode = kumirProgram_->runner()->currentRunMode();
        _debugger->setDebuggerEnabled(RunInterface::RM_ToEnd != runMode);
    }
    else if (state==PluginInterface::GS_Input) {
//        m_kumirStateLabel->setText(tr("Pause"));
        mainWindow_->lockActions();
        const RunInterface::RunMode runMode = kumirProgram_->runner()->currentRunMode();
        _debugger->setDebuggerEnabled(RunInterface::RM_ToEnd != runMode);
    }
    kumirProgram_->switchGlobalState(old, state);
    terminal_->changeGlobalState(old, state);
    mainWindow_->statusBar_->setState(state);


}

void Plugin::prepareKumirProgramToRun()
{
    TabWidgetElement * twe = mainWindow_->currentTab();
    kumirProgram_->setEditorInstance(twe->editor());
}

bool Plugin::showWorkspaceChooseDialog()
{
    SwitchWorkspaceDialog * dialog = new
            SwitchWorkspaceDialog(ExtensionSystem::PluginManager::instance()->globalSettings());
    dialog->setMessage(sessionsDisableFlag_
                       ? SwitchWorkspaceDialog::MSG_ChangeWorkingDirectory
                       : SwitchWorkspaceDialog::MSG_ChangeWorkspace);
    dialog->setUseAlwaysHidden(sessionsDisableFlag_);
    if (dialog->exec() == QDialog::Accepted) {
        ExtensionSystem::PluginManager::instance()->
                switchToWorkspace(
                    dialog->currentWorkspace(),
                    sessionsDisableFlag_
                    )
                ;
        return true;
    }
    else {
        return false;
    }
}

void Plugin::start()
{
    bool showDialogOnStartup = ! ExtensionSystem::PluginManager::instance()->globalSettings()
            ->value(ExtensionSystem::PluginManager::SkipChooseWorkspaceKey, false).toBool();
    if (showDialogOnStartup) {
        // Prevent quit between choose dialog closed and main window shown
        qApp->setQuitOnLastWindowClosed(false);
    }
    if (!sessionsDisableFlag_ && showDialogOnStartup) {
        if (!showWorkspaceChooseDialog()) {
            ExtensionSystem::PluginManager::instance()->shutdown();
            return;
        }
    }
    else {
        ExtensionSystem::PluginManager::instance()->switchToDefaultWorkspace(sessionsDisableFlag_);
        if (sessionsDisableFlag_) {
            updateSettings(QStringList());
            restoreSession();
        }
    }
    PluginManager::instance()->switchGlobalState(PluginInterface::GS_Unlocked);
    mainWindow_->setupMenuBarContextMenu();
    mainWindow_->show();
    if (fileNameToOpenOnReady_.length() > 0) {
        mainWindow_->loadFromUrl(QUrl::fromLocalFile(fileNameToOpenOnReady_), true);
    }
}

void Plugin::stop()
{

}


void Plugin::saveSession() const
{
    if (mainWindow_->isPresentationMode())
        mainWindow_->leavePresentationMode();
    mainWindow_->saveSession();
    mainWindow_->saveSettings();
    foreach (Widgets::SecondaryWindow * secWindow, secondaryWindows_)
        secWindow->saveState();
}

void Plugin::setStartTabStyle(const QString &tabStyle)
{
#if QT_VERSION >= 0x050000
    int styleBeginImplPos = tabStyle.indexOf("{");
    if (-1 == styleBeginImplPos)
        return;
    const QString styleSelector = "QTabBar::tab:first, QTabBar::tab:only-one";
    const QString styleImpl = tabStyle.mid(styleBeginImplPos);
    const QString newCss = styleSelector + " " + styleImpl;
    const QString oldCss = mainWindow_->tabWidget_->tabBar()->styleSheet();
    mainWindow_->tabWidget_->tabBar()->setStyleSheet(oldCss + "\n" + newCss);
#endif
}

void Plugin::createStartPage()
{
    using namespace ExtensionSystem;
    using namespace Shared;

    StartpageWidgetInterface * plugin = PluginManager::instance()->findPlugin<StartpageWidgetInterface>();
    if (plugin) {
        createSpecializedStartPage(plugin);
    }
    else {
//#if QT_VERSION >= 0x050000
        createDefaultStartPage();
//#else
//        createWebKitStartPage();
//#endif
    }
}

void Plugin::createWebKitStartPage()
{
    Shared::Browser::InstanceInterface * startPage = plugin_browser->createBrowser();

    startPage->setTitleChangeHandler(mainWindow_, SLOT(updateStartPageTitle(QString, const Shared::Browser::InstanceInterface*)));

    (*startPage)["mainWindow"] = mainWindow_;

    (*startPage)["gui"] = this;

    m_browserObjects["mainWindow"] = mainWindow_;

    startPage->widget()->setProperty("uncloseable", true);

    if (mainWindow_->tabWidget_->count()==0) {
        QMenu * editMenu = new QMenu(mainWindow_->ui->menuEdit->title(), mainWindow_);
        QMenu * insertMenu = new QMenu(mainWindow_->ui->menuInsert->title(), mainWindow_);
        QAction * editNotAvailable = editMenu->addAction(mainWindow_->tr("No actions for this tab"));
        QAction * insertNotAvailable = insertMenu->addAction(mainWindow_->tr("No actions for this tab"));

        editNotAvailable->setEnabled(false);
        insertNotAvailable->setEnabled(false);

        TabWidgetElement * twe = mainWindow_->addCentralComponent(
                    tr("Start"),
                    startPage->widget(),
                    QList<QAction*>(),
                    QList<QMenu*>() << editMenu << insertMenu,
                    MainWindow::StartPage
                    );

        twe->setStartPage(startPage);
        const QString browserEntryPoint = myResourcesDir().absoluteFilePath("startpage/russian/index2.html");
        const QUrl browserEntryUrl = QUrl::fromLocalFile(browserEntryPoint);
        startPage->go(browserEntryUrl);
    }
}

void Plugin::createDefaultStartPage()
{
    QWidget * startPage = new DefaultStartPage(this, mainWindow_);
    startPage->setProperty("uncloseable", true);


    if (mainWindow_->tabWidget_->count()==0) {
        QMenu * editMenu = new QMenu(mainWindow_->ui->menuEdit->title(), mainWindow_);
        QMenu * insertMenu = new QMenu(mainWindow_->ui->menuInsert->title(), mainWindow_);
        QAction * editNotAvailable = editMenu->addAction(mainWindow_->tr("No actions for this tab"));
        QAction * insertNotAvailable = insertMenu->addAction(mainWindow_->tr("No actions for this tab"));

        editNotAvailable->setEnabled(false);
        insertNotAvailable->setEnabled(false);

        TabWidgetElement * twe = mainWindow_->addCentralComponent(
                    tr("Start"),
                    startPage,
                    QList<QAction*>(),
                    QList<QMenu*>() << editMenu << insertMenu,
                    MainWindow::StartPage
                    );

        twe->setStartPage(qobject_cast<Shared::StartpageWidgetInterface*>(startPage));
        const QString tabStyle = qobject_cast<Shared::StartpageWidgetInterface*>(startPage)->startPageTabStyle();
        if (tabStyle.length() > 0) {
            setStartTabStyle(tabStyle);
        }
    }
    mainWindow_->setTitleForTab(0);
}

void Plugin::createSpecializedStartPage(Shared::StartpageWidgetInterface * plugin)
{
    plugin->setStartPageTitleChangeHandler(mainWindow_, SLOT(updateStartPageTitle(QString,const Shared::Browser::InstanceInterface*)));
    QWidget * widget = plugin->startPageWidget();
    const QString title = plugin->startPageTitle();
    widget->setProperty("uncloseable", true);
    if (mainWindow_->tabWidget_->count()==0) {

        const QMenu * mainWindowEditMenu = mainWindow_->ui->menuEdit;
        const QString menuEditTitle = mainWindowEditMenu->title();
        QMenu * editMenu = new QMenu(menuEditTitle, mainWindow_);
        QAction * editNotAvailable = editMenu->addAction(mainWindow_->tr("No actions for this tab"));
        editNotAvailable->setEnabled(false);

        const QMenu * mainWindowInsertMenu = mainWindow_->ui->menuInsert;
        QList<QMenu*> menus = QList<QMenu*>() << editMenu;

        if (mainWindowInsertMenu) {
            const QString menuInsertTitle = mainWindowInsertMenu->title();
            QMenu * insertMenu = new QMenu(menuInsertTitle, mainWindow_);
            QAction * insertNotAvailable = insertMenu->addAction(mainWindow_->tr("No actions for this tab"));
            insertNotAvailable->setEnabled(false);
            menus << insertMenu;
        }

        TabWidgetElement * twe = mainWindow_->addCentralComponent(
                    title,
                    widget,
                    QList<QAction*>(),
                    menus,
                    MainWindow::StartPage
                    );

        twe->setStartPage(plugin);
    }
    mainWindow_->setTitleForTab(0);
}


void Plugin::restoreSession()
{
    if (!sessionsDisableFlag_) {
    }
    else if (mainWindow_->tabWidget_->count() > 0) {
        mainWindow_->tabWidget_->setCurrentIndex(0);
        mainWindow_->setTitleForTab(0);
        mainWindow_->setFocusOnCentralWidget();
    }
    else {
        mainWindow_->newProgram();
    }
    foreach (Widgets::SecondaryWindow * secWindow, secondaryWindows_)
        secWindow->restoreState();
}

Plugin::~Plugin()
{

}


QObject *Plugin::mainWindowObject()
{
    return mainWindow_;
}

QObject *Plugin::pluginObject()
{
    return this;
}


void Plugin::setProgramSource(const ProgramSourceText &source)
{
    if (mainWindow_) {
        mainWindow_->loadFromCourseManager(source);
    }
}

GuiInterface::ProgramSourceText Plugin::programSource() const
{
    return mainWindow_->courseManagerProgramSource();
}

void Plugin::startTesting()
{
    kumirProgram_->setCourseManagerRequest();
    kumirProgram_->testingRun();
}

void Plugin::showCoursesWindow(const QString & id)
{
    if (courseManager_ && !id.isEmpty()) {
        courseManager_->activateCourseFromList(id);
    }
    if (coursesWindow_) {
        coursesWindow_->activate();
    }
}

QStringList Plugin::coursesList(bool fullPaths) const
{
    const QStringList files = courseManager_->getListOfCourses();
    if (fullPaths)
        return files;
    else {
        QStringList result;
        for (int i=0; i<files.size(); i++) {
            result << QFileInfo(files[i]).fileName();
        }
        return result;
    }
}

void Plugin::showHelpWindow(int index)
{
    if (helpWindow_) {
        helpWindow_->activate();
    }
    if (helpViewer_) {
        helpViewer_->activateBookIndex(index);
    }
}

void Plugin::showHelpWindowFromQuickReference(const int topicType, const QString &name)
{
    if (helpWindow_) {
        helpWindow_->activate();
    }
    if (helpViewer_) {
        helpViewer_->navigateFromQuickReference(topicType, name);
    }
}

QStringList Plugin::helpList() const
{
    return helpViewer_ ? helpViewer_->booksList() : QStringList();
}

int Plugin::overridenEditorFontSize() const
{
    if (mainWindow_ && mainWindow_->isPresentationMode() && mySettings()) {
        return mySettings()->value(PresentationModeEditorFontSizeKey,
                                   PresentationModeEditorFontSizeDefaultValue).toInt();
    }
    else {
        return 0;
    }
}

QString Plugin::wsName() const
{
#ifdef Q_WS_X11
    return "x11";
#endif
#ifdef Q_WS_WIN32
    return "win32";
#endif
#ifdef Q_WS_MAC
    return "mac";
#endif
    return "";
}

QString Plugin::applicationVersionString() const
{
    return qApp->applicationVersion();
}


} // namespace CoreGUI

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(CoreGui, CoreGUI::Plugin)
#endif
