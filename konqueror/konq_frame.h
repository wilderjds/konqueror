/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_frame_h__
#define __konq_frame_h__

#include <qlayout.h>
#include <qpixmap.h>

#include <kpixmap.h>

#include <opFrame.h>

enum KonqFrameHeaderLook{
  PLAIN,
  H_SHADED,
  V_SHADED,
  PIXMAP
};

class KonqFrame;

class KonqFrameHeader : public QWidget
{
  Q_OBJECT

public:
  KonqFrameHeader( KonqFrame *_parent = 0L, const char *_name = 0L );
  ~KonqFrameHeader() {}

signals:
  void headerClicked();

protected: 
  virtual void paintEvent( QPaintEvent* );
  virtual void mousePressEvent( QMouseEvent* );

  void gradientFill(KPixmap &pm, QColor ca, QColor cb,bool vertShaded);

  KonqFrame* m_pParentKonqFrame;

private:
  KonqFrameHeaderLook frameHeaderLook;
  //int titleAnimation;
  //bool framedActiveTitle;
  //bool pixmapUnderTitleText;
  QPixmap* frameHeaderActive;
  QPixmap* frameHeaderInactive;
  KPixmap activeShadePm;
  KPixmap inactiveShadePm;
  QColor frameHeaderBlendActive;
  QColor frameHeaderBlendInactive;

};

class KonqFrame : public OPFrame
{
  Q_OBJECT

public:
  KonqFrame( QWidget *_parent = 0L, const char *_name = 0L );
  ~KonqFrame() {}
 
public slots:  

  /**
   * Is called when the frame header has been clicked
   */
  void slotHeaderClicked();

protected:
  virtual void resizeEvent( QResizeEvent* );
  virtual void paintEvent( QPaintEvent* );

  KonqFrameHeader* m_pHeader;
};

#endif
