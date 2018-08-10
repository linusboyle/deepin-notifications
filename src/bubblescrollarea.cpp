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

#include "bubblescrollarea.h"
#include "notificationentity.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

#include <QtMath>

//margin around the screen bounding rect
//mainly for topbar
static const int m_padding=25;
//spacing between bubbles
static const int m_spacing=3;
//and the golden ratio :)
static const double m_ratio=0.618;

BubbleScrollArea::BubbleScrollArea(QWidget* parent):
    DScrollArea(parent)
{
    setObjectName("bubbleScrollArea");
    setWindowFlags(Qt::X11BypassWindowManagerHint|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFrameShape(QFrame::NoFrame);

    setAutoHideScrollBar(false);

    m_layout=new BubbleScrollLayout(this);

    setWidget(m_layout);
    setMaximumWidth(BubbleWidth);
    setMinimumWidth(BubbleWidth);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(m_layout,&BubbleScrollLayout::heightChanged,this,&BubbleScrollArea::onHeightChanged);

    //quit the application when all notifications are closed
    //this is mainly to avoid occupying too much memory(because notificationentity
    //is deleted only when the bubblemanager is deleted,i.e,when app quits)
    //if this causes low speed or performance problem,
    //change 'close' to 'hide'
    connect(m_layout,&BubbleScrollLayout::allNotificationsClosed,this,&BubbleScrollArea::close);
    connect(m_layout,&BubbleScrollLayout::notificationShow,this,&BubbleScrollArea::show);
}

void BubbleScrollArea::onHeightChanged(int newHeight, const QRect &boundingRect){
    if (newHeight==0) {
        this->hide();
        return;
    }
    if (newHeight == -1){
        newHeight=height();
    }

    //get the screen rect
    //TODO
    //move all the codes intergrated with dde here 
    //to make it show correctly with dock and control
    //center
    
    //QDesktopWidget* desktop=QApplication::desktop();    
    //int screen=desktop->primaryScreen();
    
    //First,set the width
    QRect screenArea=boundingRect;
    screenArea-=QMargins(m_padding,m_padding,m_padding,m_padding);

    QRect scrollArea=screenArea;
    scrollArea.setWidth(BubbleWidth);

    //second,set height according to bubblelayout height
    //NOTE
    //only show part of the screen
    //just my personal taste
    //and maybe avoid notifications filling all the desktop space

    scrollArea.setHeight(qMin(qFloor(screenArea.height()*m_ratio),newHeight));

    //if(screenArea.height()/2 > newHeight){
        //scrollArea.setHeight(newHeight);
    //}
    //else{
        //scrollArea.setHeight(height()/2);    
    //}

    //put it at right side of screen (and top,of course)
    scrollArea.moveRight(screenArea.right());
    setGeometry(scrollArea);
    
    //and always show the latest notifications
    ensureVisible(0,newHeight,0,0);
}

BubbleScrollLayout::BubbleScrollLayout(QWidget* parent):
    QWidget(parent),
    m_layout(new QVBoxLayout(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::X11BypassWindowManagerHint|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);

    m_layout->setMargin(0);
    m_layout->setSpacing(m_spacing);
    setLayout(m_layout);
}

QRect BubbleScrollLayout::getScreenRect(){
    //NOTE
    QDesktopWidget* desktop=qApp->desktop();
    int screenRect = desktop->primaryScreen();
    
    return desktop->screenGeometry(screenRect);
}

void BubbleScrollLayout::addBubble(NotificationEntity* entity){

    //if already has the same uid,change the content of it;
    if(m_bubbles.contains(entity->id())){
        //IDEA
        //I'm wondering if there will be possibility of segv,as there is no lock
        //to prevent the timer to call the eraseBubble method and delete it
        //should we cease the timer when we do this?
        m_bubbles.value(entity->id())->setEntity(entity);
        return;
    }

    //else if the request have replaceID,check for the existence of the id,and potentially replace it
    if(entity->replacesId()!="0"&&m_bubbles.contains(entity->replacesId().toUInt())){
        m_bubbles.value(entity->replacesId().toUInt())->setEntity(entity);
        return;
    }// Theses two circumstance have no need for recalculation of height;

    //else,create a new bubble
    Bubble* _bubble=new Bubble(entity);

    //handled by bubblelayout
    connect(_bubble,&Bubble::expired,this,&BubbleScrollLayout::onBubbleTimeout);
    connect(_bubble,&Bubble::dismissed,this,&BubbleScrollLayout::onBubbleDismissed);
    //directly passed to and handled by bubblemanager
    connect(_bubble,&Bubble::actionInvoked,this,&BubbleScrollLayout::onBubbleActionTriggered);

    m_bubbles[entity->id()] = _bubble;
    m_layout->addWidget(_bubble);

    //update size
    calcHeight();

    show();

    //and inform the scrollarea to showup
    emit notificationShow();
}

void BubbleScrollLayout::onBubbleTimeout(uint ID){
#ifdef QT_DEBUG
       if(!m_bubbles.contains(ID)){
           qDebug()<<"the instance "<<ID<< "not avaliable when timeout";
       }
#endif
    eraseBubble(ID,1);
}

void BubbleScrollLayout::onBubbleDismissed(uint ID){
#ifdef QT_DEBUG
       if(!m_bubbles.contains(ID)){
           qDebug()<<"the instance "<<ID<< "not avaliable when dismissed";
       }
#endif
    eraseBubble(ID,2);
}

void BubbleScrollLayout::onBubbleActionTriggered(uint ID,const QString& key){
    emit actionInvoked(ID,key);
}

void BubbleScrollLayout::calcHeight(){
    int sumHeight=0;
    QHashIterator<uint,Bubble*> iterator(m_bubbles);
    while(iterator.hasNext()){
        iterator.next();
        //TODO
        //add space between two bubbles
        sumHeight+=iterator.value()->height()+m_spacing*2;
    }

    resize(BubbleWidth,sumHeight);

//#ifdef QT_DEBUG
//  qDebug()<<QString("size of bubblelayout on height change: ")+QString::number(this->width())+QString("x")+QString::number(this->height());
//#endif

    emit heightChanged(sumHeight,BubbleScrollLayout::getScreenRect());
}

void BubbleScrollLayout::eraseBubble(uint ID,uint reason){
    Bubble* _bubble = m_bubbles.take(ID);
    int index=m_layout->indexOf(_bubble);

#ifdef QT_DEBUG
    if(index == -1){
        qDebug()<<"the bubble is not in the layout";
    }
#endif

    delete m_layout->takeAt(index);
    //_bubble->deleteLater();
    emit notificationClosed(ID,reason);


    if(m_bubbles.count()==0){
        emit allNotificationsClosed();
    }
    calcHeight();
}
