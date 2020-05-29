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
#ifndef KAYTTOOIKEUSMODEL_H
#define KAYTTOOIKEUSMODEL_H

#include <QAbstractListModel>
#include <QList>

class KayttooikeusModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit KayttooikeusModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    enum {
        NimiRooli = Qt::UserRole,
        EmailRooli = Qt::UserRole + 1,
        OikeusRooli = Qt::UserRole + 2
    };

    void paivita();
    QModelIndex lisaa(const QString& email, const QString nimi);

private:
    void listaSaapuu(QVariant* data);

private:
    class Kayttaja {
    public:
        Kayttaja(const QVariant& data);
        Kayttaja(const QString& email, const QString& nimi);
        QString nimi() const;
        QString email() const;
        QStringList oikeudet() const;

    private:
        QString nimi_;
        QString email_;
        QStringList oikeudet_;
    };


    QList<Kayttaja> kayttajat_;

};

#endif // KAYTTOOIKEUSMODEL_H
