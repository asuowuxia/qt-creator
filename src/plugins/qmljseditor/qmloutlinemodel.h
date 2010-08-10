#ifndef QMLOUTLINEMODEL_H
#define QMLOUTLINEMODEL_H

#include "qmljseditor.h"
#include <utils/changeset.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsicons.h>
#include <qmljs/qmljslookupcontext.h>

#include <QStandardItemModel>

namespace QmlJS {
namespace Interpreter {
class Value;
class Context;
}
}

namespace QmlJSEditor {
namespace Internal {

class QmlOutlineModel;

class QmlOutlineItem : public QStandardItem
{
public:
    QmlOutlineItem(QmlOutlineModel *model);

    //QStandardItem
    QVariant data(int role = Qt::UserRole + 1) const;
    int type() const;

    QString annotation() const;
    void setAnnotation(const QString &id);

    QmlJS::AST::SourceLocation sourceLocation() const;
    void setSourceLocation(const QmlJS::AST::SourceLocation &location);


    QmlJS::AST::Node *node() const;
    void setNode(QmlJS::AST::Node *node);

    QmlJS::AST::UiQualifiedId *idNode() const;
    void setIdNode(QmlJS::AST::UiQualifiedId *idNode);

    QmlOutlineItem &copyValues(const QmlOutlineItem &other); // so that we can assign all values at onc

private:
    QString prettyPrint(const QmlJS::Interpreter::Value *value, QmlJS::Interpreter::Context *context) const;

    QmlOutlineModel *m_outlineModel;
    QmlJS::AST::Node *m_node;
    QmlJS::AST::UiQualifiedId *m_idNode;
};


class QmlOutlineModel : public QStandardItemModel
{
    Q_OBJECT
public:

    enum CustomRoles {
        SourceLocationRole = Qt::UserRole + 1,
        ItemTypeRole,
        AnnotationRole
    };

    enum ItemTypes {
        ElementType,
        PropertyType
    };

    QmlOutlineModel(QObject *parent = 0);

    // QStandardItemModel
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex  &parent);

    QmlJS::Document::Ptr document() const;
    void update(const SemanticInfo &semanticInfo);
    QmlJS::AST::Node *nodeForIndex(const QModelIndex &index);

signals:
    void updated();

private:
    QModelIndex enterObjectDefinition(QmlJS::AST::UiObjectDefinition *objectDefinition);
    void leaveObjectDefiniton();

    QModelIndex enterScriptBinding(QmlJS::AST::UiScriptBinding *scriptBinding);
    void leaveScriptBinding();

    QModelIndex enterPublicMember(QmlJS::AST::UiPublicMember *publicMember);
    void leavePublicMember();

private:
    QModelIndex enterNode(const QmlOutlineItem &prototype);
    void leaveNode();

    void reparentNodes(QmlOutlineItem *targetItem, int targetRow, QList<QmlOutlineItem*> itemsToMove);
    void moveObjectMember(QmlJS::AST::UiObjectMember *toMove, QmlJS::AST::UiObjectMember *newParent,
                          bool insertionOrderSpecified, QmlJS::AST::UiObjectMember *insertAfter,
                          Utils::ChangeSet *changeSet, Utils::ChangeSet::Range *addedRange);

    QStandardItem *parentItem();

    static QString asString(QmlJS::AST::UiQualifiedId *id);
    static QmlJS::AST::SourceLocation getLocation(QmlJS::AST::UiObjectMember *objMember);
    QIcon getIcon(QmlJS::AST::UiObjectDefinition *objDef);
    static QString getId(QmlJS::AST::UiObjectDefinition *objDef);


    SemanticInfo m_semanticInfo;
    QList<int> m_treePos;
    QStandardItem *m_currentItem;
    QmlJS::Icons *m_icons;

    QmlJS::LookupContext::Ptr m_context;
    QHash<QString, QIcon> m_typeToIcon;

    friend class QmlOutlineModelSync;
    friend class QmlOutlineItem;
};

} // namespace Internal
} // namespace QmlJSEditor

Q_DECLARE_METATYPE(QmlJS::AST::SourceLocation);

#endif // QMLOUTLINEMODEL_H
