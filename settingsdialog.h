#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

public slots:
    void accept() override;

private slots:
    void on_btnBrowse_clicked();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
