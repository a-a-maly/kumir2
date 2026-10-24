/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

// Self include
#include "grasshoppermodule.h"
#include "pult.h"
#include "kuznec.h"
#include <QLayout>
#include <QAction>

namespace ActorGrasshopper
{

class AAA : public QWidget
{
public:
	inline explicit AAA(QWidget *pult): QWidget(), pult_(pult)
	{
		setLayout(new QVBoxLayout);
		layout()->setContentsMargins(0, 0, 0, 0);
		layout()->addWidget(pult);
	}
	inline QSize minimumSizeHint() const
	{
		return pult_->minimumSizeHint();
	}
private:
	QWidget *pult_;
};

GrasshopperModule::GrasshopperModule(ExtensionSystem::KPlugin *parent)
	: GrasshopperModuleBase(parent)
{

	kuznec = nullptr;

	// Module constructor, called once on plugin load
	// TODO implement me
}

void GrasshopperModule::createGui()
{
	kuznec = new KumKuznec(myResourcesDir());
	kuznec->myModule = this;
	connect(m_actionGrasshopperNewEnvironment, SIGNAL(triggered()), kuznec, SLOT(ClearPicture()));
	connect(m_actionGrasshopperLoadEnvironment, SIGNAL(triggered()), kuznec, SLOT(LoadFromFileActivated()));
	connect(m_actionGrasshopperSave, SIGNAL(triggered()), kuznec, SLOT(SaveToFileActivated()));
	reloadSettings(mySettings(), QStringList());
}

QString GrasshopperModule::initialize(const QStringList &configurationParameters, const ExtensionSystem::CommandLine &)
{
	if (!configurationParameters.contains("tablesOnly")) {
		createGui();
	}
	return "";
}

QVariantList GrasshopperModule::templateParameters() const
{
	QVariantList jumps;
	jumps.append(kuznec->Fstep);
	jumps.append(kuznec->Bstep);
	return jumps;

};

/* public static */ QList<ExtensionSystem::CommandLineParameter> GrasshopperModule::acceptableCommandLineParameters()
{
	// See "src/shared/extensionsystem/commandlineparameter.h" for constructor details
	return QList<ExtensionSystem::CommandLineParameter>();
}

/* public slot */ void GrasshopperModule::changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current)
{
	// Called when changed kumir state. The states are defined as enum ExtensionSystem::GlobalState:
	/*
	namespace ExtensionSystem {
	    enum GlobalState {
	        GS_Unlocked, // Edit mode
	        GS_Observe, // Observe mode
	        GS_Running, // Running mode
	        GS_Input,  // User input required
	        GS_Pause  // Running paused
	    };
	}
	*/
	// TODO implement me
	using namespace ExtensionSystem;  // not to write "ExtensionSystem::" each time in this method scope
	if (old == Shared::PluginInterface::GS_Running && current == Shared::PluginInterface::GS_Observe) {
		kuznec->FindKuznec();
	}

	Q_UNUSED(old);  // Remove this line on implementation
	Q_UNUSED(current);  // Remove this line on implementation
}

/* public slot */ void GrasshopperModule::loadActorData(QIODevice *source)
{
	// Set actor specific data (like environment)
	// The source should be ready-to-read QIODevice like QBuffer or QFile
	Q_UNUSED(source);  // By default do nothing

}

/* public */ QWidget *GrasshopperModule::mainWidget() const
{
	if (!kuznec) {
		return 0;
	}
	// Returns module main view widget, or nullptr if there is no any views
	// NOTE: the method is const and might be called at any time,
	//       so DO NOT create widget here, just return!
	// TODO implement me
	return kuznec->mainWindow();
}
void GrasshopperModule::jumpsChanged()
{
	emit notifyOnTemplateParametersChanged();
};
/* public */ QWidget *GrasshopperModule::pultWidget() const
{
	if (!kuznec) {
		return 0;
	}
	// Returns module control view widget, or nullptr if there is no control view
	// NOTE: the method is const and might be called at any time,
	//       so DO NOT create widget here, just return!
	// TODO implement me
	static QWidget *dummy = new AAA(kuznec->Kpult);
	return dummy;

}

/* public slot */ void GrasshopperModule::reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList &keys)
{
	// Updates setting on module load, workspace change or appliyng settings dialog.
	// If @param keys is empty -- should reload all settings, otherwise load only setting specified by @param keys
	Q_UNUSED(keys);
	if (!kuznec) {
		return;
	}
	kuznec->setColors(
		QColor(settings->value("Colors/Background", "#90ee90").toString()),
		QColor(settings->value("Colors/Ruler", "#0000ff").toString()),
		QColor(settings->value("Colors/Grasshopper", "#0000ff").toString()),
		QColor(settings->value("Colors/Track", "#000000").toString())
	);
}

/* public slot */ void GrasshopperModule::reset()
{
	// Resets module to initial state before program execution
	// TODO implement me
	kuznec->Reset();
}

/* public slot */ void GrasshopperModule::setAnimationEnabled(bool enabled)
{
	// Sets GUI animation flag on run
	// NOTE this method just setups a flag and might be called anytime, even module not needed
	// TODO implement me
	Q_UNUSED(enabled);  // Remove this line on implementation
	// kuznec->Reset();
}

/* public slot */ void GrasshopperModule::runGoForward(const int distance)
{
	/* алг вперед \1(цел distance) */
	// TODO implement me
	//Q_UNUSED(distance)  // Remove this line on implementation;
	kuznec->setFwdStep(distance);
	kuznec->MoveFwd();
	qDebug() << "GrasshopperModule::runGoForward(" << distance << ")";
}

/* public slot */ void GrasshopperModule::runGoBackward(const int distance)
{
	/* алг назад \2(цел distance) */
	// TODO implement me
	Q_UNUSED(distance)  // Remove this line on implementation;
	kuznec->setBackStep(distance);
	kuznec->MoveBack();
	qDebug() << "GrasshopperModule::runGoBackward(" << distance << ")";
}

/* public slot */ void GrasshopperModule::runDoRepaint()
{
	/* алг перекрасить */
	// TODO implement me
	kuznec->ColorUncolor();

}

} // namespace ActorGrasshopper
