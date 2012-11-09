#include <QFileDialog>

#include "batchprocessdialog.hxx"
#include "ui_batchprocessdialog.h"

BatchProcessDialog::BatchProcessDialog(const QString &startingDir, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchProcessDialog)
{
    ui->setupUi(this);
	m_startDir = startingDir;
}

BatchProcessDialog::~BatchProcessDialog()
{
    delete ui;
}

bool BatchProcessDialog::individualLightGroups() { return ui->allLightGroupsModeRadioButton->isChecked(); }
QString BatchProcessDialog::inputDir() { return ui->inputDirectoryLineEdit->text(); }
QString BatchProcessDialog::outputDir() { return ui->outputDirectoryLineEdit->text(); }
bool BatchProcessDialog::applyTonemapping() { return ui->tonemapCheckBox->isChecked(); }
int BatchProcessDialog::format() { return ui->imageFormatComboBox->currentIndex(); }

void BatchProcessDialog::on_browseForInputDirectoryButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Input Directory"), m_startDir,
                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(dir.isNull() || dir.isEmpty()) return;

    ui->inputDirectoryLineEdit->setText(dir);
}

void BatchProcessDialog::on_browseForOutputDirectoryButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), m_startDir,
                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(dir.isNull() || dir.isEmpty()) return;

    ui->outputDirectoryLineEdit->setText(dir);
}
