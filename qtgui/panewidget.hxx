/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#ifndef PANEWIDGET_H
#define PANEWIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QLabel>

namespace Ui
{
	class PaneWidget;
	class ClickableLabel;
}

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:

	ClickableLabel(const QString& label = "", QWidget *parent = 0);
	~ClickableLabel() {};

protected:

	void mouseReleaseEvent(QMouseEvent* event);

signals:

	void clicked();

};

class PaneWidget : public QWidget
{
	Q_OBJECT

public:

	PaneWidget(QWidget *parent, const QString& label = "", const QString& icon = "");
	~PaneWidget();

	void setTitle(const QString& title);
	void setIcon(const QString& icon);

	void setWidget(QWidget *widget);
	QWidget *getWidget();

	void expand();
	void collapse();

private:

	Ui::PaneWidget *ui;

	QWidget *mainwidget;
	ClickableLabel *expandlabel;

	bool expanded;

private slots:

	void expandClicked();
  
};

#endif // PANEWIDGET_H

