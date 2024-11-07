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
#include <QDate>

#include "pilvikayttaja.h"
#include "listanpilvi.h"
#include "avattupilvi.h"

class QTimer;
class QNetworkReply;
class PaivitysInfo;
class QProgressDialog;

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
    void alustaPilvi(QVariant* data, bool siirrossa = false);
    void uusiPilviAlustettu();
    void virheUudenPilvenAlustamisessa();

    KpKysely* kysely(const QString& polku = QString(),
                     KpKysely::Metodi metodi = KpKysely::GET) override;

    KpKysely *loginKysely(const QString& polku,
                          KpKysely::Metodi metodi = KpKysely::GET);

    void sulje() override;
    void poistaNykyinenPilvi();

    QString token() const;
    AvattuPilvi pilvi() const { return nykyPilvi_; }
    PilviKayttaja kayttaja() const { return kayttaja_; }
    PaivitysInfo *paivitysInfo() { return paivitysInfo_;}

    qlonglong oikeudet() const override { return nykyPilvi_.oikeudet(); }

    QString service(const QString& serviceName) const;

    static void asetaPilviLoginOsoite(const QString& osoite);    

    bool tilausvoimassa() const;
    void asetaAlias(const QString& alias);
    void poistaNotify(const QString &id);

// Yhteensopivuutta varten
    int pilviId() const { return pilvi().id();}
    int kayttajaPilvessa() const { return kayttaja().id(); }
    QString finvoiceOsoite() const { return service("finvoice");}
    QString kbcOsoite() const { return service("bci");}    

public slots:
    void kirjauduUlos();
    void paivitaLista(int avaaPilvi = 0);
    void nimiMuuttui();

signals:
    void siirtoPilviAvattu();

public:
    void kirjautuminen(const QVariantMap& data, int avaaPilvi = 0);

private:    
    void pilviLisatty(QVariant* paluu);
    void poistettu();    
    void tarkistaKirjautuminen();

    void alusta();
    void lataaInit(QVariant* reply);
    void keskeytaLataus();

    void haeIlmoitusPaivitys();
    void paivitaIlmoitukset(QVariant* data);

    void paivitaPilviToken(QVariant* data);

private:    
    void asetaPilviLista(const QVariantList lista);

    QString avaamisVirheTeksti(int koodi) const;
    void virheAvaamisessa(int virhekoodi, const QString virheviesti);

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
    QTimer* ilmoitusTimer_ = nullptr;

    int avaaPilvi_ = 0;

    PaivitysInfo* paivitysInfo_;
    QProgressDialog* progressDialog_ = nullptr;

private:

    static QString pilviLoginOsoite__;    
};

#endif // PILVIMODEL_H
