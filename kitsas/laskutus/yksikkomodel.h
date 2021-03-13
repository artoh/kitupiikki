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
#ifndef YKSIKKOMODEL_H
#define YKSIKKOMODEL_H

#include <QAbstractListModel>
#include <QList>

class YksikkoModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit YksikkoModel(QObject *parent = nullptr);

    enum {
        NimiRooli = Qt::DisplayRole,
        UNKoodiRooli = Qt::UserRole
    };

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;    

protected:
    void lisaa(const QString& UNkoodi, const QString& nimi);

private:
    class Yksikko {
    public:
        Yksikko();
        Yksikko(const QString& UNkoodi, const QString& nimi);
        QString unKoodi() const { return UNkoodi_; }
        QString nimi() const { return nimi_;}

    private:
        QString UNkoodi_;
        QString nimi_;
    };

    QList<Yksikko> yksikot_;
};

#endif // YKSIKKOMODEL_H
