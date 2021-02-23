/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "kitsaslokimodel.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

KitsasLokiModel::KitsasLokiModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

void KitsasLokiModel::alusta()
{
    instanssi__ = new KitsasLokiModel(qApp);
    qInstallMessageHandler(KitsasLokiModel::messageHandler);
}

void KitsasLokiModel::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    instanssi__->append( LokiRivi(type, context, message) );

    if( !instanssi__->filename_.isEmpty()) {
        QString fname = context.file;
        QString nameonly = fname.mid( fname.lastIndexOf('/') + 1 );

        QString text = fname.isEmpty() ? QString("%1 %2").arg(levelText(type)).arg(message)  :
            QString("%1 [%2:%3] %4")
                .arg(levelText(type))
                .arg(nameonly)
                .arg(context.line)
                .arg(message);

        QFile file(instanssi__->filename_);
        file.open( QIODevice::WriteOnly | QIODevice::Append);
        QTextStream stream(&file);
        stream << text << endl;
    }
}

void KitsasLokiModel::setLoggingToFile(const QString &filename)
{
    instanssi__->filename_ = filename;

    if(!filename.isEmpty()) {
        QString viivat;
        viivat.fill('-', 80);
        QFile file(instanssi__->filename_);
        file.open( QIODevice::WriteOnly | QIODevice::Append);
        QTextStream stream(&file);
        stream << endl << QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm") << " " << viivat << endl;
    }
}

QVariant KitsasLokiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
}

int KitsasLokiModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return loki_.count();
}

int KitsasLokiModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
}

QVariant KitsasLokiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    return QVariant();
}

void KitsasLokiModel::append(const KitsasLokiModel::LokiRivi &rivi)
{
    beginInsertRows( QModelIndex(), loki_.count(), loki_.count());
    loki_.append(rivi);
    endInsertRows();
}

QString KitsasLokiModel::levelText(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg: return "debug";
        case QtInfoMsg: return "info";
        case QtWarningMsg: return "warning";
        case QtCriticalMsg: return "critical";
        case QtFatalMsg: return "fatal";
        default: return QString();
    }
}

KitsasLokiModel* KitsasLokiModel::instanssi__ = nullptr;
KitsasLokiModel::LokiRivi::LokiRivi(QtMsgType type, const QMessageLogContext &context, const QString &message) :
    type_{type}, file_{context.file}, funktio_{context.function}, line_{context.line}, message_{message}
{

}
