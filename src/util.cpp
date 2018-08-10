/*
 * Copyright (C) 2014 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     kirigaya <kirigaya@mkacg.com>
 *             linusboyle <linusboyle@gmail.com>
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

#include "util.h"

#include <QDesktopWidget>
#include <QApplication>
#include <QXmlStreamReader>

bool util::onPrimaryScreen(){
    QDesktopWidget* desktop = qApp->desktop();
    int primaryScreen = desktop->primaryScreen();
    int pointerScreen = desktop->screenNumber(QCursor::pos());

    return primaryScreen == pointerScreen ;
}

QRect util::screenGeometry(){
    QDesktopWidget* desktop = qApp->desktop();
    int primaryScreen = desktop->primaryScreen();

    return desktop->screenGeometry(primaryScreen);
}

//TODO
//pango markup
QString util::removeHTML(const QString &source) {
    QXmlStreamReader xml(source);
    QString textString;
    while (!xml.atEnd()) {
        if ( xml.readNext() == QXmlStreamReader::Characters ) {
            textString += xml.text();
        }
    }
    return textString.isEmpty() ? source : textString;
}
