#include "friendlist.h"
#include "ui_friendlist.h"

#include "avatarutil.h"
#include "chatservice.h"
#include "friendwid.h"

#include <QResizeEvent>

FriendList::FriendList(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FriendList)
    , m_applyRedDot(nullptr)
{
    ui->setupUi(this);

    // 好友列表样式与 chat_user_list 保持一致，去掉默认边框
    ui->friends_list->setStyleSheet(QStringLiteral(
        "QListWidget{border:none;background-color:rgb(247,247,248);}"));
    ui->friends_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // newfriend_btn 右上角红点（父对象传按钮本身，不参与布局）
    // 红点 14×14，比 IconSwitchBtn 上那个略小一点，两处统一改了也可以
    m_applyRedDot = new QLabel(ui->newfriend_btn);
    m_applyRedDot->setFixedSize(18, 18);
    m_applyRedDot->setScaledContents(true);
    m_applyRedDot->setPixmap(QPixmap(QStringLiteral(":/new/prefix1/res/red_point.png")));
    m_applyRedDot->setVisible(false);

    // 登录响应下发好友列表后自动刷新
    connect(ChatService::instance(), &ChatService::friendListReady,
            this, &FriendList::refreshFriends);

    if (ChatService::instance()->isLoggedIn())
    {
        refreshFriends();
    }

    // 功能区按钮 → 转发成信号，由 ChatDialog 弹对应页面
    connect(ui->newfriend_btn, &QPushButton::clicked, this, &FriendList::newFriendClicked);
    connect(ui->addfriend_btn, &QPushButton::clicked, this, &FriendList::addFriendClicked);
    connect(ui->addgroup_btn, &QPushButton::clicked, this, &FriendList::addGroupClicked);
    connect(ui->creategroup_btn, &QPushButton::clicked, this, &FriendList::createGroupClicked);
    connect(ui->group_btn, &QPushButton::clicked, this, &FriendList::groupListClicked);

    // 点某好友条目 → 把 friendId 转发给 ChatDialog，后者查 FriendInfo 渲染 FriendPage
    connect(ui->friends_list, &QListWidget::itemClicked,
            this, [this](QListWidgetItem *item)
    {
        QWidget *wid = ui->friends_list->itemWidget(item);
        auto *fw = qobject_cast<friendwid*>(wid);
        if (fw)
        {
            emit friendClicked(fw->friendId());
        }
    });

    // 初始化一次红点位置（setupUi 后按钮才知道自己的尺寸）
    setApplyRed(ChatService::instance()->hasApplyUnread());
}

FriendList::~FriendList()
{
    delete ui;
}

void FriendList::setApplyRed(bool show)
{
    if (m_applyRedDot)
    {
        m_applyRedDot->setVisible(show);
    }
}

void FriendList::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 关键：红点位置固定贴到 icon 右上角，**不随按钮整体宽度变化**
    // friendlist.ui 里 newfriend_btn 固定参数：
    //   padding-left = 15px（样式里写死）, iconSize = 30×30, 按钮高度固定 60
    // icon.left = 15, icon.right = 45, icon.top = (60-30)/2 = 15
    // 红点 18×18，压到 icon 右上顶点 → 红点左上角 x=45-9, y=15-9
    if (m_applyRedDot && ui && ui->newfriend_btn)
    {
        m_applyRedDot->move(36, 6);
    }
}

void FriendList::refreshFriends()
{
    ui->friends_list->clear();

    const QVector<FriendInfo> friends = ChatService::instance()->friends();
    for (const FriendInfo &friendInfo : friends)
    {
        auto *wid = new friendwid;
        wid->SetInfo(friendInfo.id,
                     friendInfo.displayName(),
                     AvatarUtil::pixmap(friendInfo.id, friendInfo.name, friendInfo.avatar),
                     friendInfo.online ? QStringLiteral("在线") : QStringLiteral("离线"));

        QListWidgetItem *item = new QListWidgetItem;
        item->setSizeHint(wid->sizeHint());
        ui->friends_list->addItem(item);
        ui->friends_list->setItemWidget(item, wid);
    }
}
