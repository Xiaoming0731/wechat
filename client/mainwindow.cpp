#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QWidget>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _logindialog(nullptr), _regdialog(nullptr), _chatdialog(nullptr), _forgetpage(nullptr)
{
    ui->setupUi(this);

    // 首次创建三个页面 + 连好各自的信号
    _logindialog = new LoginDialog();
    connect(_logindialog,&LoginDialog::switchReg,this,&MainWindow::SlotSwitchReg);
    connect(_logindialog,&LoginDialog::switchChat,this,&MainWindow::SlotSwitchChat);
    connect(_logindialog,&LoginDialog::switchForget,this,&MainWindow::SlotSwitchForget);

    _regdialog = new RegDialog();
    connect(_regdialog,&RegDialog::switchLogin,this,&MainWindow::SlotSwitchLogin);

    _chatdialog = new ChatDialog();
    connect(_chatdialog,&ChatDialog::switchLogin,this,&MainWindow::SlotSwitchLogin);

    // 初始：登录页，固定 300x530
    setCentralWidget(_logindialog);
    setMinimumSize(300, 530);
    setMaximumSize(300, 530);
    centerOnScreen();
}

MainWindow::~MainWindow()
{
    delete ui;
    // 三个 dialog 若还活着（没被 deleteLater 过），手动 delete
    if(_logindialog) delete _logindialog;
    if(_regdialog)   delete _regdialog;
    if(_chatdialog)  delete _chatdialog;
    if(_forgetpage)  delete _forgetpage;
}

void MainWindow::centerOnScreen()
{
    QScreen* scr = screen();
    if(!scr) scr = QGuiApplication::primaryScreen();
    if(!scr) return;

    QRect geo = scr->availableGeometry();
    int x = geo.x() + (geo.width()  - width())  / 2;
    int y = geo.y() + (geo.height() - height()) / 2;
    move(x, y);
}

// 把当前中心部件对应的成员指针置 nullptr
// （因为 setCentralWidget 会 deleteLater 旧的，必须先标记悬空，否则下一次 ensure 会误用）
void MainWindow::nullifyOldCentral()
{
    QWidget* old = centralWidget();
    if(old == _logindialog) _logindialog = nullptr;
    else if(old == _regdialog) _regdialog = nullptr;
    else if(old == _chatdialog) _chatdialog = nullptr;
    else if(old == _forgetpage) _forgetpage = nullptr;
}

void MainWindow::ensureLogin()
{
    if(_logindialog) return;
    _logindialog = new LoginDialog();
    connect(_logindialog,&LoginDialog::switchReg,this,&MainWindow::SlotSwitchReg);
    connect(_logindialog,&LoginDialog::switchChat,this,&MainWindow::SlotSwitchChat);
    connect(_logindialog,&LoginDialog::switchForget,this,&MainWindow::SlotSwitchForget);
}

void MainWindow::ensureReg()
{
    if(_regdialog) return;
    _regdialog = new RegDialog();
    connect(_regdialog,&RegDialog::switchLogin,this,&MainWindow::SlotSwitchLogin);
}

void MainWindow::ensureChat()
{
    if(_chatdialog) return;
    _chatdialog = new ChatDialog();
    connect(_chatdialog,&ChatDialog::switchLogin,this,&MainWindow::SlotSwitchLogin);
}

void MainWindow::ensureForget()
{
    if(_forgetpage) return;
    _forgetpage = new ForgetPasswordPage();
    connect(_forgetpage,&ForgetPasswordPage::switchLogin,this,&MainWindow::SlotSwitchLogin);
}

void MainWindow::SlotSwitchReg()
{
    nullifyOldCentral();   // ← 关键：先把旧的置空
    ensureReg();
    setCentralWidget(_regdialog);

    setMinimumSize(300, 530);
    setMaximumSize(300, 530);
    resize(300, 530);
    centerOnScreen();
}

void MainWindow::SlotSwitchChat()
{
    nullifyOldCentral();
    ensureChat();
    setCentralWidget(_chatdialog);

    setMinimumSize(1000, 800);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(1000, 630);
    centerOnScreen();
}

void MainWindow::SlotSwitchLogin()
{
    nullifyOldCentral();
    ensureLogin();
    setCentralWidget(_logindialog);

    setMinimumSize(300, 530);
    setMaximumSize(300, 530);
    resize(300, 530);
    centerOnScreen();
}

void MainWindow::SlotSwitchForget()
{
    nullifyOldCentral();
    ensureForget();
    setCentralWidget(_forgetpage);

    setMinimumSize(300, 400);
    setMaximumSize(300, 400);
    resize(300, 580);
    centerOnScreen();
}
