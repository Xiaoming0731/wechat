#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "regdialog.h"
#include "chatdialog.h"
#include "forgetpasswordpage.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void SlotSwitchReg();
    void SlotSwitchChat();
    void SlotSwitchLogin();
    void SlotSwitchForget();

private:
    void centerOnScreen();
    void nullifyOldCentral();
    void ensureLogin();
    void ensureReg();
    void ensureChat();
    void ensureForget();

    Ui::MainWindow *ui;
    LoginDialog* _logindialog;
    RegDialog* _regdialog;
    ChatDialog* _chatdialog;
    ForgetPasswordPage* _forgetpage;
};
#endif // MAINWINDOW_H
