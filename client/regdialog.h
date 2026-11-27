#ifndef REGDIALOG_H
#define REGDIALOG_H

#include <QDialog>

namespace Ui {
class RegDialog;
}

class RegDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegDialog(QWidget *parent = nullptr);
    ~RegDialog();

signals:
    void switchLogin();


private slots:
    void onConfirmClicked();

private:
    void showTip(QString s,bool ok);
    Ui::RegDialog *ui;
};

#endif // REGDIALOG_H
