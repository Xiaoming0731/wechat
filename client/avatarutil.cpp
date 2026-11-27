#include "avatarutil.h"

#include <QColor>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QHash>
#include <QPainter>

namespace {
// 进程内缓存，同一用户不重复绘制
QHash<QString, QPixmap> g_cache;
}

QPixmap AvatarUtil::pixmap(int uid, const QString &name, const QString &avatar)
{
    return pixmapImpl(uid, name, avatar, 1);
}

QPixmap AvatarUtil::groupPixmap(int uid, const QString &name, const QString &avatar)
{
    return pixmapImpl(uid, name, avatar, 2);
}

QPixmap AvatarUtil::pixmapImpl(int uid, const QString &name, const QString &avatar, int charCount)
{
    // charCount 必须进缓存 key：同名同 uid 的文字头像和群头像不能互相命中
    const QString key = QString::number(uid) + '|' + name + '|' + avatar + '|'
                        + QString::number(charCount);
    const auto it = g_cache.constFind(key);
    if (it != g_cache.constEnd())
    {
        return it.value();
    }

    QPixmap pix;
    if (!avatar.isEmpty())
    {
        // 头像上传下载功能还没做，这里先支持加载已下载到本地的文件
        const QString path = QCoreApplication::applicationDirPath()
                             + "/cache/avatar/" + QFileInfo(avatar).fileName();
        if (QFile::exists(path))
        {
            pix.load(path);
        }
    }
    if (pix.isNull())
    {
        pix = makeAvatar(uid, name, charCount);
    }

    g_cache.insert(key, pix);
    return pix;
}

QPixmap AvatarUtil::makeAvatar(int uid, const QString &name, int charCount)
{
    // 用 uid 推色相，保证同一个人在任何地方、任何时间颜色都一样
    const int hue = (uid * 47) % 360;

    QPixmap pix(40, 40);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor::fromHsv(hue, 160, 220));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, 40, 40);

    QFont f = p.font();
    f.setPixelSize(charCount > 1 ? 14 : 18); // 两个字要小一号，40px 圆里才放得下
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(pix.rect(), Qt::AlignCenter,
               name.isEmpty() ? QString::number(uid) : name.left(charCount));
    return pix;
}
