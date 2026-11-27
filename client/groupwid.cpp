#include "groupwid.h"
#include "ui_groupwid.h"

GroupWid::GroupWid(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GroupWid)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "groupwid{background-color:rgb(255,255,255);}"
        "groupwid:hover{background-color:rgb(235,235,235);}"));
}

GroupWid::~GroupWid()
{
    delete ui;
}

void GroupWid::SetInfo(int groupId, const QString &name, const QPixmap &icon)
{
    _groupId = groupId;
    ui->icon_lb->setPixmap(icon);
    ui->icon_lb->setScaledContents(true);
    ui->name_lb->setText(name);
}
