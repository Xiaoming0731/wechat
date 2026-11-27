#include "creategrouppage.h"
#include "ui_creategrouppage.h"

#include <QMessageBox>

CreateGroupPage::CreateGroupPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CreateGroupPage)
{
    ui->setupUi(this);

    connect(ui->confirm_btn, &QPushButton::clicked, this, &CreateGroupPage::onConfirmClicked);
    connect(ui->cancel_btn, &QPushButton::clicked, this, &CreateGroupPage::onCancelClicked);
}

CreateGroupPage::~CreateGroupPage()
{
    delete ui;
}

void CreateGroupPage::onConfirmClicked()
{
    const QString name = ui->lineEdit->text().trimmed();
    const QString desc = ui->lineEdit_2->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("创建群聊"),
                             QStringLiteral("群聊名称不能为空"));
        return;
    }
    emit createGroupRequested(name, desc);
    close();
}

void CreateGroupPage::onCancelClicked()
{
    close();
}
