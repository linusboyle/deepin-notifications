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

#ifndef BUBBLEMANAGER_H
#define BUBBLEMANAGER_H

#include <QStringList>
#include <QVariantMap>
#include <QDesktopWidget>
#include <QApplication>
#include <QGuiApplication>
#include "dbusdock_interface.h"
#include <com_deepin_dde_daemon_dock.h>
#include "bubblescrollarea.h"

using DockDaemonInterface =  com::deepin::dde::daemon::Dock;

class DBusControlCenter;
class DBusDaemonInterface;
class Login1ManagerInterface;
class DBusDockInterface;
class Persistence;

class BubbleManager : public QObject
{
    Q_OBJECT
public:
    explicit BubbleManager(QObject *parent = 0);
    ~BubbleManager();

    /*  Specification: 
        1 - The notification expired.
        2 - The notification was dismissed by the user.
        3 - The notification was closed by a call to CloseNotification slot.
        4 - Undefined/reserved reasons.
    */

    //enum ClosedReason {
        //Expired = 1,
        //Dismissed = 2,
        //Closed = 3,
        //Unknown = 4
    //};

    enum DockPosition {
        Top = 0,
        Right = 1,
        Bottom = 2,
        Left = 3
    };

signals:
    /* 
     * Standard Notifications dbus implementation
     */

    //inform external caller user has chosen certain action
    void ActionInvoked(uint id, const QString& key);

    //inform external caller a notification is closed
    void NotificationClosed(uint id, uint reason);

    /* Extra DBus APIs
     */

    //inform a record of notification added
    void RecordAdded(const QString &);

    /* Internal Signal
     */

    //request the bubblelayout to create and add a bubble
    void toAddNotification(NotificationEntity* entity);
    //request the bubblelayout to delete the bubble associated with uid ID
    void toCloseNotification(uint ID,uint reason);
    
public slots:

    /* Standard Notifications dbus implementation
     */

    // Handle external request for closing notification
    void CloseNotification(uint id);
    // List supported features of this implementation
    // TODO
    // the features need updating
    QStringList GetCapabilities();
    // Give server/daemon info about this implementation 
    QString GetServerInformation(QString &, QString &, QString &);
    // Handle new notify request
    uint Notify(const QString &, uint replacesId, const QString &, const QString &, const QString &, const QStringList &, const QVariantMap, int);

    // Extra DBus APIs - Record Related
    QString GetAllRecords();
    QString GetRecordById(const QString &id);
    QString GetRecordsFromId(int rowCount, const QString &offsetId);
    void RemoveRecord(const QString &id);
    void ClearRecords();

private slots:
    void onRecordAdded(NotificationEntity *entity);
    void onBubbleActionTriggered(uint ID,const QString& key);

    //void onCCDestRectChanged(const QRect &destRect);
    //void onDockRectChanged(const QRect &geometry);
    void onDockPositionChanged(int position);
    //void onDbusNameOwnerChanged(QString, QString, QString);

    void onPrepareForSleep(bool);

private:
    void registerAsService();

    bool checkDockExistence();
    bool checkControlCenterExistence();

    //int getX();
    //int getY();

    // return geometry of the containing specified point screen,
    // and return true if primary-screen and specified-point-screen are the same screen,
    // or return false.
    //QPair<QRect, bool> screensInfo(const QPoint &point) const;

    //void bindControlCenterX();

private:
    BubbleScrollArea* m_area;

    Persistence *m_persistence;

    //interfaces
    DBusControlCenter *m_dbusControlCenter;
    DBusDaemonInterface *m_dbusDaemonInterface;
    Login1ManagerInterface *m_login1ManagerInterface;
    DBusDockInterface *m_dbusdockinterface;
    DockDaemonInterface *m_dockDeamonInterface;

    QRect m_ccGeometry;
    QRect m_dockGeometry;

    DockPosition m_dockPosition;
};

#endif // BUBBLEMANAGER_H
