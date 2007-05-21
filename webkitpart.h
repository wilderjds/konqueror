/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
#ifndef WEBKITPART_H
#define WEBKITPART_H

#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>

class QWebPage;
class QWebFrame;
class KAboutData;
class WebKitBrowserExtension;

class WebKitPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/);

    virtual bool openUrl(const KUrl &url);
    virtual bool closeUrl();

    static KAboutData *createAboutData();

protected:
    virtual bool openFile();

private slots:
    void frameStarted(QWebFrame *frame);
    void frameFinished(QWebFrame *frame);

private:
    QWebPage *webPage;
    WebKitBrowserExtension *browserExtension;
};

class WebKitBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    WebKitBrowserExtension(WebKitPart *parent);
};

#endif // WEBKITPART_H
