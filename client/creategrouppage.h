#ifndef CREATEGROUPPAGE_H
#define CREATEGROUPPAGE_H

#include <QWidget>

namespace Ui {
class CreateGroupPage;
}

class CreateGroupPage : public QWidget
{
    Q_OBJECT

public:
    explicit CreateGroupPage(QWidget *parent = nullptr);
    ~CreateGroupPage();

signals:
    // 用户点"确认"时发出，把群名和描述带出去
    void createGroupRequested(const QString &name, const QString &desc);

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    Ui::CreateGroupPage *ui;
};

#endif // CREATEGROUPPAGE_H
