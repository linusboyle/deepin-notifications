/*
 * Copyright (C) 2014 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     kirigaya <kirigaya@mkacg.com>
 *             listenerri <listenerri@gmail.com>
 *
 * Maintainer: listenerri <listenerri@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BUBBLE_H
#define BUBBLE_H

#include <QFrame>
#include <DBlurEffectWidget>
#include <QStandardPaths>
#include <QDir>
#include <DPlatformWindowHandle>
#include <DWindowManagerHelper>
#include <QDBusArgument>

DWIDGET_USE_NAMESPACE

static const int BubbleWidth = 300;
static const int BubbleHeight = 70;

static const QString ControlCenterDBusService = "com.deepin.dde.ControlCenter";
static const QString ControlCenterDBusPath = "/com/deepin/dde/ControlCenter";
static const QString DBusDockDBusServer = "com.deepin.dde.Dock";
static const QString DBusDockDBusPath = "/com/deepin/dde/Dock";
static const QString DBusDaemonDBusService = "org.freedesktop.DBus";
static const QString DBusDaemonDBusPath = "/org/freedesktop/DBus";
static const QString NotificationsDBusService = "org.freedesktop.Notifications";
static const QString NotificationsDBusPath = "/org/freedesktop/Notifications";
static const QString DDENotifyDBusServer = "com.deepin.dde.Notification";
static const QString DDENotifyDBusPath = "/com/deepin/dde/Notification";
static const QString Login1DBusService = "org.freedesktop.login1";
static const QString Login1DBusPath = "/org/freedesktop/login1";
static const QString DockDaemonDBusService = "com.deepin.dde.daemon.Dock";
static const QString DockDaemonDBusPath = "/com/deepin/dde/daemon/Dock";
static const int ControlCenterWidth = 400;

class QLabel;
class AppIcon;
class QPropertyAnimation;
class QParallelAnimationGroup;
class NotificationEntity;
class ActionButton;
class AppBody;
class QGraphicsDropShadowEffect;

static const QStringList Directory = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
static const QString CachePath = Directory.first() + "/.cache/deepin/deepin-notifications/";

class Bubble : public DBlurEffectWidget
{
    Q_OBJECT
public:
    Bubble(NotificationEntity *entity = nullptr,QWidget* parent=nullptr);

    inline NotificationEntity *entity() const {
        return m_entity;
    }

    // replace the content with provided entity
    void setEntity(NotificationEntity *entity);

Q_SIGNALS:
    //notification closed for different reasons:
    //handled by BubbleLayout
    
    //emitted when timer timeout etc.
    void expired(uint);
    //emitted when user click mouse without default action
    void dismissed(uint);
    //TODO
    //not used yet
    void replacedByOther(uint);
    //emitted when user click mouse on bubble with default action
    //or when the action button is clicked
    void actionInvoked(uint, QString);

public Q_SLOTS:
    // change ui style when compositor changed,dde only
    void compositeChanged();
    // if autoquit is set,automatically quit the application
    void onDelayQuit();

    // update the animation,taking parameter rect as the screen bounding rect
    //void resetMoveAnim(const QRect &rect);

protected:
    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *event) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onActionButtonClicked(const QString &actionId);
    void onOutTimerTimeout();
    //void onOutAnimFinished();

private:
    void initUI();
    //void initAnimations();
    void initTimers();
    
    void updateContent();
    void processActions();
    void processIconData();

    //helper function
    bool containsMouse() const;
    void saveImg(const QImage &image);
    const QPixmap converToPixmap(const QDBusArgument &value);

private:
    NotificationEntity *m_entity;
    QString m_defaultAction;

    // UI components
    AppIcon *m_icon = nullptr;
    AppBody *m_body = nullptr;
    ActionButton *m_actionButton = nullptr;

    //QPropertyAnimation *m_outAnimation = nullptr;
    //QPropertyAnimation *m_moveAnimation = nullptr;

    //Timer
    QTimer *m_outTimer = nullptr;
    QTimer *m_quitTimer;

    //Helper
    DPlatformWindowHandle *m_handle;
    DWindowManagerHelper *m_wmHelper;
};

#endif // BUBBLE_H
