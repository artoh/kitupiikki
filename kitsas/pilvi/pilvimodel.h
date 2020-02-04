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
#include <QPixmap>

class QTimer;
class QNetworkReply;

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


    int rowCount(const QModelIndex &parent=QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QString kayttajaNimi() const { return data_.value("name").toString();}
    QString kayttajaEmail() const { return data_.value("email").toString();}
    int kayttajaPilvessa() const { return kayttajaId_; }
    int plan() const { return data_.value("plan").toMap().value("id").toInt();}
    QString planname() const { return data_.value("plan").toMap().value("name").toString();}
    int omatPilvet() const;
    int pilviMax() const { return data_.value("cloudsmax").toInt();}

    static QString pilviLoginOsoite();

    void uusiPilvi(const QVariant& initials);

    bool avaaPilvesta(int pilviId);

    KpKysely* kysely(const QString& polku = QString(),
                     KpKysely::Metodi metodi = KpKysely::GET) override;

    void sulje() override;


    int pilviId() const { return pilviId_;}
    QString pilviosoite() const { return osoite_;}
    QString token() const { return token_; }

    qlonglong oikeudet() const override { return oikeudet_;}

    /**
     * @brief Muodostaa oikeuksista bittikartan
     * @param Oikeudet listana ["Ts","Tl"] jne
     * @return YhteysModelin oikeuksista koostuva bittikartta
     */
    static qlonglong oikeudet(const QVariantList& lista);

public slots:
    void kirjaudu(const QString sahkoposti = QString(), const QString& salasana = QString(), bool pyydaAvain = false);
    void kirjauduUlos();
    void paivitaLista();


private:
    void kirjautuminenValmis();
    void paivitysValmis(QVariant* paluu);
    void pilviLisatty(QVariant* paluu);
    void tilaaLogo(const QVariantMap& map);


signals:
    void kirjauduttu();
    void loginvirhe();

private:
    int kayttajaId_ = 0;
    int pilviId_ = 0;
    int avaaPilvi_ = 0;
    QString osoite_;
    QString token_;
    qlonglong oikeudet_ = 0;

    QVariantMap data_;
    QTimer *timer_;
    QMap<int,QPixmap> logot_;

private:
    static std::map<QString,qlonglong> oikeustunnukset__;
};

#endif // PILVIMODEL_H
