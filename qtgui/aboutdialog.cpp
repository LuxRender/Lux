#include "aboutdialog.hxx"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::AboutDialog)
{
	ui->setupUi(this);
	
	imageview = new AboutImage(this);
	imageview->setFixedSize(500, 270);
	imageview->setFrameShape(QFrame::NoFrame);
	imageview->show();
	
	connect(imageview, SIGNAL(clicked()), this, SLOT(close()));
}

AboutDialog::~AboutDialog()
{
	delete imageview;
	delete ui;
}

AboutImage::AboutImage(QWidget *parent) : QGraphicsView(parent) {
	scene = new QGraphicsScene;
	scene->setSceneRect(0, 0, 500, 270);
	this->setScene(scene);
	this->setBackgroundBrush(QImage(":/images/splash.png"));
	this->setCacheMode(QGraphicsView::CacheBackground);
	
	authors = new QGraphicsTextItem(QString::fromUtf8("Jean-Philippe Grimaldi, David Bucciarelli, Asbjørn Heid, Tom Bech, Jean-François Romang, Doug Hammond, Jens Verwiebe, Vlad Miller, Matt Pharr, Greg Humphreys, Thomas De Bodt, David Washington, Abel Groenewolt, Liang Ma, Peter Bienkowski, Pascal Aebischer, Michael Gangolf, Anir-Ban Deb, Terrence Vergauwen, Ricardo Lipas Augusto, Campbell Barton"));
	authors->setDefaultTextColor(Qt::white);
	scene->addItem(authors);
	authors->setPos(510,240);

	scrolltimer = new QTimer();
	scrolltimer->start(10);
	connect(scrolltimer, SIGNAL(timeout()), SLOT(scrollTimeout()));
}

AboutImage::~AboutImage()
{
	scrolltimer->stop();
	delete scrolltimer;
	delete authors;
	delete scene;
}

void AboutImage::scrollTimeout()
{
	qreal xpos = authors->x();
	qreal endpos = xpos+authors->sceneBoundingRect().width();

	if (endpos < 0)
		xpos = 510.0f;
	else
		xpos = xpos - 1.0f;
	
	authors->setX(xpos);
}

void AboutImage::mousePressEvent(QMouseEvent* event) 
{
	emit clicked();
}