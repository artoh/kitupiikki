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

#include <QPixmap>
#include <QIcon>

#include <QClipboard>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

KitsasLokiModel::KitsasLokiModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

QIcon KitsasLokiModel::colorFilledIcon(const QColor &color)
{
    QPixmap pixmap(QSize(32,32));
    pixmap.fill(color);
    return QIcon(pixmap);
}

QColor KitsasLokiModel::levelColor(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg: return Qt::black;
        case QtInfoMsg: return Qt::blue;
        case QtWarningMsg: return Qt::yellow;
        case QtCriticalMsg: return Qt::red;
        case QtFatalMsg: return Qt::darkRed;
        default: return Qt::gray;
    }
}


void KitsasLokiModel::alusta()
{
    instanssi__ = new KitsasLokiModel(nullptr);
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
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case FILE: return tr("Tiedosto");
            case LINE: return tr("Rivi");
            case MESSAGE: return tr("Viesti");
        }
    }
    return QVariant();
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

    return 3;
}

QVariant KitsasLokiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = loki_.count() - index.row() - 1;

    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case FILE: {
            QString fname = loki_.value(row).fileName();
            QString nameonly = fname.mid( fname.lastIndexOf('/') + 1 );
            return nameonly;
        }
        case LINE: {
            int line = loki_.value(row).line();
            return line ? line : QVariant();
        }
        case MESSAGE:
            return loki_.value(row).message();
        }
    }
    else if( role == Qt::DecorationRole && index.column() == MESSAGE) {
        return colorFilledIcon( levelColor( loki_.value(row).type() ) );
    }
    return QVariant();
}

void KitsasLokiModel::append(const KitsasLokiModel::LokiRivi &rivi)
{    
    if( loki_.count() > MAXLINES * 5 / 4) {
        beginResetModel();
        QList<LokiRivi>::iterator iter(loki_.begin());
        iter += MAXLINES  / 4;
        loki_.erase(loki_.begin(), iter);
        endResetModel();
    }

    beginInsertRows( QModelIndex(), 0, 0);
    loki_.append(rivi);
    endInsertRows();
}

void KitsasLokiModel::copyAll()
{
    QString txt = QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm") + "\n";

    txt.append(QString("%1 %2 @%8\n%3 \n%4 %5 %6 %7 \n\n"
                           ).arg(qApp->applicationVersion())
                            .arg(QSysInfo::prettyProductName())
                            .arg(kp()->kirjanpitoPolku())
                            .arg(kp()->asetukset()->asetus(AsetusModel::Tilikartta))
                            .arg(kp()->asetukset()->asetus(AsetusModel::TilikarttaPvm))
                            .arg(kp()->asetukset()->asetus(AsetusModel::Muoto))
                            .arg(kp()->asetukset()->asetus(AsetusModel::Laajuus))
                            .arg(kp()->pilvi()->kayttajaPilvessa())
               );

    for(const LokiRivi& rivi : loki_) {

        QString fname = rivi.fileName();
        QString nameonly = fname.mid( fname.lastIndexOf('/') + 1 );

        txt.append(fname.isEmpty() ? QString("%1 %2\n").arg(levelText(rivi.type())).arg(rivi.message())  :
            QString("%1 [%2:%3] %4\n")
                .arg(levelText(rivi.type()))
                .arg(nameonly)
                .arg(rivi.line())
                .arg(rivi.message()) );
    }

    qApp->clipboard()->setText(txt);

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
KitsasLokiModel::LokiRivi::LokiRivi()
{

}

KitsasLokiModel::LokiRivi::LokiRivi(QtMsgType type, const QMessageLogContext &context, const QString &message) :
    type_{type}, file_{context.file}, funktio_{context.function}, line_{context.line}, message_{message}
{

}
