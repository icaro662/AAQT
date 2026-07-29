#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QFileDialog>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    QSettings settings;
    settings.setValue("instancePath", ui->lineEditInstancePath->text());
    QDialog::accept();
}

void SettingsDialog::on_btnBrowse_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Minecraft Instance Folder",
        ui->lineEditInstancePath->text()
        );

    if (!dir.isEmpty()) {
        ui->lineEditInstancePath->setText(dir);
    }
}
