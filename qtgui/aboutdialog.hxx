#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QtGui/QDialog>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QFrame>
#include <QtGui/QImage>

namespace Ui
{
    class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    AboutDialog(QWidget *parent = 0);
    ~AboutDialog();

private:
	QGraphicsView *imageview;
	Ui::AboutDialog *ui;
    
private slots:

//	void exitApp ();

};

class AboutImage : public QGraphicsView
{
    Q_OBJECT

public:
    AboutImage(QWidget *parent = 0);
    ~AboutImage();

protected:
	void mousePressEvent(QMouseEvent* event);

private:
  QGraphicsScene *scene;

signals:
    void clicked();

};

#endif // ABOUTDIALOG_H
