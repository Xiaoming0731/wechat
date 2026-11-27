#include "newfriendpage.h"
#include "ui_newfriendpage.h"

#include "chatservice.h"
#include "friendapplywid.h"

NewFriendPage::NewFriendPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NewFriendPage)
{
    ui->setupUi(this);

    // 列表样式和好友列表一致
    ui->friend_apply_list->setStyleSheet(QStringLiteral(
        "QListWidget{border:none;background-color:rgb(247,247,248);}"));
    ui->friend_apply_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->friend_apply_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 连上 ChatService 的信号
    ChatService *service = ChatService::instance();

    // 查询申请列表返回 → 刷 UI
    connect(service, &ChatService::friendRequestsReady,
            this, &NewFriendPage::onFriendRequestsReady);

    // 处理同意/拒绝的结果 → 成功置灰/失败恢复
    connect(service, &ChatService::handleRequestResult,
            this, &NewFriendPage::onHandleRequestResult);

    // 实时推送的新申请 → 头插到顶部，不重拉整个列表
    connect(service, &ChatService::friendRequestNotify,
            this, &NewFriendPage::onFriendRequestNotify);

    // 已登录就立即拉取一次
    if (service->isLoggedIn())
    {
        service->getFriendRequests(service->userId());
    }
}

NewFriendPage::~NewFriendPage()
{
    delete ui;
}

void NewFriendPage::insertItem(const ApplyInfo &info, int row)
{
    auto *wid = new FriendApplyWid;
    wid->SetInfo(info);

    // 每个条目的同意按钮 → 交给 ChatService 处理
    connect(wid, &FriendApplyWid::acceptClicked,
            this, &NewFriendPage::onAcceptClicked);

    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(wid->sizeHint());
    ui->friend_apply_list->insertItem(row, item);
    ui->friend_apply_list->setItemWidget(item, wid);

    _widMap.insert(info.reqid, wid);
}

void NewFriendPage::refreshList(const QVector<ApplyInfo> &list)
{
    // 整体刷新：先清本地 map，再清 QListWidget，再按服务端 desc 排序头插
    _widMap.clear();
    ui->friend_apply_list->clear();

    // 服务端 SQL 是 order by r.id desc → list[0] 是最新的
    // 头插：每次 insertItem(0) 保证最新的永远在顶部
    for (const ApplyInfo &info : list)
    {
        insertItem(info, 0);
    }
}

void NewFriendPage::onFriendRequestsReady(const QVector<ApplyInfo> &list)
{
    refreshList(list);
}

void NewFriendPage::onHandleRequestResult(int reqid, bool success)
{
    auto it = _widMap.find(reqid);
    if (it == _widMap.end())
    {
        return;
    }
    FriendApplyWid *wid = it.value();
    if (success)
    {
        wid->markHandled();   // 按钮变"已同意"，禁用
        // ChatService 会在收到 HANDLE_FRIEND_REQ_ACK 时自动解析 friends 数组
        // 并 emit friendListReady → FriendList 自动刷新

        // 同意成功后请求 ChatDialog 弹 SetRemark（需求 2）
        // 初始备注为空，让 SetRemark 的 setup 显示对方昵称作为默认值
        const ApplyInfo &info = wid->info();
        emit openSetRemarkRequested(info.fromId, info.fromName, QString());
    }
    else
    {
        wid->markEnabled();   // 请求被拒绝/失败 → 恢复按钮
    }
}

void NewFriendPage::onFriendRequestNotify(int reqid, int fromId, const QString &fromName, const QString &applyMsg)
{
    // 实时推送 → 直接构造条目头插到顶部，不打断现有列表的状态
    ApplyInfo info;
    info.reqid = reqid;
    info.fromId = fromId;
    info.fromName = fromName;
    info.applyMsg = applyMsg;
    insertItem(info, 0);
}

void NewFriendPage::onAcceptClicked(int reqid)
{
    ChatService *service = ChatService::instance();
    if (!service->isLoggedIn())
    {
        return;
    }

    // 本地立刻置灰，UI 反馈即时
    auto it = _widMap.find(reqid);
    if (it != _widMap.end())
    {
        it.value()->markHandled();
    }

    service->handleFriendRequest(service->userId(), reqid, true);
}
