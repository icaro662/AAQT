#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadAdvancementReference("/home/hikaru/Projects/AA-Tool/AA-Tool/advancements_reference.json");
    loadAdvancements("/home/hikaru/.var/app/org.prismlauncher.PrismLauncher/data/PrismLauncher/instances/1.16.1/minecraft/saves/New World/advancements/fa707870-b3b3-4e17-a4d8-89dfd5bfa073.json");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadAdvancements(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->listAdvancements->addItem("Could not open file: " + filePath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        ui->listAdvancements->addItem("JSON parse error: " + parseError.errorString());
        return;
    }

    QJsonObject root = doc.object();

    for (const QString &key : root.keys()) {
        if (key == "DataVersion") continue;
         if (key.startsWith("minecraft:recipes/")) continue;

        QJsonObject advancement = root.value(key).toObject();
        bool done = advancement.value("done").toBool();

        QString displayText;
        if (advancementReference.contains(key)) {
            displayText = advancementReference.value(key).name;
        } else {
            displayText = key; // fallback: no reference entry yet, show raw key
        }

        QListWidgetItem *item = new QListWidgetItem(displayText);
        if (done) {
            item->setForeground(QColor("green"));
        }
        ui->listAdvancements->addItem(item);
    }
}

void MainWindow::loadAdvancementReference(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    for (const QString &key : root.keys()) {
        QJsonObject entry = root.value(key).toObject();

        AdvancementInfo info;
        info.name = entry.value("name").toString();
        info.description = entry.value("description").toString();

        advancementReference.insert(key, info);
    }
}


