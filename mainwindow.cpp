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
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QLayoutItem>
#include <QGroupBox>
#include <QGridLayout>
#include <QListView>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractItemView>

#include <zlib.h>
#include <iostream>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    multiCriteriaOrder = {
        "minecraft:adventure/adventuring_time",
        "minecraft:adventure/kill_all_mobs",
        "minecraft:husbandry/balanced_diet",
        "minecraft:husbandry/bred_all_animals",
        "minecraft:husbandry/complete_catalogue",
        "minecraft:nether/explore_nether"
    };

    ui->setupUi(this);
    ui->listAdvancements->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listAdvancementsMulti->setSelectionMode(QAbstractItemView::NoSelection);
    QLayout *oldLayout = centralWidget()->layout();
    if (oldLayout) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item;
        }
        delete oldLayout;
    }
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget());
    mainLayout->addWidget(ui->listAdvancements, 2);
    mainLayout->addWidget(ui->listAdvancementsMulti, 2);
    mainLayout->addWidget(ui->btnSettings, 0);
    ui->listAdvancements->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->listAdvancementsMulti->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->listAdvancementsMulti->setViewMode(QListView::IconMode);
    ui->listAdvancementsMulti->setFlow(QListView::LeftToRight);
    ui->listAdvancementsMulti->setResizeMode(QListView::Adjust);
    ui->listAdvancementsMulti->setWrapping(true);
    ui->listAdvancements->setGridSize(QSize(90, 90));
    ui->listAdvancements->setUniformItemSizes(true);
    setWindowTitle("AAQT");
    showMaximized();

    loadAdvancementReference("/home/hikaru/Projects/AAQT/AA-Tool/advancements_reference.json");
    refreshFromInstance();

    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshFromInstance);
    refreshTimer->start(60000);
}


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
    QDir selectedDir(instancePath);
    if (!selectedDir.exists()) {
        qDebug() << "Selected path doesn't exist:" << instancePath;
        return QString();
    }

    // Case 1: the selected folder IS a world folder itself
    if (QFileInfo(selectedDir.filePath("level.dat")).exists()) {
        qDebug() << "Path is a world folder directly:" << instancePath;
        return instancePath;
    }

    // Case 2: the selected folder directly contains world folders (i.e. it IS "saves")
    QFileInfoList directChildren = selectedDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &child : directChildren) {
        if (QFileInfo(child.absoluteFilePath() + "/level.dat").exists()) {
            qDebug() << "Path is a saves folder directly:" << instancePath;
            return findMostRecentWorld(instancePath);
        }
    }

    // Case 3: try common nested locations for a saves folder
    QStringList candidates = {
        instancePath + "/saves",
        instancePath + "/minecraft/saves",
        instancePath + "/.minecraft/saves"
    };

    for (const QString &candidate : candidates) {
        if (QDir(candidate).exists()) {
            QString result = findMostRecentWorld(candidate);
            if (!result.isEmpty()) {
                qDebug() << "Found saves folder at:" << candidate;
                return result;
            }
        }
    }

    qDebug() << "Could not resolve a world path from:" << instancePath;
    return QString();
}


QString MainWindow::findMostRecentWorld(const QString &savesPath)
{
    QDir savesDir(savesPath);
    QFileInfoList worldFolders = savesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    QString mostRecentWorld;
    QDateTime mostRecentTime;

    for (const QFileInfo &folder : worldFolders) {
        QFileInfo levelDat(folder.absoluteFilePath() + "/level.dat");
        if (!levelDat.exists()) continue;

        QDateTime modified = levelDat.lastModified();
        if (mostRecentWorld.isEmpty() || modified > mostRecentTime) {
            mostRecentWorld = folder.absoluteFilePath();
            mostRecentTime = modified;
        }
    }

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
    QJsonObject root;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            root = doc.object();
        }
    }

    for (const QString &key : advancementReference.keys()) {
        if (key.endsWith("/root")) continue;
        if (multiCriteriaOrder.contains(key)) continue;

        AdvancementInfo info = advancementReference.value(key);
        QString displayText = info.name;

        QJsonObject advancement = root.value(key).toObject();
        bool done = advancement.value("done").toBool();

        QPixmap pixmap(64, 64);
        pixmap.fill(done ? QColor("#3a7d3a") : QColor("#555555"));
        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.drawRect(0, 0, 63, 63);
        painter.end();

        QListWidgetItem *item = new QListWidgetItem(QIcon(pixmap), displayText);
        ui->listAdvancements->addItem(item);
    }

    for (const QString &key : multiCriteriaOrder) {
        if (!advancementReference.contains(key)) continue;

        AdvancementInfo info = advancementReference.value(key);
        QString displayText = info.name;
        QStringList criteriaList = info.criteria;

        QJsonObject advancement = root.value(key).toObject();
        QJsonObject completedCriteria = advancement.value("criteria").toObject();

        QGroupBox *card = new QGroupBox(displayText);
        QGridLayout *cardLayout = new QGridLayout(card);
        cardLayout->setSpacing(2);
        cardLayout->setContentsMargins(4, 4, 4, 4);

        const int maxRows = 16;
        int rowIndex = 0;
        int colIndex = 0;

        QFont smallFont;
        smallFont.setPointSize(8);

        for (const QString &criterion : criteriaList) {
            bool criterionDone = completedCriteria.contains(criterion);

            QPixmap smallPixmap(12, 12);
            smallPixmap.fill(criterionDone ? QColor("#3a7d3a") : QColor("#555555"));
            QPainter smallPainter(&smallPixmap);
            smallPainter.setPen(Qt::white);
            smallPainter.drawRect(0, 0, 11, 11);
            smallPainter.end();

            QWidget *row = new QWidget();
            QHBoxLayout *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(3);

            QLabel *iconLabel = new QLabel();
            iconLabel->setPixmap(smallPixmap);

            QLabel *textLabel = new QLabel(formatCriterionName(criterion));
            textLabel->setFont(smallFont);

            rowLayout->addWidget(iconLabel);
            rowLayout->addWidget(textLabel);
            rowLayout->addStretch();

            cardLayout->addWidget(row, rowIndex, colIndex);

            rowIndex++;
            if (rowIndex >= maxRows) {
                rowIndex = 0;
                colIndex++;
            }
        }

        QListWidgetItem *listItem = new QListWidgetItem();
        listItem->setSizeHint(card->sizeHint());
        ui->listAdvancementsMulti->addItem(listItem);
        ui->listAdvancementsMulti->setItemWidget(listItem, card);
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
