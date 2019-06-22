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
#ifndef PILVIMODEL_H
#define PILVIMODEL_H

#include "db/yhteysmodel.h"

class PilviYhteys;

/**
 * @brief Pilvessä olevien kirjanpitojen luettelo
 *
 * Kirjautuminen Kitupiikin pilveen ja kirjanpitojen luettelo
 *
 */
class PilviModel : public YhteysModel
{
    Q_OBJECT
public:
    PilviModel(QObject *parent = nullptr);

    enum {
        IdRooli = Qt::UserRole + 1,
        NimiRooli = Qt::UserRole + 2
    };


    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QString kayttajaNimi() const { return kayttajaNimi_;}
    int kayttajaPilvessa() const { return kayttajaId_ != 0;}

    static QString pilviLoginOsoite();

    bool avaaPilvesta(int pilviId);

    KpKysely* kysely(const QString& polku = QString(),
                     KpKysely::Metodi metodi = KpKysely::GET) override;

    void sulje() override;


    int pilviId() const { return pilviId_;}
    QString pilviosoite() const { return osoite_;}
    QString token() const { return token_; };

public slots:
    void kirjaudu(const QString sahkoposti = QString(), const QString& salasana = QString(), bool pyydaAvain = false);
    void kirjauduUlos();



private slots:
    void kirjautuminenValmis();

signals:
    void kirjauduttu();
    void loginvirhe();



private:
    int kayttajaId_ = 0;
    QString kayttajaNimi_;
    QString kayttajaToken_;
    int pilviId_ = 0;
    QString osoite_;
    QString token_;

    QList<QVariantMap> pilvet_;
};

#endif // PILVIMODEL_H
