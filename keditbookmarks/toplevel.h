/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __toplevel_h
#define __toplevel_h

#include <time.h>
#include <kmainwindow.h>
#include <kbookmark.h>
#include <qlistview.h>
#include <klistview.h>
#include <kcommand.h>

class KToggleAction;
class TestLink;

class KEBListViewItem : public QListViewItem
{
public:
    // toplevel item (there should be only one!)
    KEBListViewItem(QListView *parent, const KBookmark & group );
    // bookmark (first of its group)
    KEBListViewItem(KEBListViewItem *parent, const KBookmark & bk );
    // bookmark (after another)
    KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmark & bk );
    // group
    KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmarkGroup & gp );
    void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

    virtual void setOpen( bool );
    void modUpdate( );
    void nsPut ( QString nm);
    void setTmpStatus(QString status,  QString &oldStatus);
    void restoreStatus( QString oldStatus);

    const KBookmark & bookmark() { return m_bookmark; }

private:
    void init( const KBookmark & bk );
    void nsGet( QString &nModify );
    void nsGet( QString &nCreate, QString &nAccess, QString &nModify );

    KBookmark m_bookmark;
    int render;
};

class KEBListView : public KListView
{
    Q_OBJECT
public:
    KEBListView( QWidget * parent ) : KListView( parent ) {}
    virtual ~KEBListView() {}

public slots:
    virtual void rename( QListViewItem *item, int c );

protected:
    virtual bool acceptDrag( QDropEvent * e ) const;
    virtual QDragObject *dragObject();
};

typedef QMap<QString, QString> StringMap;

// Those methods aren't in KEBTopLevel since KEBTopLevel registers in DCOP
// and we need to create this DCOPObject after registration.
class KBookmarkEditorIface : public QObject, public DCOPObject
{
    Q_OBJECT
    K_DCOP
public:
    KBookmarkEditorIface();
k_dcop:
    void slotAddedBookmark( QString url, QString text, QString address, QString icon );
    void slotCreatedNewFolder( QString text, QString address );
signals:
    void addedBookmark( QString url, QString text, QString address, QString icon );
    void createdNewFolder( QString text, QString address );
};

// Also a DCOP object, in order to catch KBookmarkNotifier signals.
class KEBTopLevel : public KMainWindow
{
    Q_OBJECT
public:
    static KEBTopLevel * self() { return s_topLevel; }

    KEBTopLevel( const QString & bookmarksFile );
    virtual ~KEBTopLevel();

public:
    bool save();

    void setModified( bool modified = true );

    KBookmark selectedBookmark() const;

    // @return where to insert a new item - depending on the selected item
    QString insertionAddress() const;

    KEBListViewItem * findByAddress( const QString & address ) const;

    /**
     * Rebuild the whole list view from the dom document
     * Openness of folders is saved, as well as current item.
     * Call this every time the underlying qdom document is modified.
     */
    void update();

    KListView * listView() const { return m_pListView; }

    StringMap Modify;
    StringMap oldModify;
    QPtrList <TestLink> tests;

public slots:
    void slotImportNS();
    void slotExportNS();
    void slotImportMoz();
    void slotExportMoz();
    void slotSave();
    void slotSaveAs();
    void slotDocumentRestored();
    void slotCut();
    void slotCopy();
    void slotPaste();
    void slotRename();
    void slotChangeURL();
    void slotChangeIcon();
    void slotDelete();
    void slotNewFolder();
    void slotNewBookmark();
    void slotInsertSeparator();
    void slotSort();
    void slotSetAsToolbar();
    void slotOpenLink();
    void slotShowNS();
    void slotConfigureKeyBindings();
    void slotConfigureToolbars();
    void slotTestLink();
    void slotTestAllLinks();
    void slotCancelTest(TestLink *);
    void slotCancelAllTests();

protected slots:
    void slotItemRenamed(QListViewItem *, const QString &, int);
    void slotDropped(QDropEvent* , QListViewItem* , QListViewItem* );
    void slotSelectionChanged();
    void slotClipboardDataChanged();
    void slotContextMenu( KListView *, QListViewItem *, const QPoint & );
    void slotBookmarksChanged( const QString &, const QString & );
    void slotCommandExecuted();
    void slotNewToolbarConfig();

    // slots for DCOP-originated events
    void slotAddedBookmark( QString url, QString text, QString address, QString icon );
    void slotCreatedNewFolder( QString text, QString address );

protected:
    void fillGroup( KEBListViewItem * parentItem, KBookmarkGroup group );
    virtual bool queryClose();
    void fillListView();
    void pasteData( const QString & cmdName, QMimeSource * data, const QString & insertionAddress );
    void itemMoved(QListViewItem * item, const QString & newAddress, bool copy);
    QString correctAddress(QString address);

    bool m_bModified;
    bool m_bCanPaste;
    bool m_bReadOnly;
    bool m_bUnique;
    KToggleAction * m_taShowNS;
    KListView * m_pListView;
    KCommandHistory m_commandHistory;
    
    KBookmarkEditorIface * m_dcopIface;

    static KEBTopLevel * s_topLevel;

};


#endif

