#include "ui_openexroptionsdialog.h"
#include "openexroptionsdialog.hxx"

OpenEXROptionsDialog::OpenEXROptionsDialog(QWidget *parent, const bool halfFloats, const bool depthBuffer,
                                           const int compressionType) :
    QDialog(parent),
    ui(new Ui::OpenEXROptionsDialog)
{
    ui->setupUi(this);

    if(!halfFloats) { ui->halfFloatRadioButton->setChecked(false); ui->singleFloatRadioButton->setChecked(true); }
    if(depthBuffer) { ui->depthChannelCheckBox->setChecked(true); }
    if(compressionType != 1) ui->compressionTypeComboBox->setCurrentIndex(compressionType);
}

OpenEXROptionsDialog::~OpenEXROptionsDialog()
{
    delete ui;
}

void OpenEXROptionsDialog::disableZBufferCheckbox()
{
    // Uncheck Zbuf and disable
    ui->depthChannelCheckBox->setChecked(false);
    ui->depthChannelCheckBox->setEnabled(false);
}

bool OpenEXROptionsDialog::useHalfFloats() const { return ui->halfFloatRadioButton->isChecked(); }
bool OpenEXROptionsDialog::includeZBuffer() const { return ui->depthChannelCheckBox->isChecked(); }
int OpenEXROptionsDialog::getCompressionType() const { return ui->compressionTypeComboBox->currentIndex(); }
