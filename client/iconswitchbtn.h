#ifndef ICONSWITCHBTN_H
#define ICONSWITCHBTN_H

#include <QPushButton>
#include <QLabel>

class IconSwitchBtn : public QPushButton
{
    Q_OBJECT
public:
    explicit IconSwitchBtn(QWidget *parent = nullptr);

    // 六态图标：普通、悬浮、按下、选中、选中悬浮、选中按下
    void setIcons(const QString &normal,
                  const QString &hover = "",
                  const QString &press = "",
                  const QString &checked = "",
                  const QString &checkedHover = "",
                  const QString &checkedPress = "");
    void setIconSize(const QSize &sz);

    // 红点控制
    void ShowRedPoint(bool show);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // 把红点搬到外层父容器并对齐到按钮右上角，同时按 m_redVisible 恢复显隐
    void relocateRedPoint();

    QLabel* m_redPoint; // 未读红点
    bool m_redVisible = false;  // 红点应否显示（setParent 会隐藏控件，需要这个状态兜底）
};

#endif // ICONSWITCHBTN_H
