#include "setremark.h"
#include "ui_setremark.h"

SetRemark::SetRemark(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SetRemark)
{
    ui->setupUi(this);

    // 弹窗级别的"取消"：右上角 × 也会触发 closeEvent，直接 hide 掉
    // 这里不设 Qt::WA_DeleteOnClose，生命周期跟随 ChatDialog
}

SetRemark::~SetRemark()
{
    delete ui;
}

void SetRemark::setup(int targetId, bool isGroup,
                      const QString &originalName,
                      const QString &currentRemark)
{
    _targetId = targetId;
    _isGroup = isGroup;

    // 标题区动态适配
    ui->label->setText(isGroup ? QStringLiteral("设置群备注")
                               : QStringLiteral("设置好友备注"));
    ui->label_2->setText(isGroup ? QStringLiteral("请输入群备注")
                                 : QStringLiteral("请输入好友备注"));

    // 默认值：有备注显示备注，没备注显示原昵称（需求 1）
    const QString defaultValue = currentRemark.isEmpty() ? originalName : currentRemark;
    ui->lineEdit->setText(defaultValue);
    ui->lineEdit->selectAll();   // 全选方便直接覆盖
    ui->lineEdit->setFocus();
}

void SetRemark::on_pushButton_clicked()
{
    emit remarkConfirmed(_targetId, _isGroup, ui->lineEdit->text().trimmed());
    hide();
}

void SetRemark::on_pushButton_2_clicked()
{
    hide();
}
