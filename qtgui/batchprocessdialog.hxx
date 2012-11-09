#ifndef BATCHPROCESSDIALOG_H
#define BATCHPROCESSDIALOG_H

#include <QDialog>
#include <QString>
#include <QWidget>

namespace Ui {
    class BatchProcessDialog;
}

class BatchProcessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchProcessDialog(const QString &startDir, QWidget *parent = 0);
    ~BatchProcessDialog();

    // Retrieve dialog specifics
    bool individualLightGroups();
    QString inputDir();
    QString outputDir();
    bool applyTonemapping();
    int format();

private slots:
    void on_browseForInputDirectoryButton_clicked();
    void on_browseForOutputDirectoryButton_clicked();

private:
    Ui::BatchProcessDialog *ui;
	QString m_startDir;
};

#endif // BATCHPROCESSDIALOG_H
