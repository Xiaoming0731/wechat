#include "friendwid.h"
#include "ui_friendwid.h"

#include <QPixmap>

friendwid::friendwid(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::friendwid)
{
    ui->setupUi(this);

    // QWidget 子类要用样式表画背景，必须开启 WA_StyledBackground
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "friendwid{background-color:rgb(255,255,255);}"
        "friendwid:hover{background-color:rgb(235,235,235);}"));
}

friendwid::~friendwid()
{
    delete ui;
}

void friendwid::SetInfo(int friendId, const QString &name, const QPixmap &icon, const QString &state)
{
    _friendId = friendId;
    ui->icon_lb->setPixmap(icon);
    ui->icon_lb->setScaledContents(true);
    ui->name_lb->setText(name);
    ui->state_lb->setText(state);
}
