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
#ifndef KITSASLOKIMODEL_H
#define KITSASLOKIMODEL_H

#include <QAbstractTableModel>

class KitsasLokiModel : public QAbstractTableModel
{
    Q_OBJECT

    class LokiRivi {
    public:
        LokiRivi();
        LokiRivi(QtMsgType type, const QMessageLogContext& context, const QString& message);
        QtMsgType type() const { return type_;}
        QString fileName() const { return file_;}
        QString functionName() const { return funktio_;}
        int line() const { return line_;}
        QString message() const { return message_;}
    private:
        QtMsgType type_;
        QString file_;
        QString funktio_;
        int line_;
        QString message_;
    };


public:

    enum {FILE, LINE, MESSAGE};
    enum {MAXLINES = 1024};
    enum {TypeRole = Qt::UserRole  };

    static void alusta();
    static KitsasLokiModel* instanssi() { return instanssi__;};
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);

    static void setLoggingToFile(const QString& filename);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void append(const LokiRivi& rivi);    

    void copyAll();
    QVariantList asList() const;

private:
    static QString levelText(QtMsgType type);

    KitsasLokiModel(QObject *parent = nullptr);
    static KitsasLokiModel* instanssi__;

    static QIcon colorFilledIcon(const QColor& color);
    static QColor levelColor(QtMsgType type);    

    QString filename_;
    QList<LokiRivi> loki_;

};

#endif // KITSASLOKIMODEL_H
