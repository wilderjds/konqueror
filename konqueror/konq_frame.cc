/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <math.h>

#include <qpainter.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qwhatsthis.h>

#include <kapp.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kpixmap.h>
#include <kprogress.h>
#include <klocale.h>
#include <kseparator.h>

#include <kparts/browserextension.h>
#include <kparts/event.h>
#include "konq_frame.h"
#include "konq_childview.h"
#include "konq_viewmgr.h"
#include "konq_mainview.h"

#include <assert.h>

//these both have white background
//#include "anchor.xpm"
//#include "white.xpm"
#include "green.xpm"
//these both have very bright grey background
#include "lightgrey.xpm"
#include "anchor_grey.xpm"

#define DEFAULT_HEADER_HEIGHT 13

void KonqCheckBox::paintEvent(QPaintEvent *)
{
    static QPixmap anchor_grey(anchor_grey_xpm);
    static QPixmap lightgrey(lightgrey_xpm);

   QPainter p(this);

   if (isOn() || isDown())
      p.drawPixmap(0,0,anchor_grey);
   else
      p.drawPixmap(0,0,lightgrey);
}

KonqFrameStatusBar::KonqFrameStatusBar( KonqFrame *_parent, const char *_name )
:QWidget( _parent, _name )
,m_pParentKonqFrame( _parent )
,m_yOffset(0)
,m_showLed(true)
{
   m_pLinkedViewCheckBox = new KonqCheckBox( this, "m_pLinkedViewCheckBox" );
   m_pLinkedViewCheckBox->show();
   QWhatsThis::add( m_pLinkedViewCheckBox,
                    i18n("Checking this box on at least two views sets those views as 'linked'. "
                         "Then, when you change directories in one view, the other views "
                         "linked with it will automatically update to show the current directory. "
                         "This is especially useful with different types of views, such as a "
                         "directory tree with an icon view or detailed view, and possibly a "
                         "terminal emulator window." ) );

   m_pStatusLabel = new QLabel( this );
   m_pStatusLabel->show();
   m_pStatusLabel->installEventFilter(this);

   int h=fontMetrics().height()+2;
   if (h<DEFAULT_HEADER_HEIGHT ) h=DEFAULT_HEADER_HEIGHT;
   setFixedHeight(h);
   m_yOffset=(h-13)/2;

   m_pLinkedViewCheckBox->setGeometry(20,m_yOffset,13,13);
   m_pLinkedViewCheckBox->setFocusPolicy(NoFocus);
   m_pStatusLabel->setGeometry(40,0,50,h);

   connect(m_pLinkedViewCheckBox,SIGNAL(toggled(bool)),this,SIGNAL(linkedViewClicked(bool)));

   m_progressBar = new KProgress( 0, 100, 0, KProgress::Horizontal, this );
   m_progressBar->hide();
  //m_statusBar->insertWidget( m_progressBar, 120, STATUSBAR_LOAD_ID );
}

void KonqFrameStatusBar::resizeEvent( QResizeEvent* )
{
   m_progressBar->setGeometry( width()-160, 0, 140, height() );
}

void KonqFrameStatusBar::mousePressEvent( QMouseEvent* event )
{
   QWidget::mousePressEvent( event );
   if ( !m_pParentKonqFrame->childView()->passiveMode() )
   {
      emit clicked();
      update();
   }
   if (event->button()==RightButton)
      splitFrameMenu();
}

void KonqFrameStatusBar::splitFrameMenu()
{
   KActionCollection *actionColl = m_pParentKonqFrame->childView()->mainView()->actionCollection();

   QPopupMenu menu;

   actionColl->action( "splitviewh" )->plug( &menu );
   actionColl->action( "splitviewv" )->plug( &menu );
   actionColl->action( "splitwindowh" )->plug( &menu );
   actionColl->action( "splitwindowv" )->plug( &menu );
   actionColl->action( "removeview" )->plug( &menu );

   menu.exec(QCursor::pos());
}

bool KonqFrameStatusBar::eventFilter(QObject*,QEvent *e)
{
   if (e->type()==QEvent::MouseButtonPress)
   {
      emit clicked();
      update();
      if ( ((QMouseEvent*)e)->button()==RightButton)
         splitFrameMenu();
      return TRUE;
   }
   return FALSE;
}

void KonqFrameStatusBar::slotDisplayStatusText(const QString& text)
{
   //kdDebug(1202)<<"KongFrameHeader::slotDisplayStatusText("<<text<<")"<<endl;
   m_pStatusLabel->resize(fontMetrics().width(text),13);
   m_pStatusLabel->setText(text);
}

void KonqFrameStatusBar::slotLoadingProgress( int percent )
{
  //m_iProgress = percent;
  if ( percent != -1 )
  {
    if ( !m_progressBar->isVisible() )
      m_progressBar->show();
  }
  else
    m_progressBar->hide();

  m_progressBar->setValue( percent );
  //m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
  //m_statusBar->changeItem( 0L, STATUSBAR_MSG_ID );
}

void KonqFrameStatusBar::slotSpeedProgress( int bytesPerSecond )
{
  QString sizeStr;

  if ( bytesPerSecond > 0 )
    sizeStr = KIO::convertSize( bytesPerSecond ) + QString::fromLatin1( "/s" );
  else
    sizeStr = i18n( "stalled" );

  //m_statusBar->changeItem( sizeStr, STATUSBAR_SPEED_ID );
  slotDisplayStatusText( sizeStr ); // let's share the same label...
}

void KonqFrameStatusBar::slotConnectToNewView(KonqChildView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *newOne)
{
   if (newOne!=0)
      connect(newOne,SIGNAL(setStatusBarText(const QString &)),this,SLOT(slotDisplayStatusText(const QString&)));
   slotDisplayStatusText( QString::null );
}

void KonqFrameStatusBar::showStuff()
{
    m_pLinkedViewCheckBox->show();
    m_showLed = true;
    repaint();
}

void KonqFrameStatusBar::hideStuff()
{
    m_pLinkedViewCheckBox->hide();
    m_showLed = false;
    repaint();
}

void KonqFrameStatusBar::setLinkedView( bool b )
{
  m_pLinkedViewCheckBox->setChecked( b );
}

void KonqFrameStatusBar::paintEvent(QPaintEvent* e)
{
    static QPixmap green(green_xpm);
    static QPixmap lightgrey(lightgrey_xpm);

   if (!isVisible()) return;
   QWidget::paintEvent(e);
   if (!m_showLed) return;
   bool hasFocus = m_pParentKonqFrame->isActivePart();
   // Can't happen
   //   if ( m_pParentKonqFrame->childView()->passiveMode() )
   //   hasFocus = false;
   QPainter p(this);
   if (hasFocus)
      p.drawPixmap(4,m_yOffset,green);
   else
   {
      p.drawPixmap(4,m_yOffset,lightgrey);
   };
}

//###################################################################

KonqFrame::KonqFrame( KonqFrameContainer *_parentContainer, const char *_name )
:QWidget(_parentContainer,_name)
{
   m_pLayout = 0L;
   m_pChildView = 0L;

   // add the frame statusbar to the layout
   m_pStatusBar = new KonqFrameStatusBar( this, "KonquerorFrameStatusBar");
   connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL( linkedViewClicked( bool ) ), this, SLOT( slotLinkedViewClicked( bool ) ) );
   m_separator = 0;

#ifdef METAVIEWS
   m_metaViewLayout = 0;
   m_metaViewFrame = new QFrame( this, "metaviewframe" );
   m_metaViewFrame->show();
#endif
}

KonqFrame::~KonqFrame()
{
  kdDebug(1202) << "KonqFrame::~KonqFrame() " << this << endl;
  //delete m_pLayout;
}

KParts::ReadOnlyPart * KonqFrame::view()
{
  return m_pView;
}

bool KonqFrame::isActivePart()
{
  return ( m_pChildView &&
           static_cast<KonqChildView*>(m_pChildView) == m_pChildView->mainView()->currentChildView() );
}

void KonqFrame::listViews( ChildViewList *viewList )
{
  viewList->append( childView() );
}

void KonqFrame::saveConfig( KConfig* config, const QString &prefix, int /*id*/, int /*depth*/ )
{
  config->writeEntry( QString::fromLatin1( "URL" ).prepend( prefix ), childView()->url().url() );
  config->writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), childView()->serviceType() );
  config->writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), childView()->service()->name() );
  config->writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), childView()->passiveMode() );
  config->writeEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), childView()->linkedView() );
}

KParts::ReadOnlyPart *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
   KonqViewFactory factory( viewFactory );

   // Note that we set the parent to 0.
   // We don't want that deleting the widget deletes the part automatically
   // because we already have that taken care of in KParts...

#ifdef METAVIEWS
   m_pView = factory.create( m_metaViewFrame, "childview widget", 0, "child view" );
#else
   m_pView = factory.create( this, "childview widget", 0, "child view" );
#endif

   assert( m_pView->widget() );

   attachInternal();

   m_pStatusBar->slotConnectToNewView(0, 0,m_pView);
   return m_pView;
}

void KonqFrame::attachInternal()
{
   kdDebug(1202) << "KonqFrame::attachInternal()" << endl;
   if (m_pLayout) delete m_pLayout;

   m_pLayout = new QVBoxLayout( this, 0, -1, "KonqFrame's QVBoxLayout" );

#ifdef METAVIEWS
   if ( m_metaViewLayout ) delete m_metaViewLayout;
   m_metaViewFrame->setFrameStyle( QFrame::NoFrame );
   m_metaViewFrame->setLineWidth( 0 );
   //   m_metaViewFrame->setFrameStyle( QFrame::Panel | QFrame::Raised );
   //      m_metaViewFrame->setLineWidth( 50 );

   m_metaViewLayout = new QVBoxLayout( m_metaViewFrame );
   m_metaViewLayout->setMargin( m_metaViewFrame->frameWidth() );
   m_metaViewLayout->addWidget( m_pView->widget() );

   m_pLayout->addWidget( m_metaViewFrame );
#else
   m_pLayout->addWidget( m_pView->widget() );
#endif

   m_pLayout->addWidget( m_pStatusBar );
   m_pView->widget()->show();
   if ( m_pChildView->mainView()->fullScreenMode() )
     m_pChildView->mainView()->attachToolbars( this );
   else
     m_pStatusBar->show();
   m_pLayout->activate();
}

void KonqFrame::setChildView( KonqChildView* child )
{
   m_pChildView = child;
   if (m_pChildView)
   {
     connect(m_pChildView,SIGNAL(sigViewChanged(KonqChildView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)),
             m_pStatusBar,SLOT(slotConnectToNewView(KonqChildView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)));
     //connect(m_pChildView->view(),SIGNAL(setStatusBarText(const QString &)),
     //m_pHeader,SLOT(slotDisplayStatusText(const QString&)));
   }
};

KonqFrameContainer* KonqFrame::parentContainer()
{
   if( parentWidget()->isA("KonqFrameContainer") )
      return static_cast<KonqFrameContainer*>(parentWidget());
   else
      return 0L;
}

void KonqFrame::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
   QWidget::reparent( parent, p, showIt );
}

void KonqFrame::slotStatusBarClicked()
{
  if ( !isActivePart() )
     m_pChildView->mainView()->viewManager()->setActivePart( m_pView );
}

void KonqFrame::slotLinkedViewClicked( bool mode )
{
  if (mode && m_pChildView->mainView()->viewCount() == 2)
  {
    // Two views : link both
    KonqMainView::MapViews mapViews = m_pChildView->mainView()->viewMap();
    KonqMainView::MapViews::Iterator it = mapViews.begin();
    (*it)->setLinkedView( mode );
    ++it;
    (*it)->setLinkedView( mode );
  }
  else // Normal case : just this view
    m_pChildView->setLinkedView( mode );
}

void
KonqFrame::paintEvent( QPaintEvent* )
{
   m_pStatusBar->repaint();
}

void KonqFrame::detachMetaView()
{
  if ( m_separator )
    delete m_separator;
  m_separator = 0;
}

#ifdef METAVIEWS
void KonqFrame::attachMetaView( KParts::ReadOnlyPart *view, bool enableMetaViewFrame, const QMap<QString,QVariant> &framePropertyMap )
{
//  m_separator = new KSeparator( this );
//  m_pLayout->insertWidget( 0, m_separator );
//  m_pLayout->insertWidget( 0, view->widget() );
  m_metaViewLayout->insertWidget( 0, view->widget() );
  if ( enableMetaViewFrame )
  {
    QMapConstIterator<QString,QVariant> it = framePropertyMap.begin();
    QMapConstIterator<QString,QVariant> end = framePropertyMap.end();
    for (; it != end; ++it )
      m_metaViewFrame->setProperty( it.key(), it.data() );

    m_metaViewLayout->setMargin( m_metaViewFrame->frameWidth() );
  }
}
#else
void KonqFrame::attachMetaView( KParts::ReadOnlyPart *, bool, const QMap<QString,QVariant> & )
{
  kdError(1202) << "Meta views not supported" << endl;
}
#endif

//###################################################################

KonqFrameContainer::KonqFrameContainer( Orientation o,
					QWidget* parent,
					const char * name)
  : QSplitter( o, parent, name)
{
  m_pFirstChild = 0L;
  m_pSecondChild = 0L;
}

KonqFrameContainer::~KonqFrameContainer()
{
    kdDebug(1202) << "KonqFrameContainer::~KonqFrameContainer() " << this << " - " << className() << endl;
}

void KonqFrameContainer::listViews( ChildViewList *viewList )
{
   if( m_pFirstChild )
      m_pFirstChild->listViews( viewList );

   if( m_pSecondChild )
      m_pSecondChild->listViews( viewList );
}

void KonqFrameContainer::saveConfig( KConfig* config, const QString &prefix, int id, int depth )
{
  int idSecond = id + (int)pow( 2, depth );

  //write children sizes
  config->writeEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ), sizes() );

  //write children
  QStringList strlst;
  if( firstChild() )
    strlst.append( firstChild()->frameType() + QString("%1").arg(idSecond - 1) );
  if( secondChild() )
    strlst.append( secondChild()->frameType() + QString("%1").arg( idSecond ) );

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  //write orientation
  QString o;
  if( orientation() == Qt::Horizontal )
    o = QString::fromLatin1("Horizontal");
  else if( orientation() == Qt::Vertical )
    o = QString::fromLatin1("Vertical");
  config->writeEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ), o );


  //write child configs
  if( firstChild() ) {
    QString newPrefix = firstChild()->frameType() + QString("%1").arg(idSecond - 1);
    newPrefix.append( '_' );
    firstChild()->saveConfig( config, newPrefix, id, depth + 1 );
  }

  if( secondChild() ) {
    QString newPrefix = secondChild()->frameType() + QString("%1").arg( idSecond );
    newPrefix.append( '_' );
    secondChild()->saveConfig( config, newPrefix, idSecond, depth + 1 );
  }
}

KonqFrameBase* KonqFrameContainer::otherChild( KonqFrameBase* child )
{
   if( firstChild() == child )
      return secondChild();
   else if( secondChild() == child )
      return firstChild();
   return 0L;
}

KonqFrameContainer* KonqFrameContainer::parentContainer()
{
  if( parentWidget()->isA("KonqFrameContainer") )
    return static_cast<KonqFrameContainer*>(parentWidget());
  else
    return 0L;
}

void KonqFrameContainer::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, p, showIt );
}

void KonqFrameContainer::swapChildren()
{
  KonqFrameBase *firstCh = m_pFirstChild;
  m_pFirstChild = m_pSecondChild;
  m_pSecondChild = firstCh;
}

// Ok, here it goes. When we receive the event for a KonqFrame[Container] that has been
// deleted - it's from the QObject destructor, so isA won't work. The class name is "QObject" !
// This is why childEvent only catches child insertion, and there is a manual removeChildFrame.
void KonqFrameContainer::childEvent( QChildEvent * ce )
{
  //kdDebug(1202) << "KonqFrameContainer::childEvent this" << this << endl;

  if( ce->type() == QEvent::ChildInserted )
  {
    KonqFrameBase* castChild = 0L;
    if( ce->child()->isA("KonqFrame") )
      castChild = static_cast< KonqFrame* >(ce->child());
    else if( ce->child()->isA("KonqFrameContainer") )
      castChild = static_cast< KonqFrameContainer* >(ce->child());

    if (castChild)
    {
        kdDebug(1202) << "KonqFrameContainer " << this << ": child " << castChild << " inserted" << endl;
        if( !m_pFirstChild )
            m_pFirstChild = castChild;

        else if( !m_pSecondChild )
            m_pSecondChild = castChild;

        else
            kdWarning(1202) << this << " already has two children..."
                            << m_pFirstChild << " and " << m_pSecondChild << endl;
    }
  }
  QSplitter::childEvent( ce );
}

void KonqFrameContainer::removeChildFrame( KonqFrameBase * frame )
{
  kdDebug(1202) << "KonqFrameContainer::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;
  if( m_pFirstChild == frame )
    m_pFirstChild = 0L;

  else if( m_pSecondChild == frame )
    m_pSecondChild = 0L;

  else
    kdWarning(1202) << this << " Can't find this child:" << frame << endl;
}

#include "konq_frame.moc"
