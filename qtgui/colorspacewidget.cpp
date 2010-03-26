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

#include "ui_colorspace.h"
#include "colorspacewidget.hxx"

#include "mainwindow.hxx"

#include "api.h"

#include <iostream>

using namespace std;

ColorSpaceWidget::ColorSpaceWidget(QWidget *parent) : QWidget(parent), ui(new Ui::ColorSpaceWidget)
{
	ui->setupUi(this);
	
	connect(ui->comboBox_colorSpacePreset, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorSpacePreset(int)));
	connect(ui->comboBox_whitePointPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(setWhitepointPreset(int)));
	connect(ui->slider_whitePointX, SIGNAL(valueChanged(int)), this, SLOT(whitePointXChanged(int)));
	connect(ui->spinBox_whitePointX, SIGNAL(valueChanged(double)), this, SLOT(whitePointXChanged(double)));
	connect(ui->slider_whitePointY, SIGNAL(valueChanged(int)), this, SLOT(whitePointYChanged(int)));
	connect(ui->spinBox_whitePointY, SIGNAL(valueChanged(double)), this, SLOT(whitePointYChanged(double)));
	
	connect(ui->slider_redX, SIGNAL(valueChanged(int)), this, SLOT(redXChanged(int)));
	connect(ui->spinBox_redX, SIGNAL(valueChanged(double)), this, SLOT(redXChanged(double)));
	connect(ui->slider_redY, SIGNAL(valueChanged(int)), this, SLOT(redYChanged(int)));
	connect(ui->spinBox_redY, SIGNAL(valueChanged(double)), this, SLOT(redYChanged(double)));
	connect(ui->slider_blueX, SIGNAL(valueChanged(int)), this, SLOT(blueXChanged(int)));
	connect(ui->spinBox_blueX, SIGNAL(valueChanged(double)), this, SLOT(blueXChanged(double)));
	connect(ui->slider_blueY, SIGNAL(valueChanged(int)), this, SLOT(blueYChanged(int)));
	connect(ui->spinBox_blueY, SIGNAL(valueChanged(double)), this, SLOT(blueYChanged(double)));
	connect(ui->slider_greenX, SIGNAL(valueChanged(int)), this, SLOT(greenXChanged(int)));
	connect(ui->spinBox_greenX, SIGNAL(valueChanged(double)), this, SLOT(greenXChanged(double)));
	connect(ui->slider_greenY, SIGNAL(valueChanged(int)), this, SLOT(greenYChanged(int)));
	connect(ui->spinBox_greenY, SIGNAL(valueChanged(double)), this, SLOT(greenYChanged(double)));
}

ColorSpaceWidget::~ColorSpaceWidget()
{
}

void ColorSpaceWidget::updateWidgetValues()
{
	// Colorspace widgets
	updateWidgetValue(ui->slider_whitePointX, (int)((FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE) * m_TORGB_xwhite));
	updateWidgetValue(ui->spinBox_whitePointX, m_TORGB_xwhite);
	updateWidgetValue(ui->slider_whitePointY, (int)((FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE) * m_TORGB_ywhite));
	updateWidgetValue(ui->spinBox_whitePointY, m_TORGB_ywhite);

	updateWidgetValue(ui->slider_redX, (int)((FLOAT_SLIDER_RES / TORGB_XRED_RANGE) * m_TORGB_xred));
	updateWidgetValue(ui->spinBox_redX, m_TORGB_xred);
	updateWidgetValue(ui->slider_redY, (int)((FLOAT_SLIDER_RES / TORGB_YRED_RANGE) * m_TORGB_yred));
	updateWidgetValue(ui->spinBox_redY, m_TORGB_yred);

	updateWidgetValue(ui->slider_greenX, (int)((FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE) * m_TORGB_xgreen));
	updateWidgetValue(ui->spinBox_greenX, m_TORGB_xgreen);
	updateWidgetValue(ui->slider_greenY, (int)((FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE) * m_TORGB_ygreen));
	updateWidgetValue(ui->spinBox_greenY, m_TORGB_ygreen);

	updateWidgetValue(ui->slider_blueX, (int)((FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE) * m_TORGB_xblue));
	updateWidgetValue(ui->spinBox_blueX, m_TORGB_xblue);
	updateWidgetValue(ui->slider_blueY, (int)((FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE) * m_TORGB_yblue));
	updateWidgetValue(ui->spinBox_blueY, m_TORGB_yblue);
}

void ColorSpaceWidget::resetValues()
{
	m_TORGB_xwhite = 0.314275f;
	m_TORGB_ywhite = 0.329411f;
	m_TORGB_xred = 0.63f;
	m_TORGB_yred = 0.34f;
	m_TORGB_xgreen = 0.31f;
	m_TORGB_ygreen = 0.595f;
	m_TORGB_xblue = 0.155f;
	m_TORGB_yblue = 0.07f;
}

void ColorSpaceWidget::resetFromFilm (bool useDefaults)
{
	m_TORGB_xwhite = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_WHITE);
	m_TORGB_ywhite = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_WHITE);
	m_TORGB_xred = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_RED);
	m_TORGB_yred = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_RED);
	m_TORGB_xgreen = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_GREEN);
	m_TORGB_ygreen = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_GREEN);
	m_TORGB_xblue = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_BLUE);
	m_TORGB_yblue = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_BLUE);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);
}

void ColorSpaceWidget::setColorSpacePreset(int choice)
{
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(choice);
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_whitePointPreset->setCurrentIndex(0);
	ui->comboBox_colorSpacePreset->blockSignals(false);

	switch (choice) {
		case 0: {
				// sRGB - HDTV (ITU-R BT.709-5)
				m_TORGB_xwhite = 0.314275f; m_TORGB_ywhite = 0.329411f;
				m_TORGB_xred = 0.63f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.31f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 1: {
				// ROMM RGB
				m_TORGB_xwhite = 0.346f; m_TORGB_ywhite = 0.359f;
				m_TORGB_xred = 0.7347f; m_TORGB_yred = 0.2653f;
				m_TORGB_xgreen = 0.1596f; m_TORGB_ygreen = 0.8404f;
				m_TORGB_xblue = 0.0366f; m_TORGB_yblue = 0.0001f; }
			break;
		case 2: {
				// Adobe RGB 98
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.64f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.21f; m_TORGB_ygreen = 0.71f;
				m_TORGB_xblue = 0.15f; m_TORGB_yblue = 0.06f; }
			break;
		case 3: {
				// Apple RGB
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.625f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.28f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 4: {
				// NTSC (FCC 1953, ITU-R BT.470-2 System M)
				m_TORGB_xwhite = 0.310f; m_TORGB_ywhite = 0.316f;
				m_TORGB_xred = 0.67f; m_TORGB_yred = 0.33f;
				m_TORGB_xgreen = 0.21f; m_TORGB_ygreen = 0.71f;
				m_TORGB_xblue = 0.14f; m_TORGB_yblue = 0.08f; }
			break;
		case 5: {
				// NTSC (1979) (SMPTE C, SMPTE-RP 145)
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.63f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.31f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 6: {
				// PAL/SECAM (EBU 3213, ITU-R BT.470-6)
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.64f; m_TORGB_yred = 0.33f;
				m_TORGB_xgreen = 0.29f; m_TORGB_ygreen = 0.60f;
				m_TORGB_xblue = 0.15f; m_TORGB_yblue = 0.06f; }
			break;
		case 7: {
				// CIE (1931) E
				m_TORGB_xwhite = 0.333f; m_TORGB_ywhite = 0.333f;
				m_TORGB_xred = 0.7347f; m_TORGB_yred = 0.2653f;
				m_TORGB_xgreen = 0.2738f; m_TORGB_ygreen = 0.7174f;
				m_TORGB_xblue = 0.1666f; m_TORGB_yblue = 0.0089f; }
			break;
		default:
			break;
	}

	// Update values in film trough API
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);

	updateWidgetValues ();

	emit valuesChanged();
}

void ColorSpaceWidget::setWhitepointPreset(int choice)
{
	ui->comboBox_whitePointPreset->blockSignals(true);
	ui->comboBox_whitePointPreset->setCurrentIndex(choice);
	ui->comboBox_whitePointPreset->blockSignals(false);

	// first choice is "none"
	if (choice < 1)
		return;

	// default to E
	float x = 1;
	float y = 1;
	float z = 1;

	switch (choice - 1) {
		case 0: 
			{
				// A
				x = 1.09850;
				y = 1.00000;
				z = 0.35585;
			}
			break;
		case 1: 
			{
				// B
				x = 0.99072;
				y = 1.00000;
				z = 0.85223;
			}
			break;
		case 2: 
			{
				// C
				x = 0.98074;
				y = 1.00000;
				z = 1.18232;
			}
			break;
		case 3: 
			{
				// D50
				x = 0.96422;
				y = 1.00000;
				z = 0.82521;
			}
			break;
		case 4: 
			{
				// D55
				x = 0.95682;
				y = 1.00000;
				z = 0.92149;
			}
			break;
		case 5: 
			{
				// D65
				x = 0.95047;
				y = 1.00000;
				z = 1.08883;
			}
			break;
		case 6: 
			{
				// D75
				x = 0.94972;
				y = 1.00000;
				z = 1.22638;
			}
			break;
		case 7: 
			{
				// E
				x = 1.00000;
				y = 1.00000;
				z = 1.00000;
			}
			break;
		case 8: 
			{
				// F2
				x = 0.99186;
				y = 1.00000;
				z = 0.67393;
			}
			break;
		case 9: 
			{
				// F7
				x = 0.95041;
				y = 1.00000;
				z = 1.08747;
			}
			break;
		case 10: 
			{
				// F11
				x = 1.00962;
				y = 1.00000;
				z = 0.64350;
			}
			break;
		default:
			break;
	}

	m_TORGB_xwhite = x / (x+y+z);
	m_TORGB_ywhite = y / (x+y+z);

	// Update values in film trough API
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);

	updateWidgetValues ();

	emit valuesChanged();
}

void ColorSpaceWidget::whitePointXChanged(int value)
{
	whitePointXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE ) );
}

void ColorSpaceWidget::whitePointXChanged(double value)
{
	m_TORGB_xwhite = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE) * m_TORGB_xwhite);

	updateWidgetValue(ui->slider_whitePointX, sliderval);
	updateWidgetValue(ui->spinBox_whitePointX, m_TORGB_xwhite);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);

	emit valuesChanged();
}

void ColorSpaceWidget::whitePointYChanged (int value)
{
	whitePointYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE ) );
}

void ColorSpaceWidget::whitePointYChanged (double value)
{
	m_TORGB_ywhite = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE) * m_TORGB_ywhite);

	updateWidgetValue(ui->slider_whitePointY, sliderval);
	updateWidgetValue(ui->spinBox_whitePointY, m_TORGB_ywhite);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);

	emit valuesChanged();
}

void ColorSpaceWidget::redYChanged (int value)
{
	redYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YRED_RANGE ) );
}

void ColorSpaceWidget::redYChanged (double value)
{
	m_TORGB_yred = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YRED_RANGE) * m_TORGB_yred);

	updateWidgetValue(ui->slider_redY, sliderval);
	updateWidgetValue(ui->spinBox_redY, m_TORGB_yred);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);

	emit valuesChanged();
}

void ColorSpaceWidget::redXChanged (int value)
{
	redXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XRED_RANGE ) );
}

void ColorSpaceWidget::redXChanged (double value)
{
	m_TORGB_xred = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XRED_RANGE) * m_TORGB_xred);

	updateWidgetValue(ui->slider_redX, sliderval);
	updateWidgetValue(ui->spinBox_redX, m_TORGB_xred);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);

	emit valuesChanged();
}

void ColorSpaceWidget::blueYChanged (int value)
{
	blueYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE ) );
}

void ColorSpaceWidget::blueYChanged (double value)
{
	m_TORGB_yblue = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE) * m_TORGB_yblue);

	updateWidgetValue(ui->slider_blueY, sliderval);
	updateWidgetValue(ui->spinBox_blueY, m_TORGB_yblue);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);

	emit valuesChanged();
}

void ColorSpaceWidget::blueXChanged (int value)
{
	blueXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE ) );
}

void ColorSpaceWidget::blueXChanged (double value)
{
	m_TORGB_xblue = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE) * m_TORGB_xblue);

	updateWidgetValue(ui->slider_blueX, sliderval);
	updateWidgetValue(ui->spinBox_blueX, m_TORGB_xblue);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);

	emit valuesChanged();
}

void ColorSpaceWidget::greenYChanged (int value)
{
	greenYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE ) );
}

void ColorSpaceWidget::greenYChanged (double value)
{
	m_TORGB_ygreen = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE) * m_TORGB_ygreen);

	updateWidgetValue(ui->slider_greenY, sliderval);
	updateWidgetValue(ui->spinBox_greenY, m_TORGB_ygreen);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);

	emit valuesChanged();
}

void ColorSpaceWidget::greenXChanged (int value)
{
	greenXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE ) );
}

void ColorSpaceWidget::greenXChanged (double value)
{
	m_TORGB_xgreen = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE) * m_TORGB_xgreen);

	updateWidgetValue(ui->slider_greenX, sliderval);
	updateWidgetValue(ui->spinBox_greenX, m_TORGB_xgreen);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);

	emit valuesChanged();
}
