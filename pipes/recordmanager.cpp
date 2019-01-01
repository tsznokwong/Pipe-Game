#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>
#include <QMessageBox>

#include "recordmanager.h"
#include "loginwindow.h"

const QString RecordManager::record_dir =
    QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/comp2012h_pipes";
const QString RecordManager::record_path =
    QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/comp2012h_pipes/record.txt";
const QString RecordManager::record_path_feature =
    QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/comp2012h_pipes/record_feature.txt";

RecordManager::RecordManager()
{
    // Create directory if not exist
    if (!QDir(this->record_dir).exists()) {
        QDir().mkdir(this->record_dir);
    }

    QFile record{this->record_path};
    if (!record.exists()) {
        // File does not exist, write data
        for (int level = 1; level <= this->NUM_OF_LEVELS; ++level) {
            this->update_record(level, -1);
        }
    } else {
        // File exist, read data
        record.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream{&record};
        for (int level = 0; level < this->NUM_OF_LEVELS; ++level) {
            stream >> this->records[level];
        }
    }
    record.close();
    // Record for feature
    QFile featureRecord{this->record_path_feature};
    if (!featureRecord.exists()) {
        // File does not exist, write data
        this->update_record(featureLevel, -1);
    } else {
        // File exist, read data
        featureRecord.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream{&featureRecord};
        stream >> this->featureRecord;
    }
    featureRecord.close();
}

int RecordManager::get_record(int level)
{
    if (level == featureLevel) return this->featureRecord;
    if (level < 1 || level > this->NUM_OF_LEVELS || level > this->MAX_LEVEL) return -1;
    return this->records[level - 1];
}

int RecordManager::get_num_of_levels()
{
    return this->NUM_OF_LEVELS;
}

void RecordManager::update_record(int level, int value)
{
    if (level < 1 || ((level > this->NUM_OF_LEVELS || level > this->MAX_LEVEL) && level != featureLevel)) return;

    QString path = level == featureLevel ? this->record_path_feature : this->record_path;
    QFile record{path};
    record.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream{&record};

    if (level == featureLevel) {
        this->featureRecord = value;
        stream << this->featureRecord << endl;
    } else {
        this->records[level - 1] = value;
        for (int level = 0; level < this->NUM_OF_LEVELS; ++level) {
            stream << this->records[level] << endl;
        }
    }

    record.close();
}

