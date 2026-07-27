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

private:
    Ui::MainWindow *ui;
    QMap<QString, AdvancementInfo> advancementReference;
    void loadAdvancementReference(const QString &filePath);
    void loadAdvancements(const QString &filePath);

};
#endif // MAINWINDOW_H
