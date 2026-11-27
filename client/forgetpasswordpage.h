#ifndef FORGETPASSWORDPAGE_H
#define FORGETPASSWORDPAGE_H

#include <QWidget>

namespace Ui {
class ForgetPasswordPage;
}

class ForgetPasswordPage : public QWidget
{
    Q_OBJECT

public:
    explicit ForgetPasswordPage(QWidget *parent = nullptr);
    ~ForgetPasswordPage();

signals:
    void switchLogin();

private slots:
    void onConfirmClicked();

private:
    Ui::ForgetPasswordPage *ui;
};

#endif // FORGETPASSWORDPAGE_H
