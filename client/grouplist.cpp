#include "grouplist.h"
#include "ui_grouplist.h"

#include "avatarutil.h"
#include "chatservice.h"
#include "groupwid.h"

GroupList::GroupList(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GroupList)
{
    ui->setupUi(this);

    // 样式和 friendlist 里的 friends_list 保持一致
    ui->group_list->setStyleSheet(QStringLiteral(
        "QListWidget{border:none;background-color:rgb(247,247,248);}"));
    ui->group_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // 登录响应、建群、加群、改备注都会回最新群列表 → 自动刷新
    connect(ChatService::instance(), &ChatService::groupListReady,
            this, &GroupList::refreshGroups);

    if (ChatService::instance()->isLoggedIn())
    {
        refreshGroups();
    }

    // 点某群条目 → 把 groupId 转发给 ChatDialog，后者后续可以渲染群资料 / 群聊
    connect(ui->group_list, &QListWidget::itemClicked,
            this, [this](QListWidgetItem *item)
    {
        QWidget *wid = ui->group_list->itemWidget(item);
        auto *gw = qobject_cast<GroupWid*>(wid);
        if (gw)
        {
            emit groupClicked(gw->groupId());
        }
    });
}

GroupList::~GroupList()
{
    delete ui;
}

void GroupList::refreshGroups()
{
    ui->group_list->clear();

    const QVector<GroupInfo> groups = ChatService::instance()->groups();
    for (const GroupInfo &group : groups)
    {
        auto *wid = new GroupWid;
        // 头像文字必须用群共享名 group.name，不能用备注（备注是私有视图）
        wid->SetInfo(group.id,
                     group.displayName(),
                     AvatarUtil::groupPixmap(group.id, group.name));

        QListWidgetItem *item = new QListWidgetItem;
        item->setSizeHint(wid->sizeHint());
        ui->group_list->addItem(item);
        ui->group_list->setItemWidget(item, wid);
    }
}
