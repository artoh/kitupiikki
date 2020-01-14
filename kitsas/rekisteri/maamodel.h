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
#ifndef MAAMODEL_H
#define MAAMODEL_H

#include <QAbstractListModel>
#include <QIcon>

class MaaModel : public QAbstractListModel
{
    Q_OBJECT

protected:
    class Maa {
    public:
        Maa(const QString& koodi, const QString& nimi, const QString& alvreg);

        QString koodi() const { return koodi_;}
        QString nimi() const { return nimi_;}
        QString alvreg() const { return alvreg_;}
        QIcon icon() const { return icon_;}

    protected:
        QString nimi_;
        QString koodi_;
        QString alvreg_;
        QIcon icon_;
    };

public:
    explicit MaaModel(QObject *parent = nullptr);

    enum {
        KoodiRooli = Qt::UserRole,
        AlvRegExpRooli = Qt::UserRole + 1
    };

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    void lisaa(const QString& koodi, const QString& nimi, const QString& regexp);

    QList<Maa> maat_;
};

#endif // MAAMODEL_H
