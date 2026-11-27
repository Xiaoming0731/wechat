#include "regdialog.h"
#include "ui_regdialog.h"
#include "global.h"
#include "IconSwitchBtn.h"
#include "chatservice.h"

RegDialog::RegDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegDialog)
{
    ui->setupUi(this);
    ui->password_edit->setEchoMode(QLineEdit::Password);
    ui->password_edit2->setEchoMode(QLineEdit::Password);
    ui->error_label->setProperty("state","normal");
    repolish(ui->error_label);
    connect(ui->cancer_btn,&QPushButton::clicked,this,&RegDialog::switchLogin);
    connect(ui->confirm_btn,&QPushButton::clicked,this,&RegDialog::onConfirmClicked);

    ChatService *service = ChatService::instance();

    connect(service, &ChatService::regOk, this, [this](int userid)
            {
                ui->confirm_btn->setEnabled(true);
                showTip(QStringLiteral("注册成功，你的账号是 %1").arg(userid), true);
            });

    connect(service, &ChatService::regFailed, this, [this](const QString &errmsg)
            {
                ui->confirm_btn->setEnabled(true);
                showTip(errmsg, false);
            });

    connect(service, &ChatService::networkError, this, [this](const QString &errmsg)
            {
                ui->confirm_btn->setEnabled(true);
                showTip(QStringLiteral("网络错误：%1").arg(errmsg), false);
            });


    ui->password_btn->setIcons(
        ":/new/prefix1/res/unvisible.png",        // normal
        ":/new/prefix1/res/unvisible_hover.png",  // hover
        ":/new/prefix1/res/unvisible.png",        // press 没有专门按下图，复用normal图，不要传空！
        ":/new/prefix1/res/visible.png",          // checked
        ":/new/prefix1/res/visible_hover.png",    // checked-hover（修正拼写！）
        ":/new/prefix1/res/visible.png"           // checked-press 没有专门按下图，复用checked图，不要传空！
        );
    ui->password_btn->setFixedSize(24,24);

    connect(ui->password_btn, &IconSwitchBtn::toggled, this, [=](bool checked)
            {
                qDebug() << "checked = " << checked; // 加这个！看控制台有没有打印
                if (checked)
                {
                    ui->password_edit->setEchoMode(QLineEdit::Normal);
                }
                else
                {
                    ui->password_edit->setEchoMode(QLineEdit::Password);
                }
            });


    ui->pass_confirm_btn->setIcons(
        ":/new/prefix1/res/unvisible.png",        // normal
        ":/new/prefix1/res/unvisible_hover.png",  // hover
        ":/new/prefix1/res/unvisible.png",        // press 没有专门按下图，复用normal图，不要传空！
        ":/new/prefix1/res/visible.png",          // checked
        ":/new/prefix1/res/visible_hover.png",    // checked-hover（修正拼写！）
        ":/new/prefix1/res/visible.png"           // checked-press 没有专门按下图，复用checked图，不要传空！
        );
    ui->pass_confirm_btn->setFixedSize(24,24);

    connect(ui->pass_confirm_btn, &IconSwitchBtn::toggled, this, [=](bool checked)
            {
                qDebug() << "checked = " << checked; // 加这个！看控制台有没有打印
                if (checked)
                {
                    ui->password_edit2->setEchoMode(QLineEdit::Normal);
                }
                else
                {
                    ui->password_edit2->setEchoMode(QLineEdit::Password);
                }
            });



}

RegDialog::~RegDialog()
{
    delete ui;
}

void RegDialog::onConfirmClicked()
{
    const QString name = ui->user_edit->text().trimmed();
    const QString password = ui->password_edit->text();
    const QString password2 = ui->password_edit2->text();

    if (name.isEmpty())
    {
        showTip(QStringLiteral("请输入用户名"), false);
        return;
    }
    if (password.isEmpty())
    {
        showTip(QStringLiteral("请输入密码"), false);
        return;
    }
    if (password != password2)
    {
        showTip(QStringLiteral("两次输入的密码不一致"), false);
        return;
    }

    ui->confirm_btn->setEnabled(false);
    ChatService::instance()->reg(name, password);
}

void RegDialog::showTip(QString s, bool ok)
{
    if(ok)
    {
         ui->error_label->setProperty("state","normal");
    }
    else
    {
         ui->error_label->setProperty("state","err");
    }
    ui->error_label->setText(s);

    repolish(ui->error_label);
}

