#include "FrmGroupChat.h"
#include "ui_FrmGroupChat.h"
#include "../../Global.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppUtils.h"
#include <QMessageBox>

CFrmGroupChat::CFrmGroupChat(QXmppMucRoom *room, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::CFrmGroupChat)
{
    ui->setupUi(this);
    Q_ASSERT(room);

    m_pRoom = room;
    m_pItem = NULL;

    ui->lstMembers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pModel = new QStandardItemModel(this);//这里不会产生内在泄漏，控件在romve操作时会自己释放内存。  
    if(m_pModel)
    {
        ui->lstMembers->setModel(m_pModel);
    }
    
    bool check = connect(m_pRoom, SIGNAL(joined()),
                         SLOT(slotJoined()));
    Q_ASSERT(check);

    check = connect(m_pRoom, SIGNAL(nameChanged(QString)), 
                    SLOT(slotNameChanged(QString)));
    Q_ASSERT(check);

    check = connect(m_pRoom, SIGNAL(subjectChanged(QString)),
                    SLOT(slotSubjectChanged(QString)));
    Q_ASSERT(check);

    check = connect(m_pRoom, SIGNAL(messageReceived(QXmppMessage)),
                    SLOT(slotMessageReceived(QXmppMessage)));
    Q_ASSERT(check);

    check = connect(m_pRoom, SIGNAL(participantAdded(QString)),
                    SLOT(slotParticipantAdded(QString)));
    Q_ASSERT(check);

    check = connect(m_pRoom, SIGNAL(participantChanged(QString)),
                    SLOT(slotParticipantChanged(QString)));
    Q_ASSERT(check);

    check = connect(m_pRoom, SIGNAL(participantRemoved(QString)),
                    SLOT(slotParticipantRemoved(QString)));
    Q_ASSERT(check);

    check = connect(m_pRoom, SIGNAL(permissionsReceived(QList<QXmppMucItem>)),
                    SLOT(slotPermissionsReceived(QList<QXmppMucItem>)));
    Q_ASSERT(check);
}

CFrmGroupChat::~CFrmGroupChat()
{
    delete ui;
}

QStandardItem* CFrmGroupChat::GetItem()
{
    if(NULL == m_pItem)
    {
        m_pItem = new QStandardItem(QIcon(":/icon/Conference"), m_pRoom->name());//这个由QTreeView释放内存  
    }
    else
    {
        LOG_MODEL_ERROR("Group chat", "m_pItem is exist");
        return m_pItem;
    }

    if(m_pItem)
    {
        m_pItem->setData(m_pRoom->jid(), ROLE_GROUPCHAT_JID);
        QVariant v;
        v.setValue(this);
        m_pItem->setData(v, ROLE_GROUPCHAT_OBJECT);
        m_pItem->setToolTip(m_pRoom->jid());
    }
    return m_pItem;
}

void CFrmGroupChat::slotJoined()
{
}

void CFrmGroupChat::slotNameChanged(const QString &name)
{
    m_pItem->setData(name, Qt::DisplayRole);
    ChangeTitle();
}

void CFrmGroupChat::slotAllowedActionsChanged(QXmppMucRoom::Actions actions) const
{
    LOG_MODEL_DEBUG("Group chat", "actions:%d", actions);
}

void CFrmGroupChat::slotConfigurationReceived(const QXmppDataForm &configuration)
{
    LOG_MODEL_DEBUG("Group chat", "CFrmGroupChat::slotConfigurationReceived");
}

void CFrmGroupChat::slotSubjectChanged(const QString &subject)
{
    Q_UNUSED(subject);
    ChangeTitle();
}

void CFrmGroupChat::slotMessageReceived(const QXmppMessage &message)
{
    LOG_MODEL_DEBUG("Group chat", "CFrmGroupChat::slotMessageReceived:type:%d;state:%d;from:%s;to:%s;body:%s",
           message.type(),
           message.state(), //消息的状态 0:消息内容，其它值表示这个消息的状态  
           qPrintable(message.from()),
           qPrintable(message.to()),
           qPrintable(message.body())
          );

    if(QXmppUtils::jidToBareJid(message.from()) != QXmppUtils::jidToBareJid(m_pRoom->jid()))
    {
        LOG_MODEL_DEBUG("Group chat", "the room is %s, from %s received", m_pRoom->jid(), message.from());
        return;
    }

    if(message.body().isEmpty())
        return;

    QString nick;
    nick = QXmppUtils::jidToResource(message.from());
    AppendMessageToList(message.body(), nick);
}

void CFrmGroupChat::slotParticipantAdded(const QString &jid)
{
    LOG_MODEL_DEBUG("Group chat", "CFrmGroupChat::slotParticipantAdded:jid:%s", qPrintable(jid));
    QStandardItem* pItem = new QStandardItem(QXmppUtils::jidToResource(jid));
    m_pModel->appendRow(pItem);
}

void CFrmGroupChat::slotParticipantChanged(const QString &jid)
{
    LOG_MODEL_DEBUG("Group chat", "CFrmGroupChat::slotParticipantChanged:jid:%s", qPrintable(jid));
}

void CFrmGroupChat::slotParticipantRemoved(const QString &jid)
{
    LOG_MODEL_DEBUG("Group chat", "CFrmGroupChat::slotParticipantRemoved:jid:%s", qPrintable(jid));
    QList<QStandardItem*> items = m_pModel->findItems(QXmppUtils::jidToResource(jid));
    QStandardItem* item;
    foreach(item, items)
    {
        m_pModel->removeRow(item->row());
    }
}

void CFrmGroupChat::slotPermissionsReceived(const QList<QXmppMucItem> &permissions)
{
    QXmppMucItem item;
    foreach(item, permissions)
    {
        LOG_MODEL_DEBUG("Group chat", "jid:%s;nick:%s;actor:%s;affiliation:%s;role:%s",
                        item.jid().toStdString().c_str(),
                        item.nick().toStdString().c_str(),
                        item.actor().toStdString().c_str(),
                        QXmppMucItem::affiliationToString(item.affiliation()).toStdString().c_str(),
                        QXmppMucItem::roleToString(item.role()).toStdString().c_str());
    }
}

void CFrmGroupChat::on_pbSend_clicked()
{
    QString msg = ui->txtInput->toPlainText();
    if(msg.isEmpty())
    {
        QMessageBox::warning(this, tr("Warning"), tr("There is empty, please input."));
        return;
    }

    //AppendMessageToList(msg, m_pRoom->nickName());//服务器会转发，所以不需要  
    m_pRoom->sendMessage(msg);
    ui->txtInput->clear();//清空输入框中的内容  
}

void CFrmGroupChat::on_pbClose_clicked()
{
    close();
}

int CFrmGroupChat::AppendMessageToList(const QString &szMessage, const QString &nick)
{
    QString recMsg = szMessage;
    QString msg;
//    if(bRemote)
//        msg += "<p align='left'>";
//    else
//        msg += "<p align='right'>";
   // msg += "<img src='";
   // msg += CGlobal::Instance()->GetFileUserAvatar(nick) + "' width='16' height='16'>";
    msg += "<font color='";
    if(m_pRoom->nickName() != nick)
        msg += CGlobal::Instance()->GetRosterColor().name();
    else
        msg += CGlobal::Instance()->GetUserColor().name();
    msg += "'>[";
    msg += QTime::currentTime().toString() + "]" + nick + ":</font><br /><font color='";
    if(m_pRoom->nickName() != nick)
        msg += CGlobal::Instance()->GetRosterMessageColor().name();
    else
        msg += CGlobal::Instance()->GetUserMessageColor().name();
    msg += "'>";
    msg += recMsg.replace(QString("\n"), QString("<br />"));
    msg += "</font>";
    ui->txtView->append(msg);
    return 0;
}

int CFrmGroupChat::ChangeTitle()
{
    this->setWindowTitle(tr("Group chat [%1]:%2").arg(m_pRoom->name(), m_pRoom->subject()));
    return 0;
}
