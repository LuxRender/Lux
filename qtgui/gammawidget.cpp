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

#include "ui_gamma.h"
#include "gammawidget.hxx"

#include "mainwindow.hxx"

#include "api.h"

GammaWidget::GammaWidget(QWidget *parent) : QWidget(parent), ui(new Ui::GammaWidget)
{
	ui->setupUi(this);
	
	connect(ui->slider_gamma, SIGNAL(valueChanged(int)), this, SLOT(gammaChanged(int)));
	connect(ui->spinBox_gamma, SIGNAL(valueChanged(double)), this, SLOT(gammaChanged(double)));
	connect(ui->checkBox_CRF, SIGNAL(stateChanged(int)), this, SLOT(CRFChanged(int)));
	connect(ui->pushButton_loadCRF, SIGNAL(clicked()), this, SLOT(loadCRF()));
	ui->CRF_lineEdit->setText("- empty -");
}

GammaWidget::~GammaWidget()
{
}

void GammaWidget::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::EnabledChange) {
		// Reset from film when enabling in case values were not properly initialized
		if (LUX_FILM_TORGB_GAMMA == m_TORGB_gamma)
			resetFromFilm(false);
		
		updateParam(LUX_FILM, LUX_FILM_TORGB_GAMMA, (this->isEnabled() ? m_TORGB_gamma : 1.0));
		emit valuesChanged ();
	}
}

void GammaWidget::updateWidgetValues()
{
	// Gamma widgets
	updateWidgetValue(ui->slider_gamma, (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * m_TORGB_gamma));
	updateWidgetValue(ui->spinBox_gamma, m_TORGB_gamma);
}

void GammaWidget::resetValues()
{
	m_TORGB_gamma = 2.2f;
}

void GammaWidget::resetFromFilm (bool useDefaults)
{
	m_TORGB_gamma =  retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_GAMMA);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);
}

void GammaWidget::gammaChanged (int value)
{
	gammaChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE ) );
}

void GammaWidget::gammaChanged (double value)
{
	m_TORGB_gamma = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * m_TORGB_gamma);

	updateWidgetValue(ui->slider_gamma, sliderval);
	updateWidgetValue(ui->spinBox_gamma, m_TORGB_gamma);

	updateParam (LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);

	emit valuesChanged ();
}

void GammaWidget::CRFChanged(int value) // TODO: add functions
{
/*	if (value == Qt::Checked)

	else
*/
	
//	Update();
}

void GammaWidget::loadCRF() // TODO: add functions
{
	
	QString fileName = QFileDialog::getOpenFileName(this, tr("Choose a CRF file to open"), m_lastOpendir, tr("LuxRender Files (*.crf *.txt)"));
    
	if(!fileName.isNull()) {
		QFileInfo fi(fileName);
		QString name = fi.fileName();
		ui->CRF_lineEdit->setText(name);
		QString str = fileName;
		QByteArray fileName = str.toAscii();
		const char* crfName = fileName.data();
		luxSetStringAttribute("film", "CameraResponse", crfName);
	}
}
