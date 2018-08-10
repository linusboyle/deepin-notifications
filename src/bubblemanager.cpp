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

#include <QStringList>
#include <QVariantMap>
#include <QDebug>
#include "bubblemanager.h"
#include "notificationentity.h"
#include "dbuscontrol.h"
#include "dbus_daemon_interface.h"
#include "dbuslogin1manager.h"
#include "persistence.h"
#include "util.h"

BubbleManager::BubbleManager(QObject *parent)
    : QObject(parent),
    m_area(new BubbleScrollArea()),
    m_persistence(new Persistence(this)),
    m_dockPosition(DockPosition::Bottom)
{
    m_dbusDaemonInterface = new DBusDaemonInterface(DBusDaemonDBusService, DBusDaemonDBusPath,
                                                    QDBusConnection::sessionBus(), this);

    m_dbusdockinterface = new DBusDockInterface(DBusDockDBusServer, DBusDockDBusPath,
                                                QDBusConnection::sessionBus(), this);

    m_login1ManagerInterface = new Login1ManagerInterface(Login1DBusService, Login1DBusPath,
                                                          QDBusConnection::systemBus(), this);

    m_dbusControlCenter = new DBusControlCenter(ControlCenterDBusService, ControlCenterDBusPath,
                                                    QDBusConnection::sessionBus(), this);

    m_dockDeamonInterface = new DockDaemonInterface(DockDaemonDBusService, DockDaemonDBusPath,
                                            QDBusConnection::sessionBus(), this);
    m_dockDeamonInterface->setSync(false);

    connect(this,&BubbleManager::toAddNotification,m_area->m_layout,&BubbleScrollLayout::addBubble);
    connect(this,&BubbleManager::toCloseNotification,m_area->m_layout,&BubbleScrollLayout::eraseBubble);
    connect(m_area->m_layout,&BubbleScrollLayout::notificationClosed,this,&BubbleManager::NotificationClosed);
    connect(m_area->m_layout,&BubbleScrollLayout::actionInvoked,this,&BubbleManager::onBubbleActionTriggered);


    connect(m_persistence, &Persistence::RecordAdded, this, &BubbleManager::onRecordAdded);


    connect(m_login1ManagerInterface, SIGNAL(PrepareForSleep(bool)),this, SLOT(onPrepareForSleep(bool)));
    connect(m_dockDeamonInterface, &DockDaemonInterface::PositionChanged, this, &BubbleManager::onDockPositionChanged);

    //connect(m_dbusDaemonInterface, SIGNAL(NameOwnerChanged(QString, QString, QString)),
            //this, SLOT(onDbusNameOwnerChanged(QString, QString, QString)));

    //connect(m_dbusdockinterface, &DBusDockInterface::geometryChanged, this, &BubbleManager::onDockRectChanged);


    // get correct value for m_dockGeometry, m_dockPosition, m_ccGeometry
    //if (m_dbusdockinterface->isValid())
        //onDockRectChanged(m_dbusdockinterface->geometry());
    //if (m_dockDeamonInter->isValid())
        //m_dockPosition = static_cast<DockPosition>(m_dockDeamonInter->position());
    //if (m_dbusControlCenter->isValid())
        //onCCDestRectChanged(m_dbusControlCenter->rect());

    registerAsService();
}

BubbleManager::~BubbleManager()
{

}

void BubbleManager::CloseNotification(uint id)
{
    emit toCloseNotification(id,2);
}

QStringList BubbleManager::GetCapabilities()
{
    QStringList result;
    result << "action-icons" << "actions" << "body" << "body-hyperlinks" << "body-markup";

    return result;
}

QString BubbleManager::GetServerInformation(QString &name, QString &vender, QString &version)
{
    name = QString("DeepinNotifications");
    vender = QString("Deepin");
    version = QString("2.0");

    return QString("1.2");
}

uint BubbleManager::Notify(const QString &appName, uint replacesId,
                           const QString &appIcon, const QString &summary,
                           const QString &body, const QStringList &actions,
                           const QVariantMap hints, int expireTimeout)
{
    NotificationEntity *notification = new NotificationEntity(appName,0, appIcon,
                                                              summary, util::removeHTML(body), actions, hints,
                                                              QString::number(QDateTime::currentMSecsSinceEpoch()),
                                                              QString::number(replacesId),
                                                              QString::number(expireTimeout),
                                                              this);
    m_persistence->addOne(notification);

#ifdef QT_DEBUG
    qDebug() << "a new Notify:" << "appName:" + appName << "replaceID:" + QString::number(replacesId)
             << "appIcon:" + appIcon << "summary:" + summary << "body:" + body
             << "actions:" << actions << "hints:" << hints << "expireTimeout:" << expireTimeout
             << "the server allocate the id "+QString::number(notification->id())+" to this notification";
#endif

    emit toAddNotification(notification);


    // If replaces_id is 0, the return value is a UINT32 that represent the notification.
    // If replaces_id is not 0, the returned value is the same value as replaces_id.
    return replacesId == 0 ? notification->id() : replacesId;
}

QString BubbleManager::GetAllRecords()
{
    return m_persistence->getAll();
}

QString BubbleManager::GetRecordById(const QString &id)
{
    return m_persistence->getById(id);
}

QString BubbleManager::GetRecordsFromId(int rowCount, const QString &offsetId)
{
    return m_persistence->getFrom(rowCount, offsetId);
}

void BubbleManager::RemoveRecord(const QString &id)
{
    m_persistence->removeOne(id);

    QFile file(CachePath + id + ".png");
    file.remove();
}

void BubbleManager::ClearRecords()
{
    m_persistence->removeAll();

    QDir dir(CachePath);
    dir.removeRecursively();
}

void BubbleManager::onRecordAdded(NotificationEntity *entity)
{
    QJsonObject notifyJson
    {
        {"name", entity->appName()},
        {"icon", entity->appIcon()},
        {"summary", entity->summary()},
        {"body", entity->body()},
        {"id", QString::number(entity->id())},
        {"time", entity->ctime()}
    };
    QJsonDocument doc(notifyJson);
    QString notify(doc.toJson(QJsonDocument::Compact));

    Q_EMIT RecordAdded(notify);
}

void BubbleManager::registerAsService()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.interface()->registerService(NotificationsDBusService,
                                                  QDBusConnectionInterface::ReplaceExistingService,
                                                  QDBusConnectionInterface::AllowReplacement);
    connection.registerObject(NotificationsDBusPath, this);

    QDBusConnection ddenotifyConnect = QDBusConnection::sessionBus();
    ddenotifyConnect.interface()->registerService(DDENotifyDBusServer,
                                                  QDBusConnectionInterface::ReplaceExistingService,
                                                  QDBusConnectionInterface::AllowReplacement);
    ddenotifyConnect.registerObject(DDENotifyDBusPath, this);
}

//void BubbleManager::onCCDestRectChanged(const QRect &destRect)
//{
    //// get the current rect of control-center
    //m_ccGeometry = m_dbusControlCenter->rect();
    //// use the current rect of control-center to setup position of bubble
    //// to avoid a move-anim bug
    //m_bubble->setBasePosition(getX(), getY());

    //// use destination rect of control-center to setup move-anim
    //if (destRect.width() == 0) { // closing the control-center
        //if (m_dockPosition == DockPosition::Right) {
            //const QRect &screenRect = screensInfo(QCursor::pos()).first;
            //if ((screenRect.height() - m_dockGeometry.height()) / 2.0 < m_bubble->height()) {
                //QRect mRect = destRect;
                //mRect.setX((screenRect.right()) - m_dockGeometry.width());
                //m_bubble->resetMoveAnim(mRect);
                //return;
            //}
        //}
    //}
    //m_bubble->resetMoveAnim(destRect);
//}

void BubbleManager::onPrepareForSleep(bool sleep)
{
    // workaround to avoid the "About to suspend..." notifications still
    // hanging there on restoring from sleep confusing users.
    if (!sleep) {
        qDebug() << "Quit on restoring from sleep.";
        qApp->quit();
    }
}

bool BubbleManager::checkDockExistence()
{
    return m_dbusDaemonInterface->NameHasOwner(DBusDockDBusServer).value();
}

bool BubbleManager::checkControlCenterExistence()
{
    return m_dbusDaemonInterface->NameHasOwner(ControlCenterDBusService).value();
}

//int BubbleManager::getX()
//{
    //QPair<QRect, bool> pair = screensInfo(QCursor::pos());
    //const QRect &rect = pair.first;

    //// directly show the notify on the screen containing mouse,
    //// because dock and control-centor will only be displayed on the primary screen.
    //if (!pair.second)
        //return  rect.x() + rect.width();

    //// DBus object is invalid, return screen right
    //if (!m_dbusControlCenter->isValid() && !m_dbusdockinterface->isValid())
        //return rect.x() + rect.width();

    //// if dock dbus is valid and dock position is right
    //if (m_dbusdockinterface->isValid() && m_dockPosition == DockPosition::Right) {
        //// check dde-control-center is valid
        //if (m_dbusControlCenter->isValid()) {
            //if (m_ccGeometry.x() >  m_dockGeometry.x()) {
                //return (rect.x() + rect.width()) - m_dockGeometry.width();
            //}
        //}
        //// dde-control-center is invalid, return dock' x
        //return (rect.x() + rect.width()) - m_dockGeometry.width();
    //}
    ////  dock position is not right, and dde-control-center is valid
    //if (m_dbusControlCenter->isValid()) {
        //return m_dbusControlCenter->rect().x();
    //}

    //return rect.x() + rect.width();
//}

//int BubbleManager::getY()
//{
    //QPair<QRect, bool> pair = screensInfo(QCursor::pos());
    //const QRect &rect = pair.first;

    //[> TODO: remove <]
    //qDebug() << "screen Rect:" << rect;

    //if (!pair.second)
        //return  rect.y();

    //if (!m_dbusdockinterface->isValid())
        //return rect.y();

    //[> TODO: remove <]
    //qDebug() << "dock Rect:" << m_dockGeometry << m_dockPosition;

    //if (m_dockPosition == DockPosition::Top)
        //return m_dockGeometry.bottom();

    //return rect.y();
//}

//QPair<QRect, bool> BubbleManager::screensInfo(const QPoint &point) const
//{
    //QDesktopWidget *desktop = QApplication::desktop();
    //int pointScreen = desktop->screenNumber(point);
    //int primaryScreen = desktop->primaryScreen();

    //QRect rect = desktop->screenGeometry(pointScreen);

    //return QPair<QRect, bool>(rect, (pointScreen == primaryScreen));
//}

//void BubbleManager::onDockRectChanged(const QRect &geometry)
//{
    //m_dockGeometry = geometry;

    //m_bubble->setBasePosition(getX(), getY());
//}

void BubbleManager::onDockPositionChanged(int position)
{
    m_dockPosition = static_cast<DockPosition>(position);
}

//void BubbleManager::onDbusNameOwnerChanged(QString name, QString, QString newName)
//{
    //if (name == ControlCenterDBusService && util::onPrimaryScreen()) {
        //if (!newName.isEmpty()) {
            //bindControlCenterX();
        //}
    //}
//}

//void BubbleManager::bindControlCenterX()
//{
    //if (!m_dbusControlCenter) {
        //m_dbusControlCenter = new DBusControlCenter(ControlCenterDBusService,
                                                    //ControlCenterDBusPath,
                                                    //QDBusConnection::sessionBus(),
                                                    //this);
    //}
    //connect(m_dbusControlCenter, &DBusControlCenter::destRectChanged, this, &BubbleManager::onCCDestRectChanged);
//}

//void BubbleManager::consumeEntities()
//{
    //if (!m_currentNotify.isNull()) {
        //m_currentNotify->deleteLater();
        //m_currentNotify = nullptr;
    //}

    //if (m_entities.isEmpty()) {
        //m_currentNotify = nullptr;
        //return;
    //}

    //m_currentNotify = m_entities.dequeue();

    //QDesktopWidget *desktop = QApplication::desktop();
    //int pointerScreen = desktop->screenNumber(QCursor::pos());
    //int primaryScreen = desktop->primaryScreen();
    //QWidget *pScreenWidget = desktop->screen(primaryScreen);

    //if (checkDockExistence()) {
        //m_dockGeometry = m_dbusdockinterface->geometry();
    //}

    //if (checkControlCenterExistence())
        //m_ccGeometry = m_dbusControlCenter->rect();

    //if (checkControlCenterExistence() && pointerScreen == primaryScreen)
        //bindControlCenterX();

    //if (pointerScreen != primaryScreen)
        //pScreenWidget = desktop->screen(pointerScreen);

    //m_bubble->setBasePosition(getX(), getY(), pScreenWidget->geometry());
    //m_bubble->setEntity(m_currentNotify);
//}

void BubbleManager::onBubbleActionTriggered(uint ID,const QString& key){
    emit ActionInvoked(ID,key);
}
