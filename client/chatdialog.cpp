#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "global.h"
#include "chatservice.h"
#include "avatarutil.h"
#include <QButtonGroup>
#include <chatuserwid.h>
#include "addfriendpage.h"
#include "addgrouppage.h"
#include "creategrouppage.h"
#include "newfriendpage.h"
#include "friendpage.h"
#include "friendwid.h"
#include "grouplist.h"
#include "setremark.h"

#include <algorithm>

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::ChatDialog),
    _mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode),
    _addFriendPage(nullptr),
    _addGroupPage(nullptr),
    _createGroupPage(nullptr),
    _newFriendPage(nullptr),
    _friendPage(nullptr),
    _setRemarkPage(nullptr),
    _searchClearAction(nullptr),
    _currentFriendId(0),
    _activeChatFriend{},
    _activeChatGroup{}
{
    ui->setupUi(this);
    connect(ui->exit_btn,&QPushButton::clicked,this,[this]()
    {
        // 先通知服务端下线并清空本地登录态，再回登录页
        ChatService::instance()->logout();
        ui->chat_user_list->clear();
        emit switchLogin();
    });

    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/new/prefix1/res/search.png"));
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    QAction *clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/new/prefix1/res/close_transparent.png"));
    // 初始时不显示清除图标
    // 将清除动作添加到LineEdit的末尾位置
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);
    _searchClearAction = clearAction;

    // 当需要显示清除图标时，更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/new/prefix1/res/close_search.png"));
        } else {
            clearAction->setIcon(QIcon(":/new/prefix1/res/close_transparent.png")); // 文本为空时，切换回透明图标
        }
    });
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/new/prefix1/res/close_transparent.png")); // 清除文本后，切换回透明图标
        ui->search_edit->clearFocus();
        // 清除按钮被按下则不显示搜索框
        ShowSearch(false);
    });
    ShowSearch(false);   // 默认显示聊天列表，不是搜索模式

    // ========== 搜索：输入关键词 → 过滤好友+群 → 填充 search_list ==========
    connect(ui->search_edit, &QLineEdit::textChanged, this, [this](const QString &text)
    {
        const QString keyword = text.trimmed();
        if (keyword.isEmpty())
        {
            ui->search_list->clear();
            ShowSearch(false);
            return;
        }
        ShowSearch(true);
        refreshSearchResults(keyword);
    });

    // 点击结果条目 → 直接进聊天（和从会话列表点进去走同一个入口）
    connect(ui->search_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item)
    {
        auto *wid = qobject_cast<ChatUserWid*>(ui->search_list->itemWidget(item));
        if (!wid) { return; }

        // 先取 id：下面会清空列表，条目控件随之销毁
        const int friendId = wid->friendId();
        const int groupId = wid->groupId();

        resetSearch();       // 清输入框 + 还原图标 + 清结果列表（blockSignals，不重入）
        ShowSearch(false);   // 切回聊天/联系人列表

        if (groupId > 0)
        {
            openChatWithGroup(groupById(groupId));
        }
        else if (friendId > 0)
        {
            openChatWith(friendById(friendId));
        }
    });

    // 好友列表由登录响应下发，FriendList 自己会刷新；这里只更新本人头像
    connect(ChatService::instance(), &ChatService::friendListReady, this, [this]()
    {
        updateSelfAvatar();
        // 备注变更后服务端会回最新好友列表（friendListReady），同步会话列表里的显示名
        refreshSessionNames();
    });
    // 群备注同理：groupListReady 后同步会话列表里的群名
    connect(ChatService::instance(), &ChatService::groupListReady,
            this, &ChatDialog::refreshSessionNames);

    if (ChatService::instance()->isLoggedIn())
    {
        updateSelfAvatar();
        // 已在线但红点状态还没初始化（比如切页回来），主动拉一次
        ChatService::instance()->requestFriendApplyRedInit();
    }

    // 联系人页功能区按钮 → 弹出对应页面
    connect(ui->con_user_list, &FriendList::newFriendClicked,
            this, &ChatDialog::onNewFriendClicked);
    connect(ui->con_user_list, &FriendList::addFriendClicked,
            this, &ChatDialog::onAddFriendClicked);
    connect(ui->con_user_list, &FriendList::addGroupClicked,
            this, &ChatDialog::onAddGroupClicked);
    connect(ui->con_user_list, &FriendList::createGroupClicked,
            this, &ChatDialog::onCreateGroupClicked);
    // 点 FriendList 里的「群聊」按钮 → 弹顶层 GroupList，parent 传 this 让 ChatDialog 管
    connect(ui->con_user_list, &FriendList::groupListClicked, this, [this]()
    {
        auto *list = new GroupList(this);
        list->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        list->setWindowTitle(QStringLiteral("我加入的群聊"));
        list->resize(440, 600);

        // 点 GroupList 里的某个群 → 找到 GroupInfo → 开群聊（顶层弹窗要关掉）
        connect(list, &GroupList::groupClicked, this, [this, list](int groupId)
        {
            const auto groups = ChatService::instance()->groups();
            auto it = std::find_if(groups.constBegin(), groups.constEnd(),
                                   [groupId](const GroupInfo &g){ return g.id == groupId; });
            if (it != groups.constEnd())
            {
                openChatWithGroup(*it);
            }
            list->close();   // ChatDialog 是 parent，list 不手动 delete，留 Qt 自动释放
        });

        list->show();
        list->raise();
        list->activateWindow();
    });


    // 侧边栏红点：联系人图标 & friendlist 里 newfriend_btn，都听 ChatService 一条信号
    connect(ChatService::instance(), &ChatService::friendApplyRedChanged,
            ui->side_contact_btn, &IconSwitchBtn::ShowRedPoint);
    connect(ChatService::instance(), &ChatService::friendApplyRedChanged,
            ui->con_user_list, &FriendList::setApplyRed);

    // ========== 聊天红点：侧边栏图标 ==========
    connect(ChatService::instance(), &ChatService::chatSidebarRedChanged,
            ui->side_chat_btn, &IconSwitchBtn::ShowRedPoint);

    // ========== 聊天红点：每个会话 wid 右上角 ==========
    // ChatService 说"某个会话红点变了" → 找到对应 ChatUserWid 调 setUnread
    connect(ChatService::instance(), &ChatService::sessionUnreadChanged,
            this, [this](const QString &key, bool hasUnread)
    {
        auto *list = ui->chat_user_list;
        for (int i = 0; i < list->count(); ++i)
        {
            auto *w = qobject_cast<ChatUserWid*>(list->itemWidget(list->item(i)));
            if (!w) continue;
            if (ChatService::sessionKey(w->friendId(), w->groupId()) == key)
            {
                w->setUnread(hasUnread);
                break;
            }
        }
    });
    // ChatDialog 新建时错过的初始化信号（比如重新登录后 ChatDialog 比 ChatService 后创建）：
    // 直接从 ChatService 查询当前有哪些会话未读，手动点亮一遍
    auto initChatUnread = [this]()
    {
        ChatService *svc = ChatService::instance();
        if (!svc->hasChatUnread()) { return; }
        auto *list = ui->chat_user_list;
        for (int i = 0; i < list->count(); ++i)
        {
            auto *w = qobject_cast<ChatUserWid*>(list->itemWidget(list->item(i)));
            if (!w) continue;
            if (svc->isSessionUnread(w->friendId(), w->groupId()))
            {
                w->setUnread(true);
            }
        }
    };
    initChatUnread();
    // 侧边栏聊天图标也要按真源初始化一次（登录时点亮的未读信号可能早于本对象创建，没被收到）
    ui->side_chat_btn->ShowRedPoint(ChatService::instance()->hasChatUnread());

    ui->side_contact_btn->setIcons(
        ":/new/prefix1/res/contact_list.png",        // 未选中默认
        ":/new/prefix1/res/contact_list_hover.png",         // 未选中悬浮
        ":/new/prefix1/res/contact_list_press.png",         // 未选中按下
        ":/new/prefix1/res/contact_list_press.png",      // 选中默认
        ":/new/prefix1/res/contact_list_press.png",// 选中悬浮
        ":/new/prefix1/res/contact_list.png" // 选中按下
        );


    // 好友按钮
    ui->side_chat_btn->setIcons(
        ":/new/prefix1/res/chat_icon.png",
        ":/new/prefix1/res/chat_icon_hover.png",
        ":/new/prefix1/res/chat_icon_press.png",
        ":/new/prefix1/res/chat_icon_press.png",
        ":/new/prefix1/res/chat_icon_press.png",
        ":/new/prefix1/res/chat_icon.png"
        );


    QButtonGroup* sideGroup = new QButtonGroup(this);
    sideGroup->addButton(ui->side_chat_btn, 0);    // id=0 对应聊天页
    sideGroup->addButton(ui->side_contact_btn, 1); // id=1 对应联系人页
    sideGroup->setExclusive(true);
    ui->side_chat_btn->setChecked(true); // 默认选中聊天页
    _state = ChatUIMode::ChatMode;
    // 初始没有任何会话：右侧保持空白占位，不显示 ChatPage
    ui->stackedWidget->setCurrentWidget(ui->contact_placeholder);

    // ========== 3. 绑定切换逻辑：切页 + 清空/重加载 ==========
    connect(sideGroup, &QButtonGroup::idClicked, this, [this](int id)
    {
        switchPage(id);
    });

    // 好友列表里点某好友 → 显示 FriendPage（动态 new + 加到 stackedWidget）
    connect(ui->con_user_list, &FriendList::friendClicked,
            this, &ChatDialog::onFriendClicked);

    // ChatUserList 里点某现有会话 → 同步 ChatPage 到对应会话（好友或群）
    connect(ui->chat_user_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item)
    {
        auto *w = qobject_cast<ChatUserWid*>(ui->chat_user_list->itemWidget(item));
        if (!w) return;

        // 点进会话 → 清掉该会话红点（ChatService 是真源，emit sessionUnreadChanged 会自动调 wid->setUnread(false)）
        ChatService::instance()->markSessionRead(w->friendId(), w->groupId());

        if (w->isGroup())
        {
            const GroupInfo g = groupById(w->groupId(), w->name());
            // 用 openChatWithGroup 逻辑的内联简化版（不新建临时条目了，条目已经存在）
            removeTempSessions(0);
            _activeChatFriend = FriendInfo{};
            _activeChatGroup = g;
            ui->chat_page->SetGroup(g);
            ui->chat_page->clearMessages();
            switchPage(0);
            ChatService::instance()->queryGroupHistory(g.id);
        }
        else
        {
            removeTempSessions(w->friendId());
            const FriendInfo f = friendById(w->friendId(), w->name());
            if (f.id > 0)
            {
                _activeChatGroup = GroupInfo{};
                _activeChatFriend = f;
                ui->chat_page->SetFriend(f);
                ui->chat_page->clearMessages();
                switchPage(0);
                ChatService::instance()->queryHistory(f.id);
            }
        }
    });

    // 聊天消息：在线实时 / 历史回包 / 登录离线补发
    connect(ChatService::instance(), &ChatService::chatMessageReceived,
            this, &ChatDialog::onChatMessage);
    connect(ChatService::instance(), &ChatService::chatHistoryReady,
            this, &ChatDialog::onHistoryReady);
    connect(ChatService::instance(), &ChatService::groupHistoryReady,
            this, &ChatDialog::onGroupHistoryReady);
    connect(ChatService::instance(), &ChatService::offlineMessagesReady,
            this, &ChatDialog::onOfflineReady);
    connect(ChatService::instance(), &ChatService::sessionsReady,
            this, &ChatDialog::onSessionsReady);
    // 首次登录时 ChatDialog 已存在，登录成功后主动拉一次会话摘要
    connect(ChatService::instance(), &ChatService::loginOk,
            this, [this](int, const QString &) { reloadSessions(); });

    // 自己发出消息后，才算这个会话有新内容：更新预览并置顶
    connect(ui->chat_page, &ChatPage::messageSent, this, [this](const ChatMessage &msg)
    {
        if (msg.groupId > 0)
        {
            promoteGroupSession(groupById(msg.groupId), msg.text, msg.time);
        }
        else
        {
            promoteSession(friendById(msg.toId, _activeChatFriend.name), msg.text, msg.time);
        }
    });

    // 重新登录时 ChatDialog 在登录 ACK 之后才创建，登录成功信号已经错过，这里补拉一次
    if (ChatService::instance()->isLoggedIn())
    {
        reloadSessions();
    }

    // 重新登录时 ChatDialog 在登录 ACK 之后才创建，信号已经错过，这里补取一次
    const QVector<ChatMessage> offline = ChatService::instance()->takeOfflineMessages();
    if (!offline.isEmpty())
    {
        renderOfflineMessages(offline);
    }

    // ========== 设置备注弹窗的入口 ==========
    // ChatPage（常驻）的 setting_btn → 直接转调 openSetRemark
    connect(ui->chat_page, &ChatPage::settingClicked,
            this, &ChatDialog::openSetRemark);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

// 侧边栏切页的唯一实现：按钮点击和代码内跳转（如好友页点「聊天」）都走这里
void ChatDialog::switchPage(int id)
{
    // 离开聊天页：没发过消息的临时会话直接丢掉
    if (id != 0)
    {
        removeTempSessions(0);
    }

    // 切到联系人页前先销毁 FriendPage，保证右侧是纯空白占位
    if (id == 1)
    {
        delete _friendPage;
        _friendPage = nullptr;
        _currentFriendId = 0;
    }
    // 只有正在和某个好友或群聊天时才显示 ChatPage，否则右侧保持空白占位
    const bool hasActiveChat = (_activeChatFriend.id != 0 || _activeChatGroup.id != 0);
    if (id == 0 && hasActiveChat)
    {
        ui->stackedWidget->setCurrentWidget(ui->chat_page);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->contact_placeholder);
    }

    if(id == 0)
    {
        _state = ChatUIMode::ChatMode;
    }
    else if(id == 1)
    {
        _state = ChatUIMode::ContactMode;
    }
    // 切页时退出搜索，避免输入框还留着关键词、列表却已切走
    resetSearch();
    ShowSearch(false);
}

void ChatDialog::updateSelfAvatar()
{
    ChatService *service = ChatService::instance();
    ui->side_head_label->setPixmap(
        AvatarUtil::pixmap(service->userId(), service->userName(), service->selfAvatar()));
    ui->side_head_label->setScaledContents(true);
}

void ChatDialog::onAddFriendClicked()
{
    if (!_addFriendPage)
    {
        _addFriendPage = new AddFriendPage(this);
        // 作为顶层弹窗显示，但生命周期跟随 ChatDialog
        _addFriendPage->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        _addFriendPage->setWindowTitle(QStringLiteral("添加好友"));
        _addFriendPage->setFixedSize(420, 320);
    }
    _addFriendPage->show();
    _addFriendPage->raise();
    _addFriendPage->activateWindow();
}

void ChatDialog::onAddGroupClicked()
{
    if (!_addGroupPage)
    {
        _addGroupPage = new AddGroupPage(this);
        _addGroupPage->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        _addGroupPage->setWindowTitle(QStringLiteral("加入群聊"));
        _addGroupPage->setFixedSize(420, 200);

        // 输入框点确定 → 调 ChatService::addGroup
        connect(_addGroupPage, &AddGroupPage::addGroupRequested,
                ChatService::instance(), &ChatService::addGroup);
    }
    _addGroupPage->show();
    _addGroupPage->raise();
    _addGroupPage->activateWindow();
}

void ChatDialog::onCreateGroupClicked()
{
    if (!_createGroupPage)
    {
        _createGroupPage = new CreateGroupPage(this);
        _createGroupPage->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        _createGroupPage->setWindowTitle(QStringLiteral("创建群聊"));
        _createGroupPage->setFixedSize(440, 300);

        // 确认 → 调 ChatService::createGroup
        connect(_createGroupPage, &CreateGroupPage::createGroupRequested,
                ChatService::instance(), &ChatService::createGroup);
    }
    _createGroupPage->show();
    _createGroupPage->raise();
    _createGroupPage->activateWindow();
}

void ChatDialog::onNewFriendClicked()
{
    // 用户点开「新的朋友」→ 两个红点一起熄灭
    ChatService::instance()->clearApplyUnread();

    if (!_newFriendPage)
    {
        _newFriendPage = new NewFriendPage(this);
        _newFriendPage->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        _newFriendPage->setWindowTitle(QStringLiteral("新的朋友"));
        _newFriendPage->setFixedSize(440, 540);

        // 同意好友申请成功后 → ChatDialog 弹 SetRemark（每次 new NewFriendPage 都要重连）
        connect(_newFriendPage, &NewFriendPage::openSetRemarkRequested,
                this, [this](int friendId, const QString &name, const QString &remark)
        {
            openSetRemark(friendId, false, name, remark);
        });
    }
    _newFriendPage->show();
    _newFriendPage->raise();
    _newFriendPage->activateWindow();
}

void ChatDialog::openSetRemark(int targetId, bool isGroup,
                               const QString &originalName,
                               const QString &currentRemark)
{
    if (!_setRemarkPage)
    {
        _setRemarkPage = new SetRemark(this);
        _setRemarkPage->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        _setRemarkPage->setWindowTitle(QStringLiteral("设置备注"));
        _setRemarkPage->setFixedSize(420, 260);

        // 用户点 SetRemark 的"确认"→ 调 ChatService，然后乐观更新本地 UI
        connect(_setRemarkPage, &SetRemark::remarkConfirmed,
                this, [this](int targetId, bool isGroup, const QString &newRemark)
        {
            ChatService *svc = ChatService::instance();
            if (isGroup)
            {
                svc->updateGroupRemark(targetId, newRemark);
                // 乐观更新本地显示：ChatPage 如果正在聊这个群，标题立刻变
                if (_activeChatGroup.id == targetId)
                {
                    _activeChatGroup.remark = newRemark;
                    ui->chat_page->SetGroup(_activeChatGroup);
                }
            }
            else
            {
                svc->updateRemark(targetId, newRemark);
                // 同步本地 FriendInfo 缓存里的 remark
                auto friends = svc->friends();
                for (FriendInfo &f : friends)
                {
                    if (f.id == targetId) { f.remark = newRemark; break; }
                }
                // ChatPage 如果正在聊这个好友，标题立刻变
                if (_activeChatFriend.id == targetId)
                {
                    _activeChatFriend.remark = newRemark;
                    ui->chat_page->SetFriend(_activeChatFriend);
                }
                // FriendPage 如果正开着，也立刻刷新
                if (_friendPage && _currentFriendId == targetId)
                {
                    FriendInfo updated = _activeChatFriend;
                    if (updated.id == 0)
                    {
                        for (const FriendInfo &f : friends)
                        {
                            if (f.id == targetId) { updated = f; break; }
                        }
                    }
                    _friendPage->SetFriend(updated);
                }
            }
            // 服务端会回 UPDATE_REMARK_ACK，ChatService 自动 emit friendListReady / groupListReady
            // FriendList / GroupList 已经连好这些信号，会自动刷新，不用手动再拉
        });
    }

    _setRemarkPage->setup(targetId, isGroup, originalName, currentRemark);
    _setRemarkPage->show();
    _setRemarkPage->raise();
    _setRemarkPage->activateWindow();
}

void ChatDialog::ShowSearch(bool bsearch)
{
    if(bsearch){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
        return;
    }

    ui->search_list->hide();
    if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
    }else{
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        _mode = ChatUIMode::ChatMode;
    }
}

void ChatDialog::onFriendClicked(int friendId)
{
    const auto friends = ChatService::instance()->friends();
    auto it = std::find_if(friends.constBegin(), friends.constEnd(),
                           [friendId](const FriendInfo &f){ return f.id == friendId; });
    if (it == friends.constEnd())
    {
        return;
    }

    // toggle：点的是当前已显示的好友 → 关掉 FriendPage，回到空占位
    if (_friendPage && _currentFriendId == friendId)
    {
        delete _friendPage;
        _friendPage = nullptr;
        _currentFriendId = 0;
        ui->stackedWidget->setCurrentIndex(1);  // 切回预置的 contact_placeholder
        return;
    }

    // 点的是另一个 → 先关旧的（如果有），再 new 新的
    if (_friendPage)
    {
        delete _friendPage;
        _friendPage = nullptr;
    }

    _friendPage = new FriendPage();
    ui->stackedWidget->addWidget(_friendPage);
    _friendPage->SetFriend(*it);
    _currentFriendId = friendId;
    ui->stackedWidget->setCurrentWidget(_friendPage);

    // FriendPage 里点「聊天」→ 跳到 ChatPage 和这个好友聊
    connect(_friendPage, &FriendPage::chatButtonClicked,
            this, [this](int friendId)
    {
        const auto friends = ChatService::instance()->friends();
        auto it = std::find_if(friends.constBegin(), friends.constEnd(),
                               [friendId](const FriendInfo &f){ return f.id == friendId; });
        if (it != friends.constEnd())
        {
            openChatWith(*it);
        }
    });

    // FriendPage 里点设置 → 弹"改好友备注"（每次 new FriendPage 都要重连）
    connect(_friendPage, &FriendPage::settingClicked,
            this, [this](int friendId, const QString &name, const QString &remark)
    {
        openSetRemark(friendId, false, name, remark);
    });
}

void ChatDialog::openChatWith(const FriendInfo &f)
{
    // 1. 这个函数是从 FriendPage 自己的按钮槽里同步调进来的，直接 delete 会把正在
    //    执行信号的按钮一起删掉，必须用 deleteLater
    if (_friendPage)
    {
        _friendPage->deleteLater();
        _friendPage = nullptr;
    }
    _currentFriendId = 0;

    // 用户从好友页点"聊天" → 也是打开会话，清掉红点
    ChatService::instance()->markSessionRead(f.id, 0);

    // 2. 会话列表里已有这个好友：保持原位置，只切换选中行；
    //    没有则立刻在顶部建一个"临时会话"，等真正发/收消息后才转正
    auto *list = ui->chat_user_list;
    removeTempSessions(f.id);

    int row = -1;
    ChatUserWid *wid = findSession(f.id, &row);
    if (!wid)
    {
        wid = new ChatUserWid;
        wid->SetInfo(f.id, f.displayName(),
                     AvatarUtil::pixmap(f.id, f.name, f.avatar),
                     QString());
        wid->setTemp(true);
        auto *item = new QListWidgetItem;
        item->setSizeHint(wid->sizeHint());
        list->insertItem(0, item);
        list->setItemWidget(item, wid);
        row = 0;
    }

    // 3. ChatPage 同步好友并清掉上一个好友的气泡
    _activeChatFriend = f;
    ui->chat_page->SetFriend(f);
    ui->chat_page->clearMessages();

    // 4. 复用侧边栏切页：stackedWidget 索引、_state、搜索栏显隐一次同步
    switchPage(0);
    ui->side_chat_btn->setChecked(true);

    // 5. 选中对应的会话条目
    list->setCurrentRow(row);

    // 6. 拉取最近一页历史消息
    ChatService::instance()->queryHistory(f.id);
}

void ChatDialog::openChatWithGroup(const GroupInfo &g)
{
    // 1. 先关掉 FriendPage（如果开着）
    if (_friendPage)
    {
        _friendPage->deleteLater();
        _friendPage = nullptr;
    }
    _currentFriendId = 0;

    // 用户从群列表弹窗点群 → 也是打开会话，清掉红点
    ChatService::instance()->markSessionRead(0, g.id);

    // 2. 会话列表里有没有这个群的条目：有就保持位置，没有顶部建"临时会话"
    auto *list = ui->chat_user_list;
    // 离开时保留当前群的条目，其他临时条目仍清掉
    ChatUserWid *cur = findGroupSession(g.id, nullptr);
    removeTempSessions(cur ? cur->friendId() : 0);   // 好友 id 为 0 不匹配任何好友条目

    int row = -1;
    ChatUserWid *wid = findGroupSession(g.id, &row);
    if (!wid)
    {
        wid = new ChatUserWid;
        wid->SetInfo(g.id, g.displayName(),
                     AvatarUtil::groupPixmap(g.id, g.name, g.avatar),
                     QString(), true /*isGroup*/);
        wid->setTemp(true);
        auto *item = new QListWidgetItem;
        item->setSizeHint(wid->sizeHint());
        list->insertItem(0, item);
        list->setItemWidget(item, wid);
        row = 0;
    }

    // 3. 切到群聊模式：_activeChatFriend 清零、_activeChatGroup 设上
    _activeChatFriend = FriendInfo{};
    _activeChatGroup = g;
    ui->chat_page->SetGroup(g);
    ui->chat_page->clearMessages();

    // 4. 切侧边栏 + 选中条目 + 拉群历史
    switchPage(0);
    ui->side_chat_btn->setChecked(true);
    list->setCurrentRow(row);
    ChatService::instance()->queryGroupHistory(g.id);
}

FriendInfo ChatDialog::friendById(int id, const QString &fallbackName) const
{
    const auto friends = ChatService::instance()->friends();
    for (const FriendInfo &f : friends)
    {
        if (f.id == id)
        {
            return f;
        }
    }
    FriendInfo f;
    f.id = id;
    f.name = fallbackName;
    return f;
}

ChatUserWid *ChatDialog::findSession(int friendId, int *row) const
{
    auto *list = ui->chat_user_list;
    for (int i = 0; i < list->count(); ++i)
    {
        auto *w = qobject_cast<ChatUserWid*>(list->itemWidget(list->item(i)));
        if (w && w->friendId() == friendId)
        {
            if (row) { *row = i; }
            return w;
        }
    }
    if (row) { *row = -1; }
    return nullptr;
}

void ChatDialog::promoteSession(const FriendInfo &f, const QString &preview, const QString &time)
{
    auto *list = ui->chat_user_list;
    int row = -1;
    ChatUserWid *w = findSession(f.id, &row);

    if (w && row == 0)
    {
        // 已经在最前，只更新最后一条消息预览
        w->setTemp(false);   // 有真实消息了，不再是临时会话
        w->setChatText(preview);
        w->setTime(time);
        return;
    }
    if (w)
    {
        // 旧条目由视图接管释放（takeItem 后 widget 会被 deleteLater），不要继续复用
        delete list->takeItem(row);
    }

    auto *wid = new ChatUserWid;
    wid->SetInfo(f.id, f.displayName(),
                 AvatarUtil::pixmap(f.id, f.name, f.avatar),
                 preview);
    wid->setTemp(false);
    wid->setTime(time);
    auto *item = new QListWidgetItem;
    item->setSizeHint(wid->sizeHint());
    list->insertItem(0, item);
    list->setItemWidget(item, wid);
    // 这个会话可能本来就处于未读状态（重建 wid 时红点不能丢），从真源恢复
    wid->setUnread(ChatService::instance()->isSessionUnread(f.id, 0));
}

// 只更新预览文字，不改变顺序（打开会话、加载历史时用）
void ChatDialog::updateSessionPreview(int friendId, const QString &preview, const QString &time)
{
    int row = -1;
    ChatUserWid *w = findSession(friendId, &row);
    if (w)
    {
        w->setChatText(preview);
        w->setTime(time);
    }
}

GroupInfo ChatDialog::groupById(int id, const QString &fallbackName) const
{
    const auto groups = ChatService::instance()->groups();
    for (const GroupInfo &g : groups)
    {
        if (g.id == id)
        {
            return g;
        }
    }
    GroupInfo g;
    g.id = id;
    g.name = fallbackName;
    return g;
}

ChatUserWid *ChatDialog::findGroupSession(int groupId, int *row) const
{
    auto *list = ui->chat_user_list;
    for (int i = 0; i < list->count(); ++i)
    {
        auto *w = qobject_cast<ChatUserWid*>(list->itemWidget(list->item(i)));
        if (w && w->groupId() == groupId)
        {
            if (row) { *row = i; }
            return w;
        }
    }
    if (row) { *row = -1; }
    return nullptr;
}

void ChatDialog::promoteGroupSession(const GroupInfo &g, const QString &preview, const QString &time)
{
    auto *list = ui->chat_user_list;
    int row = -1;
    ChatUserWid *w = findGroupSession(g.id, &row);

    if (w && row == 0)
    {
        w->setTemp(false);
        w->setChatText(preview);
        w->setTime(time);
        return;
    }
    if (w)
    {
        delete list->takeItem(row);
    }

    auto *wid = new ChatUserWid;
    wid->SetInfo(g.id, g.displayName(),
                 AvatarUtil::groupPixmap(g.id, g.name, g.avatar),
                 preview, true /*isGroup*/);
    wid->setTemp(false);
    wid->setTime(time);
    auto *item = new QListWidgetItem;
    item->setSizeHint(wid->sizeHint());
    list->insertItem(0, item);
    list->setItemWidget(item, wid);
    // 群会话同样从真源恢复未读红点
    wid->setUnread(ChatService::instance()->isSessionUnread(0, g.id));
}

void ChatDialog::updateGroupSessionPreview(int groupId, const QString &preview, const QString &time)
{
    int row = -1;
    ChatUserWid *w = findGroupSession(groupId, &row);
    if (w)
    {
        w->setChatText(preview);
        w->setTime(time);
    }
}

void ChatDialog::removeTempSessions(int keepFriendId)
{
    auto *list = ui->chat_user_list;
    for (int i = list->count() - 1; i >= 0; --i)
    {
        auto *w = qobject_cast<ChatUserWid*>(list->itemWidget(list->item(i)));
        if (!w || !w->isTemp() || w->friendId() == keepFriendId)
        {
            continue;
        }

        const int friendId = w->friendId();
        delete list->takeItem(i);

        // 删掉的正是当前打开的会话：右侧也回到空白
        if (_activeChatFriend.id == friendId)
        {
            _activeChatFriend = FriendInfo{};
            ui->chat_page->clearMessages();
        }
    }
}

void ChatDialog::resetSearch()
{
    if (!ui->search_edit->text().isEmpty())
    {
        // 屏蔽 textChanged，避免 clear() 反过来触发筛选槽（这里可能正处在 itemClicked 里）
        ui->search_edit->blockSignals(true);
        ui->search_edit->clear();
        ui->search_edit->blockSignals(false);
    }
    // blockSignals 跳过了图标切换逻辑，这里手动还原成透明图标
    if (_searchClearAction)
    {
        _searchClearAction->setIcon(QIcon(":/new/prefix1/res/close_transparent.png"));
    }
    ui->search_list->clear();
}

void ChatDialog::refreshSearchResults(const QString &keyword)
{
    ui->search_list->clear();
    ChatService *svc = ChatService::instance();

    auto addItem = [this](ChatUserWid *wid)
    {
        auto *item = new QListWidgetItem;
        item->setSizeHint(wid->sizeHint());
        ui->search_list->addItem(item);
        ui->search_list->setItemWidget(item, wid);
    };

    // 好友：昵称或备注任一命中即可（不区分大小写）
    for (const FriendInfo &f : svc->friends())
    {
        if (!f.name.contains(keyword, Qt::CaseInsensitive)
            && !f.remark.contains(keyword, Qt::CaseInsensitive))
        {
            continue;
        }
        auto *wid = new ChatUserWid;
        wid->SetInfo(f.id, f.displayName(),
                     AvatarUtil::pixmap(f.id, f.name, f.avatar),
                     QStringLiteral("好友"));
        addItem(wid);
    }

    // 群：群名或群备注任一命中
    for (const GroupInfo &g : svc->groups())
    {
        if (!g.name.contains(keyword, Qt::CaseInsensitive)
            && !g.remark.contains(keyword, Qt::CaseInsensitive))
        {
            continue;
        }
        auto *wid = new ChatUserWid;
        wid->SetInfo(g.id, g.displayName(),
                     AvatarUtil::groupPixmap(g.id, g.name, g.avatar),
                     QStringLiteral("群聊"), true /*isGroup*/);
        addItem(wid);
    }

    // 空结果占位，避免整页空白让人以为界面坏了
    if (ui->search_list->count() == 0)
    {
        auto *item = new QListWidgetItem(QStringLiteral("未找到相关联系人"));
        item->setFlags(Qt::NoItemFlags);
        ui->search_list->addItem(item);
    }
}

void ChatDialog::refreshSessionNames()
{
    ChatService *svc = ChatService::instance();
    const auto friends = svc->friends();
    const auto groups = svc->groups();

    auto *list = ui->chat_user_list;
    for (int i = 0; i < list->count(); ++i)
    {
        auto *w = qobject_cast<ChatUserWid*>(list->itemWidget(list->item(i)));
        if (!w) { continue; }

        if (w->isGroup())
        {
            for (const GroupInfo &g : groups)
            {
                if (g.id != w->groupId()) { continue; }
                w->setDisplayName(g.displayName());
                // 正在聊这个群 → 右侧标题也跟着变
                if (_activeChatGroup.id == g.id)
                {
                    _activeChatGroup = g;
                    ui->chat_page->SetGroup(g);
                }
                break;
            }
        }
        else
        {
            for (const FriendInfo &f : friends)
            {
                if (f.id != w->friendId()) { continue; }
                w->setDisplayName(f.displayName());
                if (_activeChatFriend.id == f.id)
                {
                    _activeChatFriend = f;
                    ui->chat_page->SetFriend(f);
                }
                break;
            }
        }
    }
}

void ChatDialog::reloadSessions()
{
    ChatService::instance()->querySessions();
}

void ChatDialog::onSessionsReady(const QVector<SessionInfo> &sessions)
{
    // 服务端按"最新在前"返回；倒序插入，保证最终最新的会话在最上面
    for (int i = sessions.size() - 1; i >= 0; --i)
    {
        const SessionInfo &s = sessions.at(i);
        if (s.groupId > 0)
        {
            promoteGroupSession(groupById(s.groupId, s.name), s.lastMsg, s.time);
        }
        else if (s.friendId > 0)
        {
            promoteSession(friendById(s.friendId, s.name), s.lastMsg, s.time);
        }
    }

    // 只是恢复列表，不自动进入任何会话：清掉当前项，避免右侧空白但某行高亮
    ui->chat_user_list->setCurrentRow(-1);
    ui->chat_user_list->clearSelection();
}

void ChatDialog::onChatMessage(const ChatMessage &msg)
{
    const int selfId = ChatService::instance()->userId();

    if (msg.groupId > 0)
    {
        // 群聊：服务端会把群成员（含自己）全部转发一份，自己发的也能在这里收到
        if (msg.fromId != selfId)
        {
            promoteGroupSession(groupById(msg.groupId), msg.text, msg.time);
        }

        // 打开的是这个群的 ChatPage → 渲染气泡（自己发的也渲染成自己的）
        if (_activeChatGroup.id == msg.groupId
            && ui->stackedWidget->currentWidget() == ui->chat_page)
        {
            ui->chat_page->appendMessage(msg);
        }
        return;
    }

    // 单聊：服务端只转发给对方，自己发的不会回过来
    if (msg.fromId == selfId)
    {
        return;
    }

    promoteSession(friendById(msg.fromId, msg.fromName), msg.text, msg.time);

    if (_activeChatFriend.id == msg.fromId
        && ui->stackedWidget->currentWidget() == ui->chat_page)
    {
        ui->chat_page->appendMessage(msg);
    }
}

void ChatDialog::onHistoryReady(int friendid, const QVector<ChatMessage> &msgs)
{
    // 用户已经切到别的会话了，丢弃过期回包
    if (friendid != _activeChatFriend.id)
    {
        return;
    }

    ui->chat_page->clearMessages();
    for (const ChatMessage &msg : msgs)
    {
        ui->chat_page->appendMessage(msg);
    }

    if (!msgs.isEmpty())
    {
        // 只刷新预览文字，不调整顺序：打开会话不算"有新消息"
        updateSessionPreview(friendid, msgs.last().text, msgs.last().time);

        // 有历史说明是真实会话，去掉"临时"标记，避免离开时被删掉
        int row = -1;
        if (ChatUserWid *w = findSession(friendid, &row))
        {
            w->setTemp(false);
        }
    }
}

void ChatDialog::onOfflineReady()
{
    const QVector<ChatMessage> msgs = ChatService::instance()->takeOfflineMessages();
    if (!msgs.isEmpty())
    {
        renderOfflineMessages(msgs);
    }
}

void ChatDialog::onGroupHistoryReady(int groupid, const QVector<ChatMessage> &msgs)
{
    if (groupid != _activeChatGroup.id)
    {
        return;
    }

    ui->chat_page->clearMessages();
    for (const ChatMessage &msg : msgs)
    {
        ui->chat_page->appendMessage(msg);
    }

    if (!msgs.isEmpty())
    {
        updateGroupSessionPreview(groupid, msgs.last().text, msgs.last().time);
        int row = -1;
        if (ChatUserWid *w = findGroupSession(groupid, &row))
        {
            w->setTemp(false);
        }
    }
}

void ChatDialog::renderOfflineMessages(const QVector<ChatMessage> &msgs)
{
    // 离线消息只用来生成/更新的会话条目，气泡等内容点开会话后由历史查询统一加载
    const int selfId = ChatService::instance()->userId();
    for (const ChatMessage &msg : msgs)
    {
        if (msg.fromId == selfId)
        {
            continue;
        }
        if (msg.groupId > 0)
        {
            promoteGroupSession(groupById(msg.groupId), msg.text, msg.time);
        }
        else
        {
            promoteSession(friendById(msg.fromId, msg.fromName), msg.text, msg.time);
        }
    }
}
