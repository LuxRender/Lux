#ifndef OPENEXROPTIONSDIALOG_H
#define OPENEXROPTIONSDIALOG_H

#include <QDialog>
#include <QWidget>

namespace Ui {
    class OpenEXROptionsDialog;
}

class OpenEXROptionsDialog : public QDialog
{
    Q_OBJECT

public:
    OpenEXROptionsDialog(QWidget *parent = 0, const bool halfFloats = true, const bool depthBuffer = false,
                         const int compressionType = 1);
    ~OpenEXROptionsDialog();

    void disableZBufferCheckbox();

    bool useHalfFloats() const;
    bool includeZBuffer() const;
    int getCompressionType() const;

private:
    Ui::OpenEXROptionsDialog *ui;
};

#endif // OPENEXROPTIONSDIALOG_H
