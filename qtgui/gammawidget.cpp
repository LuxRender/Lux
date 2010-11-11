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
	ui->CRF_label->setStyleSheet(QString::fromUtf8(" QFrame {\n""background-color: rgb(250, 250, 250)\n""}"));
#if defined(__APPLE__)
	ui->pushButton_loadCRF->setFont(QFont  ("Lucida Grande", 11));
#endif
}

GammaWidget::~GammaWidget()
{
}

void GammaWidget::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::EnabledChange) {
		//// Reset from film when enabling in case values were not properly initialized
		//if (retrieveParam(false, LUX_FILM, LUX_FILM_TORGB_GAMMA) == m_TORGB_gamma)
		//	resetFromFilm(false);
		
		updateParam(LUX_FILM, LUX_FILM_TORGB_GAMMA, (this->isEnabled() ? m_TORGB_gamma : 1.0));
		updateParam(LUX_FILM, LUX_FILM_CAMERA_RESPONSE_ENABLED, this->isEnabled() && m_CRF_enabled);
		emit valuesChanged ();
		
		if(this->isEnabled() && m_CRF_enabled)
			activateCRF();
		else
			deactivateCRF();
	}
}

void GammaWidget::updateWidgetValues()
{
	// Gamma widgets
	updateWidgetValue(ui->slider_gamma, (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * m_TORGB_gamma));
	updateWidgetValue(ui->spinBox_gamma, m_TORGB_gamma);
	updateWidgetValue(ui->checkBox_CRF, m_CRF_enabled);
}

void GammaWidget::resetValues()
{
	m_TORGB_gamma = 2.2f;
	m_CRF_enabled = false;
	m_CRF_file = "";
}

void GammaWidget::resetFromFilm (bool useDefaults)
{
	m_CRF_enabled = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_CAMERA_RESPONSE_ENABLED) != 0.0;

	//ui->checkBox_CRF->setChecked(m_CRF_enabled);

#if defined(__APPLE__) // OSX locale is UTF-8, TODO check this for other OS
	m_CRF_file = QString::fromUtf8(luxGetStringAttribute("film", "CameraResponse"));
#else
	m_CRF_file = QString::fromAscii(luxGetStringAttribute("film", "CameraResponse"));
#endif

	// not ideal as this will reset gamma
	// which wont happen if CRF was defined in the scene file
	// and eg luxconsole is used
	if (m_CRF_enabled)
		activateCRF();
	else
		deactivateCRF();

	// gamma must be handled after CRF
	// as (de)activateCRF modifies the gamma values
	m_TORGB_gamma = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_GAMMA);

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
	
//	if (m_CRF_enabled && m_TORGB_gamma == 1.0f)
//		ui->gamma_label->setText("Gamma ( CRF-neutral )");
//	else
//		ui->gamma_label->setText("Gamma");
}
 

void GammaWidget::CRFChanged(int value)
{
	if (value == Qt::Checked)
		activateCRF();
	else
		deactivateCRF();
	
	emit valuesChanged ();
}

void GammaWidget::activateCRF()
{
	if(!m_CRF_file.isEmpty()) {
		m_CRF_enabled = true;

		QFileInfo fi(m_CRF_file);
		ui->CRF_label->setText(fi.fileName());
		// CRF should be used along with gamma 1.0
		//ui->gamma_label->setText("Gamma ( CRF-neutral )");
		//updateWidgetValue(ui->slider_gamma, (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * 1.0f) );
		//updateWidgetValue(ui->spinBox_gamma, 1.0f);
		updateWidgetValue(ui->checkBox_CRF, m_CRF_enabled);
		//updateParam (LUX_FILM, LUX_FILM_TORGB_GAMMA, 1.0f);
		
		updateParam(LUX_FILM, LUX_FILM_CAMERA_RESPONSE_ENABLED, m_CRF_enabled);

#if defined(__APPLE__) // OSX locale is UTF-8, TODO check this for other OS
		luxSetStringAttribute("film", "CameraResponse", m_CRF_file.toUtf8().data());
#else
		luxSetStringAttribute("film", "CameraResponse", m_CRF_file.toAscii().data());
#endif
	}
}

void GammaWidget::deactivateCRF()
{
	m_CRF_enabled = false;

	ui->CRF_label->setText("inactive");
	ui->gamma_label->setText("Gamma");
	updateWidgetValue(ui->slider_gamma, (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * m_TORGB_gamma) );
	updateWidgetValue(ui->spinBox_gamma, m_TORGB_gamma);
	updateWidgetValue(ui->checkBox_CRF, m_CRF_enabled);
	updateParam (LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);
	
	updateParam(LUX_FILM, LUX_FILM_CAMERA_RESPONSE_ENABLED, m_CRF_enabled);
}

void GammaWidget::loadCRF()
{
	
	m_CRF_file = QFileDialog::getOpenFileName(this, tr("Choose a CRF file to open"), m_lastOpendir, tr("Camera Response Files (*.crf *.txt)"));
    
	if(!m_CRF_file.isEmpty()) {
		ui->checkBox_CRF->setChecked(true);
		CRFChanged(Qt::Checked);
	}
}