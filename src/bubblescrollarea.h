/*
 * Copyright (C) 2014 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     linusboyle <linusboyle@gmail.com>     
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

#ifndef SCROLLAREA_H
#define SCROLLAREA_H

#include "dscrollarea.h"
#include "bubble.h"

class QVBoxLayout;

class BubbleScrollLayout:public QWidget{
    Q_OBJECT
public:
    BubbleScrollLayout(QWidget* parent = nullptr);
signals:
    //inform the scroll area to close,show,or resize
    void allNotificationsClosed();
    void notificationShow();
    void heightChanged(int newHeight,QRect rect);

    void notificationClosed(uint ID,uint reason);
    void actionInvoked(uint ID,const QString& key);
public slots:
    //handle external request
    void addBubble(NotificationEntity* entity);
    void eraseBubble(uint ID,uint reason);
private:
    QVBoxLayout* m_layout;
    QHash<uint,Bubble*> m_bubbles;
    //reevaluate and update the size on every addition and deletion of bubble
    void calcHeight();
    
    //Helper
    static QRect getScreenRect();
private slots:
    //handle signals from bubble
    void onBubbleTimeout(uint);
    void onBubbleDismissed(uint);
    void onBubbleActionTriggered(uint,const QString&);
};

class BubbleScrollArea: public DScrollArea{
    Q_OBJECT
    friend class BubbleManager;
public:
    BubbleScrollArea(QWidget* parent = nullptr);
private:
    BubbleScrollLayout* m_layout;
public slots:
    //change the height and reset geometry based on the boundingrect
    void onHeightChanged(int newHeight,const QRect& boundingRect);
};

#endif
