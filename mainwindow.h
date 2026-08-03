#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct AdvancementInfo {
    QString name;
    QString description;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_btnSettings_clicked();

private:
    Ui::MainWindow *ui;
    QMap<QString, AdvancementInfo> advancementReference;
    QString detectActiveWorldPath(const QString &instancePath);
    QString findAdvancementsFile(const QString &worldPath);
    QByteArray decompressGzip(const QString &filePath);
    void loadAdvancementReference(const QString &filePath);
    void loadAdvancements(const QString &filePath);
    void refreshFromInstance();
};
#endif // MAINWINDOW_H
