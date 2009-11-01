#include "aboutdialog.h"
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
}

AboutImage::~AboutImage()
{
	delete scene;
}

void AboutImage::mousePressEvent(QMouseEvent* event) 
{
	emit clicked();
}