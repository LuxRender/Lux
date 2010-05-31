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

// colorspacepresets and colorspace whitepoints
static double colorspace_presets[8][NUM_COLORSPACE_PRESETS] = {
	{0.314275f, 0.3460f, 0.313f, 0.313f, 0.310f, 0.313f, 0.313f, 0.3330f}, //xwhite
	{0.329411f, 0.3590f, 0.329f, 0.329f, 0.316f, 0.329f, 0.329f, 0.3330f}, //ywhite
	{0.630000f, 0.7347f, 0.640f, 0.625f, 0.670f, 0.630f, 0.640f, 0.7347f}, //xred
	{0.340000f, 0.2653f, 0.340f, 0.340f, 0.330f, 0.340f, 0.330f, 0.2653f}, //yred
	{0.310000f, 0.1596f, 0.210f, 0.280f, 0.210f, 0.310f, 0.290f, 0.2738f}, //xgreen
	{0.595000f, 0.8404f, 0.710f, 0.595f, 0.710f, 0.595f, 0.600f, 0.7174f}, //ygreen
	{0.155000f, 0.0366f, 0.150f, 0.155f, 0.140f, 0.155f, 0.150f, 0.1666f}, //xblue
	{0.070000f, 0.0001f, 0.060f, 0.070f, 0.080f, 0.070f, 0.060f, 0.0089f}  //yblue
};
	
// standard whitepoints
static double whitepoint_presets[2][NUM_WHITEPOINT_PRESETS] = {
	{0.448f, 0.348f, 0.310f, 0.316f, 0.332f, 0.313f, 0.299f, 0.333f, 0.372f, 0.313f, 0.381f, 0.285f}, //xwhite
	{0.407f, 0.352f, 0.316f, 0.359f, 0.347f, 0.329f, 0.315f, 0.333f, 0.375f, 0.329f, 0.377f, 0.293f}  //ywhite
};
#define DEFAULT_EPSILON_MIN 0.000005f
static bool EqualDouble(const double a, const double b)
{
	return (fabs(a-b) < DEFAULT_EPSILON_MIN);
}

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
	connect(ui->checkbox_precisionEdit, SIGNAL(stateChanged(int)), this, SLOT(precisionChanged(int)));
	
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
	
#if defined(__APPLE__)
	ui->tab_whitepoint->setFont(QFont  ("Lucida Grande", 11));
	ui->tab_rgb->setFont(QFont  ("Lucida Grande", 11));
#endif

}

ColorSpaceWidget::~ColorSpaceWidget()
{
}

void ColorSpaceWidget::precisionChanged(int value)
{
	if (value == Qt::Checked) {
		ui->spinBox_whitePointX->setDecimals(5);ui->spinBox_whitePointX->setSingleStep(0.0001);
		ui->spinBox_whitePointY->setDecimals(5);ui->spinBox_whitePointY->setSingleStep(0.0001);
		ui->spinBox_redX->setDecimals(5);ui->spinBox_redX->setSingleStep(0.0001);
		ui->spinBox_redY->setDecimals(5);ui->spinBox_redY->setSingleStep(0.0001);
		ui->spinBox_greenX->setDecimals(5);ui->spinBox_greenX->setSingleStep(0.0001);
		ui->spinBox_greenY->setDecimals(5);ui->spinBox_greenY->setSingleStep(0.0001);
		ui->spinBox_blueX->setDecimals(5);ui->spinBox_blueX->setSingleStep(0.0001);
		ui->spinBox_blueY->setDecimals(5);ui->spinBox_blueY->setSingleStep(0.0001);
	}else {
		ui->spinBox_whitePointX->setDecimals(3);ui->spinBox_whitePointX->setSingleStep(0.010);
		ui->spinBox_whitePointY->setDecimals(3);ui->spinBox_whitePointY->setSingleStep(0.010);
		ui->spinBox_redX->setDecimals(3);ui->spinBox_redX->setSingleStep(0.010);
		ui->spinBox_redY->setDecimals(3);ui->spinBox_redY->setSingleStep(0.010);
		ui->spinBox_greenX->setDecimals(3);ui->spinBox_greenX->setSingleStep(0.010);
		ui->spinBox_greenY->setDecimals(3);ui->spinBox_greenY->setSingleStep(0.010);
		ui->spinBox_blueX->setDecimals(3);ui->spinBox_blueX->setSingleStep(0.010);
		ui->spinBox_blueY->setDecimals(3);ui->spinBox_blueY->setSingleStep(0.010);
	}


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
	m_TORGB_xwhite = 0.314275;
	m_TORGB_ywhite = 0.329411;
	m_TORGB_xred = 0.63;
	m_TORGB_yred = 0.34;
	m_TORGB_xgreen = 0.31;
	m_TORGB_ygreen = 0.595;
	m_TORGB_xblue = 0.155;
	m_TORGB_yblue = 0.07;
}

int ColorSpaceWidget::colorspaceToPreset(double value)
{
	int i = 0;
	while (i < NUM_COLORSPACE_PRESETS) {
		if (EqualDouble(m_TORGB_xwhite, colorspace_presets[0][i])&& EqualDouble(m_TORGB_ywhite, colorspace_presets[1][i])&&EqualDouble(m_TORGB_xred, colorspace_presets[2][i])&& EqualDouble(m_TORGB_yred, colorspace_presets[3][i])&& EqualDouble(m_TORGB_xgreen, colorspace_presets[4][i])&& EqualDouble(m_TORGB_ygreen, colorspace_presets[5][i])&& EqualDouble(m_TORGB_xblue, colorspace_presets[6][i])&& EqualDouble(m_TORGB_yblue, colorspace_presets[7][i]))
			break;
		i++;
	}
	if (i == NUM_COLORSPACE_PRESETS)
		i = -1;
	
	return i+1;
}

int ColorSpaceWidget::whitepointToPreset(double value)
{
	int i = 0;
	while (i < NUM_WHITEPOINT_PRESETS) {
		if (EqualDouble(m_TORGB_xwhite, whitepoint_presets[0][i]) && EqualDouble(m_TORGB_ywhite, whitepoint_presets[1][i]))
			break;
		i++;
	}
	if (i == NUM_WHITEPOINT_PRESETS)
		i = -1;
	
	return i+1;
}

void ColorSpaceWidget::resetFromFilm (bool useDefaults)
{
	m_TORGB_xwhite = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_WHITE);
	m_TORGB_ywhite = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_WHITE);
	m_TORGB_xred = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_RED);
	m_TORGB_yred = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_RED);
	m_TORGB_xgreen = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_GREEN);
	m_TORGB_ygreen = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_GREEN);
	m_TORGB_xblue = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_BLUE);
	m_TORGB_yblue = (double)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_BLUE);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);
	
	ui->comboBox_whitePointPreset->setCurrentIndex(whitepointToPreset(m_TORGB_xwhite));
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_xwhite));
}

void ColorSpaceWidget::setColorSpacePreset(int choice)
{
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(choice);
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_whitePointPreset->setCurrentIndex(0);
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
	// first choice is "User-defined"
	if (choice < 1)
		return;
	
	m_TORGB_xwhite = colorspace_presets[0][choice-1];
	m_TORGB_ywhite = colorspace_presets[1][choice-1];
	m_TORGB_xred = colorspace_presets[2][choice-1];
	m_TORGB_yred = colorspace_presets[3][choice-1];
	m_TORGB_xgreen = colorspace_presets[4][choice-1];
	m_TORGB_ygreen = colorspace_presets[5][choice-1];
	m_TORGB_xblue = colorspace_presets[6][choice-1];
	m_TORGB_yblue = colorspace_presets[7][choice-1];
	

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
	
	// first choice is "User-defined"
	if (choice < 1)
		return;

	m_TORGB_xwhite = whitepoint_presets[0][choice-1];
	m_TORGB_ywhite = whitepoint_presets[1][choice-1];

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
	
	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_xwhite));
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
	// Don't trigger yet another event
	ui->comboBox_whitePointPreset->blockSignals(true);
	ui->comboBox_whitePointPreset->setCurrentIndex(whitepointToPreset(m_TORGB_xwhite));
	ui->comboBox_whitePointPreset->blockSignals(false);

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
	
	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_ywhite));
	ui->comboBox_colorSpacePreset->blockSignals(false);

	// Don't trigger yet another event
	ui->comboBox_whitePointPreset->blockSignals(true);
	ui->comboBox_whitePointPreset->setCurrentIndex(whitepointToPreset(m_TORGB_ywhite));
	ui->comboBox_whitePointPreset->blockSignals(false);

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

	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_yred));
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
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
	
	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_xred));
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
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
	
	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_yblue));
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
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
	
	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_xblue));
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
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
	
	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_ygreen));
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
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
	
	// Don't trigger yet another event
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(colorspaceToPreset(m_TORGB_xgreen));
	ui->comboBox_colorSpacePreset->blockSignals(false);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);

	emit valuesChanged();
}
