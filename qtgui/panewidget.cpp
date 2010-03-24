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

#include "ui_pane.h"
#include "panewidget.hxx"

#include <iostream>

using namespace std;

ClickableLabel::ClickableLabel(const QString& label, QWidget *parent) : QLabel(label,parent) {
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent* event) 
{
	emit clicked();
}

PaneWidget::PaneWidget(QWidget *parent, const QString& label, const QString& icon) : QWidget(parent), ui(new Ui::PaneWidget)
{
	expanded = false;

	ui->setupUi(this);
	if (!icon.isEmpty())
		ui->labelPaneIcon->setPixmap(QPixmap(icon));
	
	if (!label.isEmpty())
		ui->labelPaneName->setText(label);

	expandlabel = new ClickableLabel(">", this);
	ui->gridLayout->addWidget(expandlabel, 0, 3, 1, 1);

	connect(expandlabel, SIGNAL(clicked()), this, SLOT(expandClicked()));
}

PaneWidget::~PaneWidget()
{
}

void PaneWidget::setTitle(const QString& title)
{
	ui->labelPaneName->setText(title);
}

void PaneWidget::setIcon(const QString& icon)
{
	ui->labelPaneIcon->setPixmap(QPixmap(icon));
}

void PaneWidget::expandClicked()
{
	if (expanded)
		collapse();
	else
		expand();

}

void PaneWidget::expand()
{
	expanded = true;
	expandlabel->setText("v");
	mainwidget->show();
}

void PaneWidget::collapse()
{
	expanded = false;
	expandlabel->setText(">");
	mainwidget->hide();
}

void PaneWidget::setWidget(QWidget *widget)
{
	mainwidget = widget;
	ui->paneLayout->addWidget(widget);
	mainwidget->hide();
}

QWidget *PaneWidget::getWidget()
{
	return mainwidget;
}
