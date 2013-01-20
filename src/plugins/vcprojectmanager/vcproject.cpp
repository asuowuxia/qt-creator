#include "vcproject.h"

#include "vcprojectfile.h"
#include "vcprojectnodes.h"
#include "vcmakestep.h"
#include "vcprojectmanager.h"
#include "vcprojectmanagerconstants.h"
#include "vcprojectreader.h"
#include "vcprojectbuildconfiguration.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesystemwatcher.h>

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QLabel>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

VcProject::VcProject(VcManager *projectManager, const QString &projectFilePath)
    : m_projectManager(projectManager)
    , m_projectFile(new VcProjectFile(projectFilePath))
    , m_rootNode(new VcProjectNode(projectFilePath))
    , m_projectFileWatcher(new QFileSystemWatcher(this))
{
    setProjectContext(Core::Context(Constants::VC_PROJECT_CONTEXT));
    setProjectLanguage(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    reparse();

    m_projectFileWatcher->addPath(projectFilePath);
    connect(m_projectFileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(reparse()));
}

QString VcProject::displayName() const
{
    return m_rootNode->displayName();
}

Core::Id VcProject::id() const
{
    return Core::Id(Constants::VC_PROJECT_ID);
}

Core::IDocument *VcProject::document() const
{
    return m_projectFile;
}

IProjectManager *VcProject::projectManager() const
{
    return m_projectManager;
}

ProjectNode *VcProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList VcProject::files(Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    // TODO: respect the mode
    return m_rootNode->files();
}

QList<BuildConfigWidget *> VcProject::subConfigWidgets()
{
    QList<ProjectExplorer::BuildConfigWidget*> list;
    list << new BuildEnvironmentWidget;
    return list;
}

QString VcProject::defaultBuildDirectory() const
{
    VcProjectFile* vcFile = static_cast<VcProjectFile *>(document());
    return vcFile->path() + QLatin1String("-build");
}

void VcProject::reparse()
{
    QString projectFilePath = m_projectFile->filePath();
    VcProjectInfo::Project *projInfo = reader.parse(projectFilePath);

    // If file saving is done by replacing the file with the new file
    // watcher will loose it from its list
    if (m_projectFileWatcher->files().isEmpty())
        m_projectFileWatcher->addPath(projectFilePath);

    if (!projInfo) {
        m_rootNode->setDisplayName(QFileInfo(projectFilePath).fileName());
        m_rootNode->refresh(0);
    } else {
        m_rootNode->setDisplayName(projInfo->displayName);
        m_rootNode->refresh(projInfo->files);
    }

    delete projInfo;

    // TODO: can we (and is is important to) detect when the list really changed?
    emit fileListChanged();
}

bool VcProject::fromMap(const QVariantMap &map)
{
    loadBuildConfigurations();
    return Project::fromMap(map);
}

void VcProject::loadBuildConfigurations()
{
    Kit *defaultKit = KitManager::instance()->defaultKit();
    if (defaultKit) {

        // Note(Radovan): When targets are available from .vcproj or .vcproj.user
        // file create Target object for each of them

        Target *target = new Target(this, defaultKit);

        // Debug build configuration
        VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(target);
        bc->setDefaultDisplayName(tr("Debug"));

        // build step
        ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        VcMakeStep *makeStep = new VcMakeStep(buildSteps);
        QLatin1String argument("/p:configuration=\"Debug\"");
        makeStep->addBuildArgument(argument);
        buildSteps->insertStep(0, makeStep);

        //clean step
        ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        makeStep = new VcMakeStep(cleanSteps);
        argument = QLatin1String("/p:configuration=\"Debug\" /t:Clean");
        makeStep->addBuildArgument(argument);
        cleanSteps->insertStep(0, makeStep);

        target->addBuildConfiguration(bc);

        // Release build configuration
        bc = new VcProjectBuildConfiguration(target);
        bc->setDefaultDisplayName("Release");

        // build step
        buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        makeStep = new VcMakeStep(buildSteps);
        argument = QLatin1String("/p:configuration=\"Release\"");
        makeStep->addBuildArgument(argument);
        buildSteps->insertStep(0, makeStep);

        //clean step
        cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        makeStep = new VcMakeStep(cleanSteps);
        argument = QLatin1String("/p:configuration=\"Release\" /t:Clean");
        makeStep->addBuildArgument(argument);
        cleanSteps->insertStep(0, makeStep);

        target->addBuildConfiguration(bc);
        addTarget(target);    }
}

bool VcProject::setupTarget(ProjectExplorer::Target *t)
{
    VcProjectBuildConfigurationFactory *factory
            = ExtensionSystem::PluginManager::instance()->getObject<VcProjectBuildConfigurationFactory>();
    VcProjectBuildConfiguration *bc = factory->create(t, Constants::VC_PROJECT_BC_ID, QLatin1String("vcproj"));
    if (!bc)
        return false;

    t->addBuildConfiguration(bc);
    return true;
}

VcProjectBuildSettingsWidget::VcProjectBuildSettingsWidget()
{
    QFormLayout *f1 = new QFormLayout(this);
    f1->setContentsMargins(0, 0, 0, 0);
    f1->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QLabel *l = new QLabel(tr("Vcproj Build Configuration widget."));
    f1->addRow(tr("Vc Project"), l);
}

QString VcProjectBuildSettingsWidget::displayName() const
{
    return tr("Vc Project Settings Widget");
}

void VcProjectBuildSettingsWidget::init(BuildConfiguration *bc)
{
    Q_UNUSED(bc);
}

} // namespace Internal
} // namespace VcProjectManager
