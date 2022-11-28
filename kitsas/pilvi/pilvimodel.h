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
#include "model/euro.h"
#include <QPixmap>
#include <QDate>

#include "pilvikayttaja.h"
#include "listanpilvi.h"
#include "avattupilvi.h"

class QTimer;
class QNetworkReply;

namespace Tilitieto {
    class TilitietoPalvelu;
}


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
    PilviModel(QObject *parent = nullptr, const QString& token = QString());

    enum {
        IdRooli = Qt::UserRole + 1,
        NimiRooli = Qt::UserRole + 2
    };


    int rowCount(const QModelIndex &parent=QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;


    static QString pilviLoginOsoite();

    void uusiPilvi(const QVariant& initials);

    void avaaPilvesta(int pilviId, bool siirrossa = false);
    void alustaPilvi(QVariant* data);

    KpKysely* kysely(const QString& polku = QString(),
                     KpKysely::Metodi metodi = KpKysely::GET) override;

    KpKysely *loginKysely(const QString& polku,
                          KpKysely::Metodi metodi = KpKysely::GET);

    void sulje() override;
    void poistaNykyinenPilvi();

    QString token() const;
    AvattuPilvi pilvi() const { return nykyPilvi_; }
    PilviKayttaja kayttaja() const { return kayttaja_; }

    qlonglong oikeudet() const override { return nykyPilvi_.oikeudet(); }

    QString service(const QString& serviceName) const;

    static void asetaPilviLoginOsoite(const QString& osoite);
    Tilitieto::TilitietoPalvelu* tilitietoPalvelu();

    bool tilausvoimassa() const;

// Yhteensopivuutta varten
    int pilviId() const { return pilvi().id();}
    int kayttajaPilvessa() const { return kayttaja().id(); }
    QString finvoiceOsoite() const { return service("finvoice");}
    QString kbcOsoite() const { return service("bci");}

public slots:
    void kirjaudu(const QString sahkoposti = QString(), const QString& salasana = QString(), bool pyydaAvain = false);
    void kirjauduUlos();
    void paivitaLista(int avaaPilvi = 0);
    void nimiMuuttui();

private:    
    void kirjautuminenValmis();
    void pilviLisatty(QVariant* paluu);
    void poistettu();
    void yritaUudelleenKirjautumista();
    void tarkistaKirjautuminen();

private:    
    void lueTiedotKirjautumisesta(const QVariant& data);
    void asetaPilviLista(const QVariantList lista);

signals:
    void kirjauduttu(PilviKayttaja kayttaja);
    void loginvirhe();    


private:
    PilviKayttaja kayttaja_;
    QVector<ListanPilvi> pilvet_;
    AvattuPilvi nykyPilvi_;

    QString kayttajaToken_;
    QDateTime tokenUusittu_;
    QTimer* timer_;

    int avaaPilvi_ = 0;


private:

    static QString pilviLoginOsoite__;

    Tilitieto::TilitietoPalvelu* tilitietoPalvelu_;
};

#endif // PILVIMODEL_H
