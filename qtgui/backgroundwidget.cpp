#include "ui_background.h"
#include "backgroundwidget.hxx"

#include "mainwindow.hxx"

#include "api.h"




BackgroundWidget::BackgroundWidget(QWidget *parent) : QWidget(parent), ui(new Ui::BackgroundWidget)
{
	ui->setupUi(this);
	
	connect(ui->checkBox_back, SIGNAL(stateChanged(int)), this, SLOT(BackChanged(int)));
//	connect(ui->checkBox_Alpha, SIGNAL(stateChanged(int)), this, SLOT(AlphaChanged(int)));
	
#if defined(__APPLE__)
	setFont(QFont  ("Lucida Grande", 11));
#endif

}

BackgroundWidget::~BackgroundWidget()
{
}

void BackgroundWidget::resetFromFilm(bool useDefaults)
{
	double t = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_ENABLED);
}
void BackgroundWidget::resetValues()
{
	BackgroundEnabled = true;
//	premultiplyAlpha = false;
}

void BackgroundWidget::updateWidgetValues()
{
//	ui->checkBox_Alpha->setChecked(premultiplyAlpha);
	ui->checkBox_back->setChecked(BackgroundEnabled);
}

/*void BackgroundWidget::AlphaChanged(int value)
{
	if (value == Qt::Checked) 
		premultiplyAlpha = true;
	else
	{
		BackgroundEnabled = false;
		premultiplyAlpha = false;}

	updateParam (LUX_FILM, LUX_FILM_BACKGROUNDENABLE, BackgroundEnabled);
	updateParam (LUX_FILM, LUX_FILM_PREMULTIPLYALPHA, premultiplyAlpha);	
	emit valuesChanged();
}*/

void BackgroundWidget::BackChanged(int value)
{
	if (value == Qt::Checked) 
		BackgroundEnabled = true;
		//premultiplyAlpha = true;}
	else
		BackgroundEnabled = false;

	updateParam (LUX_FILM, LUX_FILM_BACKGROUNDENABLE, BackgroundEnabled);
//	updateParam (LUX_FILM, LUX_FILM_PREMULTIPLYALPHA, premultiplyAlpha);
	
	emit valuesChanged();
}


