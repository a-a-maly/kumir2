#include "quickreferencewidget.h"
#include "kumiranalizerplugin.h"
#include <kumir2-libs/widgets/flowlayout.h>
#include "ui_quickreferencewidget.h"
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2-libs/extensionsystem/kplugin.h>
#include <kumir2/actorinterface.h>
#include "analizer.h"

#define DO_NOT_DECLARE_STATIC
#include "stdlibmodules.h"
#undef DO_NOT_DECLARE_STATIC

#include <QMouseEvent>
#include <QResizeEvent>
#include <QPushButton>
#include <QDebug>

namespace KumirAnalizer
{

QuickReferenceWidget::QuickReferenceWidget(KumirAnalizerPlugin *plugin)
	: QWidget(0)
	, _plugin(plugin)
	, ui(new Ui::QuickReferenceWidget)
{
	this->setObjectName("quickReferenceWidget");
	ui->setupUi(this);


	ui->statementsContents->setLayout(new FlowLayout);
	ui->typesContents->setLayout(new FlowLayout);
	ui->functionsContents->setLayout(new FlowLayout);

	_builtInModules.append(new StdLibModules::RTL);
		_builtInModules.append(new StdLibModules::Files);
	if (Analizer::isModuleAlwaysEnabled(QString::fromUtf8("Строки"))) {
		_builtInModules.append(new StdLibModules::Strings);
	}

	createKeywordsList();
	createTypesList();
	createFunctionsList();

	installEventFilter(this);

	relayoutBlocks(this->width());
	reloadStyleSheet();
}

void QuickReferenceWidget::handleButtonPressed()
{
	QPushButton *btn = qobject_cast<QPushButton *>(sender());
	const QString name = btn->text();

	QWidget const *group = btn->parentWidget();
	int topicNumber = -1;
	if (group == ui->statementsContents) {
		topicNumber = StatementTopic;
	} else if (group == ui->typesContents) {
		topicNumber = TypeTopic;
	} else if (group == ui->functionsContents) {
		topicNumber = FunctionTopic;
	}

	Q_EMIT openTopicInDocumentation(topicNumber, name);
}


void QuickReferenceWidget::relayoutBlocks(int width)
{
	Q_UNUSED(width);
}

bool QuickReferenceWidget::eventFilter(QObject *obj, QEvent *evt)
{
	if (obj == this && QEvent::MouseButtonPress == evt->type()) {
		QMouseEvent *event = static_cast<QMouseEvent *>(evt);
		if (Qt::RightButton == event->button()) {
#ifndef QT_NO_DEBUG
			reloadStyleSheet();
			qDebug() << "Reload style sheet";
#endif
		}
	} else if (obj == this && QEvent::Resize == evt->type()) {
		QResizeEvent *event = static_cast<QResizeEvent *>(evt);
		relayoutBlocks(event->size().width());
	}
	return false;
}

void QuickReferenceWidget::reloadStyleSheet()
{
	const QString cssFileName = _plugin->myResourcesDir().absoluteFilePath("quick_reference_page.css");
	QFile cssFile(cssFileName);
	if (cssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		const QString cssData = QString::fromUtf8(cssFile.readAll());
		cssFile.close();
		setStyleSheet(cssData);
	}
}

void QuickReferenceWidget::createKeywordsList()
{
	static const char *items =
		"использовать алг нач кон дано надо утв "
		"нц кц кц_при для раз пока от до шаг выход "
		"если то иначе все выбор при ввод вывод нс "
		"не и или рез арг аргрез знач"
		;
	static const QStringList itemsList = QString::fromUtf8(items).split(" ");
	Q_FOREACH (const QString &item, itemsList) {
		QPushButton *btn = new QPushButton(ui->statementsContents);
		connect(btn, SIGNAL(clicked()), this, SLOT(handleButtonPressed()));
		btn->setText(item);
		btn->setCursor(Qt::PointingHandCursor);
		ui->statementsContents->layout()->addWidget(btn);
	}
}

void QuickReferenceWidget::createTypesList()
{
	static const char *BasicTypes =
		"цел вещ лог сим лит"
		;
	static const QStringList BasicTypesList = QString::fromUtf8(BasicTypes).split(" ");
	QStringList itemsList;
	Q_FOREACH (const QString &item, BasicTypesList) {
		itemsList.append(item);
	}

	Q_FOREACH (Shared::ActorInterface *actor, _builtInModules) {
		Q_FOREACH (Shared::ActorInterface::RecordSpecification type, actor->typeList()) {
			const QString typeName =
				type.localizedNames.contains(QLocale::Russian)
				? type.localizedNames[QLocale::Russian]
				: QString::fromLatin1(type.asciiName);
			itemsList.append(typeName);
		}
	}

	Q_FOREACH (const QString &item, itemsList) {
		QPushButton *btn = new QPushButton(ui->typesContents);
		connect(btn, SIGNAL(clicked()), this, SLOT(handleButtonPressed()));
		btn->setText(item);
		btn->setCursor(Qt::PointingHandCursor);
		ui->typesContents->layout()->addWidget(btn);
	}
}

void QuickReferenceWidget::createFunctionsList()
{
	QStringList itemsList;

	Q_FOREACH (Shared::ActorInterface *actor, _builtInModules) {
		Q_FOREACH (Shared::ActorInterface::Function func, actor->functionList()) {
			const QString funcName =
				func.localizedNames.contains(QLocale::Russian)
				? func.localizedNames[QLocale::Russian]
				: QString::fromLatin1(func.asciiName);
			if (!funcName.startsWith("@") && !funcName.startsWith("_")) {
				itemsList.append(funcName);
			}
		}
	}

	itemsList.sort();

	Q_FOREACH (const QString &item, itemsList) {
		QPushButton *btn = new QPushButton(ui->functionsContents);
		connect(btn, SIGNAL(clicked()), this, SLOT(handleButtonPressed()));
		btn->setText(item);
		btn->setCursor(Qt::PointingHandCursor);
		ui->functionsContents->layout()->addWidget(btn);
	}
}

} // namespace KumirAnalizer
