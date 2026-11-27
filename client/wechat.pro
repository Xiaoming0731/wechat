QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
RC_ICONS=wechat-256x256.ico
DESTDIR=./bin

CONFIG += c++17

# Qt 6.9.3 qcomparehelpers.h 里的 noexcept static_assert 在 MinGW 13.1 下误触发。
# 这个断言的 #if 条件：defined(__cpp_lib_three_way_comparison) || !(QNX || GHS)
# 通过 define Q_CC_GHS 让它走 #else 分支（no check）。
DEFINES += Q_CC_GHS

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addfriendpage.cpp \
    addgrouppage.cpp \
    bubbleframe.cpp \
    chatdialog.cpp \
    chatitembase.cpp \
    chatpage.cpp \
    chatuserlist.cpp \
    chatuserwid.cpp \
    chatview.cpp \
    chatservice.cpp \
    avatarutil.cpp \
    creategrouppage.cpp \
    forgetpasswordpage.cpp \
    friendapplywid.cpp \
    friendlist.cpp \
    friendpage.cpp \
    friendwid.cpp \
    global.cpp \
    grouplist.cpp \
    groupwid.cpp \
    iconswitchbtn.cpp \
    listitembase.cpp \
    loadingdlg.cpp \
    logindialog.cpp \
    main.cpp \
    mainwindow.cpp \
    messagetextedit.cpp \
    newfriendpage.cpp \
    picturebubble.cpp \
    protocol.cpp \
    regdialog.cpp \
    searchlist.cpp \
    setremark.cpp \
    statewidget.cpp \
    tcpclient.cpp \
    textbubble.cpp

HEADERS += \
    addfriendpage.h \
    addgrouppage.h \
    bubbleframe.h \
    chatdialog.h \
    chatitembase.h \
    chatpage.h \
    chatuserlist.h \
    chatuserwid.h \
    chatview.h \
    chatservice.h \
    avatarutil.h \
    creategrouppage.h \
    forgetpasswordpage.h \
    friendapplywid.h \
    friendlist.h \
    friendpage.h \
    friendwid.h \
    global.h \
    grouplist.h \
    groupwid.h \
    iconswitchbtn.h \
    listitembase.h \
    loadingdlg.h \
    logindialog.h \
    mainwindow.h \
    messagetextedit.h \
    newfriendpage.h \
    picturebubble.h \
    protocol.h \
    regdialog.h \
    searchlist.h \
    setremark.h \
    statewidget.h \
    tcpclient.h \
    textbubble.h

FORMS += \
    addfriendpage.ui \
    addgrouppage.ui \
    chatdialog.ui \
    chatpage.ui \
    chatuserwid.ui \
    creategrouppage.ui \
    forgetpasswordpage.ui \
    friendapplywid.ui \
    friendlist.ui \
    friendpage.ui \
    friendwid.ui \
    grouplist.ui \
    groupwid.ui \
    loadingdlg.ui \
    logindialog.ui \
    mainwindow.ui \
    newfriendpage.ui \
    regdialog.ui \
    setremark.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

DISTFILES +=
