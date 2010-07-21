#include "qmljsoutline.h"
#include "qmloutlinemodel.h"

#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QAction>
#include <QtGui/QVBoxLayout>

using namespace QmlJS;

enum {
    debug = false
};

namespace QmlJSEditor {
namespace Internal {

QmlJSOutlineTreeView::QmlJSOutlineTreeView(QWidget *parent) :
    Utils::NavigationTreeView(parent)
{
    // see also CppOutlineTreeView
    setFocusPolicy(Qt::NoFocus);
    setExpandsOnDoubleClick(false);

    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(InternalMove);
}

QmlJSOutlineFilterModel::QmlJSOutlineFilterModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

bool QmlJSOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                               const QModelIndex &sourceParent) const
{
    if (m_filterBindings) {
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        QVariant itemType = sourceIndex.data(QmlOutlineModel::ItemTypeRole);
        Q_ASSERT(itemType.isValid());
        if (itemType.isValid() && itemType == QmlOutlineModel::PropertyType) {
            return false;
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

bool QmlJSOutlineFilterModel::filterBindings() const
{
    return m_filterBindings;
}

void QmlJSOutlineFilterModel::setFilterBindings(bool filterBindings)
{
    m_filterBindings = filterBindings;
    invalidateFilter();
}

QmlJSOutlineWidget::QmlJSOutlineWidget(QWidget *parent) :
    TextEditor::IOutlineWidget(parent),
    m_treeView(new QmlJSOutlineTreeView(this)),
    m_filterModel(new QmlJSOutlineFilterModel(this)),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    m_filterModel->setFilterBindings(false);

    m_treeView->setModel(m_filterModel);

    QVBoxLayout *layout = new QVBoxLayout;

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);

    m_showBindingsAction = new QAction(this);
    m_showBindingsAction->setText(tr("Show bindings"));
    m_showBindingsAction->setCheckable(true);
    m_showBindingsAction->setChecked(true);
    connect(m_showBindingsAction, SIGNAL(toggled(bool)), this, SLOT(setShowBindings(bool)));

    setLayout(layout);
}

void QmlJSOutlineWidget::setEditor(QmlJSTextEditor *editor)
{
    m_editor = editor;

    m_filterModel->setSourceModel(m_editor.data()->outlineModel());
    modelUpdated();

    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));

    connect(m_editor.data(), SIGNAL(outlineModelIndexChanged(QModelIndex)),
            this, SLOT(updateSelectionInTree(QModelIndex)));
    connect(m_editor.data()->outlineModel(), SIGNAL(updated()),
            this, SLOT(modelUpdated()));
}

QList<QAction*> QmlJSOutlineWidget::filterMenuActions() const
{
    QList<QAction*> list;
    list.append(m_showBindingsAction);
    return list;
}

void QmlJSOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree(m_editor.data()->outlineModelIndex());
}

void QmlJSOutlineWidget::restoreSettings(int position)
{
    QSettings *settings = Core::ICore::instance()->settings();
    bool showBindings = settings->value("QmlJSOutline."+QString::number(position)+".ShowBindings", true).toBool();
    m_showBindingsAction->setChecked(showBindings);
}

void QmlJSOutlineWidget::saveSettings(int position)
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->setValue("QmlJSOutline."+QString::number(position)+".ShowBindings",
                       m_showBindingsAction->isChecked());
}

void QmlJSOutlineWidget::modelUpdated()
{
    m_treeView->expandAll();
}

void QmlJSOutlineWidget::updateSelectionInTree(const QModelIndex &index)
{
    if (!syncCursor())
        return;

    m_blockCursorSync = true;
    m_treeView->selectionModel()->select(m_filterModel->mapFromSource(index), QItemSelectionModel::ClearAndSelect);
    m_treeView->scrollTo(index);
    m_blockCursorSync = false;
}

void QmlJSOutlineWidget::updateSelectionInText(const QItemSelection &selection)
{
    if (!syncCursor())
        return;


    if (!selection.indexes().isEmpty()) {
        QModelIndex index = selection.indexes().first();
        AST::SourceLocation location = index.data(QmlOutlineModel::SourceLocationRole).value<AST::SourceLocation>();

        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->cutForwardNavigationHistory();
        editorManager->addCurrentPositionToNavigationHistory();

        QTextCursor textCursor = m_editor.data()->textCursor();
        m_blockCursorSync = true;
        textCursor.setPosition(location.offset);
        m_editor.data()->setTextCursor(textCursor);
        m_blockCursorSync = false;
    }
}

void QmlJSOutlineWidget::setShowBindings(bool showBindings)
{
    m_filterModel->setFilterBindings(!showBindings);
    modelUpdated();
}

bool QmlJSOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool QmlJSOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (qobject_cast<QmlJSEditorEditable*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget *QmlJSOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    QmlJSOutlineWidget *widget = new QmlJSOutlineWidget;

    QmlJSEditorEditable *qmlJSEditable = qobject_cast<QmlJSEditorEditable*>(editor);
    QmlJSTextEditor *qmlJSEditor = qobject_cast<QmlJSTextEditor*>(qmlJSEditable->widget());
    Q_ASSERT(qmlJSEditor);

    widget->setEditor(qmlJSEditor);

    return widget;
}

} // namespace Internal
} // namespace QmlJSEditor
