/* This file is part of the KDE project
   Copyright (C) 2000, 2009 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <kundostack.h>
#include <QObject>
class KBookmarkGroup;
class KBookmarkManager;
class QUndoCommand;
class KActionCollection;

// TODO namespacing
class CommandHistory : public QObject
{
    Q_OBJECT
public:
    CommandHistory(QObject* parent = 0);

    // Call this before putting any commands into the history!
    void setBookmarkManager(KBookmarkManager* manager);
    KBookmarkManager* bookmarkManager();

    void createActions(KActionCollection *collection);

    virtual ~CommandHistory() {}

    void notifyDocSaved();

    void clearHistory();
    void addCommand(QUndoCommand *);

Q_SIGNALS:
    void notifyCommandExecuted(const KBookmarkGroup&);

public Q_SLOTS:
    void undo();
    void redo();

private:
    void commandExecuted(const QUndoCommand *k);

private:
    KBookmarkManager* m_manager;
    // Ported from K3Command/K3CommandHistory to QUndoCommand/KUndoStack for KDE-4.4.0
    KUndoStack m_commandHistory;
};

#endif /* COMMANDHISTORY_H */
