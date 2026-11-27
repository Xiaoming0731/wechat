#ifndef AVATARUTIL_H
#define AVATARUTIL_H

#include <QPixmap>
#include <QString>

// 头像工具：没有头像时按 uid+name 生成确定性的文字头像，有头像时优先读本地缓存文件
class AvatarUtil
{
public:
    static QPixmap pixmap(int uid, const QString &name, const QString &avatar = QString());
    // 群头像：复用同一套生成逻辑，区别只是取名字前两个字
    static QPixmap groupPixmap(int uid, const QString &name, const QString &avatar = QString());

private:
    static QPixmap pixmapImpl(int uid, const QString &name, const QString &avatar, int charCount);
    static QPixmap makeAvatar(int uid, const QString &name, int charCount);
};

#endif // AVATARUTIL_H
