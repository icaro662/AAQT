#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "nbtreader.h"

#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QMainWindow>
#include <QDebug>
#include <QPainter>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractItemView>

#include <zlib.h>
#include <iostream>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->listAdvancements->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listAdvancementsMulti->setSelectionMode(QAbstractItemView::NoSelection);
    setWindowTitle("AAQT");
    showMaximized();

    loadAdvancementReference("/home/hikaru/Projects/AA-Tool/AA-Tool/advancements_reference.json");
QByteArray MainWindow::decompressGzip(const QString &filePath)
{
    gzFile file = gzopen(filePath.toUtf8().constData(), "rb");
    if (!file) {
        qDebug() << "Could not open gzip file:" << filePath;
        return QByteArray();
    }

    QByteArray result;
    char buffer[8192];
    int bytesRead;

    while ((bytesRead = gzread(file, buffer, sizeof(buffer))) > 0) {
        result.append(buffer, bytesRead);
    }

    gzclose(file);
    return result;
}

QString MainWindow::parseMinecraftVersion(const QByteArray &levelDatBytes)
{
    NbtReader reader(levelDatBytes);

    quint8 rootType = reader.readByte();
    if (rootType != 10) {
        qDebug() << "Not a valid NBT file, root type:" << rootType;
        return QString();
    }
    reader.readString(); // root name, discarded

    // Walk the root compound's children looking for "Data"
    while (true) {
        quint8 childType = reader.readByte();
        if (childType == 0) break; // End of root compound, "Data" not found

        QString childName = reader.readString();

        if (childType == 10 && childName == "Data") {
            // Found it — walk INTO the Data compound looking for "Version"
            while (true) {
                quint8 dataChildType = reader.readByte();
                if (dataChildType == 0) break;

                QString dataChildName = reader.readString();

                if (dataChildType == 10 && dataChildName == "Version") {
                    // Found it — walk INTO Version looking for "Name"
                    while (true) {
                        quint8 versionChildType = reader.readByte();
                        if (versionChildType == 0) break;

                        QString versionChildName = reader.readString();

                        if (versionChildType == 8 && versionChildName == "Name") {
                            return reader.readString(); // this IS the version string
                        } else {
                            reader.skipPayload(versionChildType);
                        }
                    }
                    return QString(); // Version found, but no Name inside
                } else {
                    reader.skipPayload(dataChildType);
                }
            }
            return QString(); // Data found, but no Version inside
        } else {
            reader.skipPayload(childType);
        }
    }

    return QString(); // "Data" not found at all
}

void MainWindow::refreshFromInstance()
{
    QSettings settings;
    QString instancePath = settings.value("instancePath", "").toString();

    if (!instancePath.isEmpty()) {
        QString worldPath = detectActiveWorldPath(instancePath);
        if (!worldPath.isEmpty()) {
            QByteArray levelDatBytes = decompressGzip(worldPath + "/level.dat");
            QString version = parseMinecraftVersion(levelDatBytes);

            setWindowTitle("AAQT - " + version);

            QString advFile = findAdvancementsFile(worldPath);
            if (!advFile.isEmpty()) {
                loadAdvancements(advFile);
            }
        }
    }
}

QString MainWindow::detectActiveWorldPath(const QString &instancePath)
{
    QDir savesDir(instancePath + "/minecraft/saves");
    if (!savesDir.exists()) {
        qDebug() << "Saves folder doesn't exist:" << savesDir.path();
        return QString();
    }

    QFileInfoList worldFolders = savesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    QString mostRecentWorld;
    QDateTime mostRecentTime;

    for (const QFileInfo &folder : worldFolders) {
        QFileInfo levelDat(folder.absoluteFilePath() + "/level.dat");

        if (!levelDat.exists()) {
            qDebug() << "No level.dat in:" << folder.absoluteFilePath();
            continue;
        }

        QDateTime modified = levelDat.lastModified();
        qDebug() << folder.fileName() << "last modified:" << modified;

        if (mostRecentWorld.isEmpty() || modified > mostRecentTime) {
            mostRecentWorld = folder.absoluteFilePath();
            mostRecentTime = modified;
        }
    }

    qDebug() << "Selected world:" << mostRecentWorld;
    return mostRecentWorld;
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

        if (entry.contains("criteria")) {
            QJsonArray criteriaArray = entry.value("criteria").toArray();
            for (const QJsonValue &val : criteriaArray) {
                info.criteria.append(val.toString());
            }
        }

        advancementReference.insert(key, info);
    }
}

QString MainWindow::findAdvancementsFile(const QString &worldPath)
{
    QDir advancementsDir(worldPath + "/advancements");
    if (!advancementsDir.exists()) {
        return QString();
    }

    QStringList jsonFiles = advancementsDir.entryList(QStringList() << "*.json", QDir::Files);

    if (jsonFiles.isEmpty()) {
        return QString();
    }

    return advancementsDir.absoluteFilePath(jsonFiles.first());
}

QString MainWindow::formatCriterionName(const QString &rawKey)
{
    QString name = rawKey;

    if (name.startsWith("minecraft:")) {
        name.remove(0, 10); // strip the "minecraft:" prefix (10 characters)
    }

    name.replace("_", " ");

    QStringList words = name.split(" ");
    for (int i = 0; i < words.size(); i++) {
        if (!words[i].isEmpty()) {
            words[i][0] = words[i][0].toUpper();
        }
    }

    return words.join(" ");
}

void MainWindow::loadAdvancements(const QString &filePath)
{
    ui->listAdvancements->clear();
    ui->listAdvancementsMulti->clear();

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
        if (key.endsWith("/root")) continue;

        QJsonObject advancement = root.value(key).toObject();
        bool done = advancement.value("done").toBool();
        QJsonObject completedCriteria = advancement.value("criteria").toObject();

        QString displayText;
        bool isMultiCriteria = false;
        QStringList criteriaList;

        if (advancementReference.contains(key)) {
            AdvancementInfo info = advancementReference.value(key);
            displayText = info.name;
            criteriaList = info.criteria;
            isMultiCriteria = criteriaList.size() > 1;
        } else {
            displayText = key;
        }

        QPixmap pixmap(64, 64);
        pixmap.fill(done ? QColor("#3a7d3a") : QColor("#555555"));
        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.drawRect(0, 0, 63, 63);
        painter.end();
        QIcon icon(pixmap);

        if (isMultiCriteria) {
            QGroupBox *card = new QGroupBox(displayText);
            QVBoxLayout *cardLayout = new QVBoxLayout(card);

            for (const QString &criterion : criteriaList) {
                bool criterionDone = completedCriteria.contains(criterion);

                QPixmap smallPixmap(20, 20);
                smallPixmap.fill(criterionDone ? QColor("#3a7d3a") : QColor("#555555"));
                QPainter smallPainter(&smallPixmap);
                smallPainter.setPen(Qt::white);
                smallPainter.drawRect(0, 0, 19, 19);
                smallPainter.end();

                QWidget *row = new QWidget();
                QHBoxLayout *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);

                QLabel *iconLabel = new QLabel();
                iconLabel->setPixmap(smallPixmap);

                QLabel *textLabel = new QLabel(formatCriterionName(criterion));

                rowLayout->addWidget(iconLabel);
                rowLayout->addWidget(textLabel);
                rowLayout->addStretch();

                cardLayout->addWidget(row);
            }

            QListWidgetItem *listItem = new QListWidgetItem();
            listItem->setSizeHint(card->sizeHint());
            ui->listAdvancementsMulti->addItem(listItem);
            ui->listAdvancementsMulti->setItemWidget(listItem, card);
        } else {
            QListWidgetItem *item = new QListWidgetItem(icon, displayText);
            ui->listAdvancements->addItem(item);
        }
    }
}

void MainWindow::on_btnSettings_clicked()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshFromInstance();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
