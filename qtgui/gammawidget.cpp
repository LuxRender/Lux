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
	connect(ui->combo_CRF_List, SIGNAL(activated(QString)), this, SLOT(SetCRFPreset(QString)));

	ui->combo_CRF_List->addItem("External...");
	ui->combo_CRF_List->addItem("Advantix_100CD");
	ui->combo_CRF_List->addItem("Advantix_200CD");
	ui->combo_CRF_List->addItem("Advantix_400CD");
	ui->combo_CRF_List->addItem("Agfachrome_ctpecisa_200CD");
	ui->combo_CRF_List->addItem("Agfachrome_ctprecisa_100CD");
	ui->combo_CRF_List->addItem("Agfachrome_rsx2_050CD");
	ui->combo_CRF_List->addItem("Agfachrome_rsx2_100CD");
	ui->combo_CRF_List->addItem("Agfachrome_rsx2_200CD");
	ui->combo_CRF_List->addItem("Agfacolor_futura_100CD");
	ui->combo_CRF_List->addItem("Agfacolor_futura_200CD");
	ui->combo_CRF_List->addItem("Agfacolor_futura_400CD");
	ui->combo_CRF_List->addItem("Agfacolor_futuraII_100CD");
	ui->combo_CRF_List->addItem("Agfacolor_futuraII_200CD");
	ui->combo_CRF_List->addItem("Agfacolor_futuraII_400CD");
	ui->combo_CRF_List->addItem("Agfacolor_hdc_100_plusCD");
	ui->combo_CRF_List->addItem("Agfacolor_hdc_200_plusCD");
	ui->combo_CRF_List->addItem("Agfacolor_hdc_400_plusCD");
	ui->combo_CRF_List->addItem("Agfacolor_optimaII_100CD");
	ui->combo_CRF_List->addItem("Agfacolor_optimaII_200CD");
	ui->combo_CRF_List->addItem("Agfacolor_ultra_050_CD");
	ui->combo_CRF_List->addItem("Agfacolor_vista_100CD");
	ui->combo_CRF_List->addItem("Agfacolor_vista_200CD");
	ui->combo_CRF_List->addItem("Agfacolor_vista_400CD");
	ui->combo_CRF_List->addItem("Agfacolor_vista_800CD");
	ui->combo_CRF_List->addItem("Ektachrome_100_plusCD");
	ui->combo_CRF_List->addItem("Ektachrome_100CD");
	ui->combo_CRF_List->addItem("Ektachrome_320TCD");
	ui->combo_CRF_List->addItem("Ektachrome_400XCD");
	ui->combo_CRF_List->addItem("Ektachrome_64CD");
	ui->combo_CRF_List->addItem("Ektachrome_64TCD");
	ui->combo_CRF_List->addItem("Ektachrome_E100SCD");
	ui->combo_CRF_List->addItem("F125CD");
	ui->combo_CRF_List->addItem("F250CD");
	ui->combo_CRF_List->addItem("F400CD");
	ui->combo_CRF_List->addItem("FCICD");
	ui->combo_CRF_List->addItem("Gold_100CD");
	ui->combo_CRF_List->addItem("Gold_200CD");
	ui->combo_CRF_List->addItem("Kodachrome_200CD");
	ui->combo_CRF_List->addItem("Kodachrome_25CD");
	ui->combo_CRF_List->addItem("Kodachrome_64CD");
	ui->combo_CRF_List->addItem("Max_Zoom_800CD");
	ui->combo_CRF_List->addItem("Portra_100TCD");
	ui->combo_CRF_List->addItem("Portra_160NCCD");
	ui->combo_CRF_List->addItem("Portra_160VCCD");
	ui->combo_CRF_List->addItem("Portra_400NCCD");
	ui->combo_CRF_List->addItem("Portra_400VCCD");
	ui->combo_CRF_List->addItem("Portra_800CD");
	

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

		updateWidgetValue(ui->checkBox_CRF, m_CRF_enabled);
		
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
		QFileInfo info(m_CRF_file);
		m_lastOpendir = info.absolutePath();

		ui->checkBox_CRF->setChecked(true);
		CRFChanged(Qt::Checked);
	}
}

void GammaWidget::SetCRFPreset( QString sOption )
{
	if ( ui->combo_CRF_List->currentIndex() == 0 )
	{
		loadCRF();

		if( !m_CRF_file.isEmpty() )
		{
			ui->combo_CRF_List->addItem(m_CRF_file);
			ui->combo_CRF_List->setCurrentIndex( ui->combo_CRF_List->count() - 1 );
		}
	}
	else
	{
		m_CRF_file = sOption;		
	}
	
	if( !m_CRF_file.isEmpty() )
	{
		activateCRF();
	}
}


