#include "addgrouppage.h"
#include "ui_addgrouppage.h"

#include <QMessageBox>

AddGroupPage::AddGroupPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AddGroupPage)
{
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::clicked, this, &AddGroupPage::onOkClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &AddGroupPage::onCancelClicked);
}

AddGroupPage::~AddGroupPage()
{
    delete ui;
}

void AddGroupPage::onOkClicked()
{
    bool ok = false;
    const int groupid = ui->lineEdit->text().trimmed().toInt(&ok);
    if (!ok || groupid <= 0)
    {
        QMessageBox::warning(this, QStringLiteral("加群"),
                             QStringLiteral("请输入有效的群 id（正整数）"));
        return;
    }
    emit addGroupRequested(groupid);
    close();
}

void AddGroupPage::onCancelClicked()
{
    close();
}
