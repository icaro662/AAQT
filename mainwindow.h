#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct AdvancementInfo {
    QString name;
    QString description;
    QStringList criteria;   // empty = single-criterion advancement
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
    QString parseMinecraftVersion(const QByteArray &levelDatBytes);
    QString detectActiveWorldPath(const QString &instancePath);
    QString findAdvancementsFile(const QString &worldPath);
    QString formatCriterionName(const QString &rawKey);
    QByteArray decompressGzip(const QString &filePath);
    QTimer *refreshTimer;
    void loadAdvancementReference(const QString &filePath);
    void loadAdvancements(const QString &filePath);
    void refreshFromInstance();
};
#endif // MAINWINDOW_H
