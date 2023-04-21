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
#ifndef ALVLASKELMA_H
#define ALVLASKELMA_H

#include "raportti/raportteri.h"
#include "model/tositevienti.h"

#include <QMap>
#include <QVariantMap>
#include <QHash>
#include <QSet>
#include <QQueue>

class Tosite;

/**
 * @brief Arvonlisälaskelman laatiminen
 *
 */
class AlvLaskelma : public Raportteri
{               
    Q_OBJECT

protected:
    struct TiliTaulu {
        QList<QVariantMap> viennit;
        void lisaa(const QVariantMap& rivi);
        Euro summa(bool debetista = false) const;
    };

    struct KantaTaulu {
        void lisaa(const QVariantMap& rivi);
        Euro summa(bool debetista = false) const;

        QMap<int, TiliTaulu> tilit;
    };

    struct KoodiTaulu {
        void lisaa(const QVariantMap& rivi);
        Euro summa(bool debetista = false) const;

        QMap<int, KantaTaulu> kannat;
    };

    struct AlvTaulu {
        void lisaa(const QVariantMap& rivi);
        Euro summa(int koodista, int koodiin = 0);

        QMap<int, KoodiTaulu> koodit;
    };


public:
    explicit AlvLaskelma(QObject *parent = nullptr, const QString kielikoodi = QString());
    ~AlvLaskelma() override;

    void kirjoitaLaskelma();

    Euro maksettava() const { return maksettava_;}
    Euro huojennus() const { return huojennus_;}

    QDate loppupvm() const { return loppupvm_;}

    static int huojennusKuukaudet(const QDate& alku, const QDate& loppu);

signals:
    void tallennettu();
    void ilmoitusVirhe(const QString& koodi, const QString& viesti);

public slots:
    void laske(const QDate& alkupvm, const QDate& loppupvm);
    void kirjaaHuojennus();
    void valmisteleTosite();

    void ilmoitaJaTallenna(const QString korjaus = QString(), bool huojennus = false);
    void tallenna();

protected:
    void viennitSaapuu(QVariant* viennit);
    void haeHuojennusJosTarpeen();
    void laskeHuojennus(QVariant* viennit);
    void tallennusValmis();
    void ilmoitettu(QVariant* data);

protected:
    void viimeistele();
    void viimeViimeistely();
    void hae();
    void lisaaKirjausVienti(TositeVienti vienti);

    void oikaiseBruttoKirjaukset();

    void kirjoitaOtsikot();
    void kirjoitaYhteenveto();
    void kirjaaVerot();
    void kirjoitaErittely();
    void yvRivi(int koodi, const QString& selite, Euro euro);
    Euro kotimaanmyyntivero(int prosentinsadasosa);

    void tilaaNollausLista(const QDate& pvm, bool palautukset = false);
    void nollaaMaksuperusteisetErat(QVariant* variant, const QDate &pvm);
    void nollaaMaksuperusteinenEra(QVariant* variant);

    void laskeMarginaaliVerotus(int kanta);
    void marginaaliRivi(const QString selite, int kanta, Euro summa);

protected:
    QDate alkupvm_;
    QDate loppupvm_;
    QDate huojennusalku_;

    AlvTaulu taulu_;
    QMap<int,Euro> koodattu_;

    QHash<int,QPair<int,Euro>> maksuperusteiset_;
    QList<int> maksuperusteTositteet_;
    QList<QPair<int,Euro>> nollattavatErat_;
    QSet<int> nollatutErat_;
    int nollattavatHaut_ = 0;

    QQueue<int> nollattavatMaksuperusteErat_;

    Euro maksettava_;

    Euro liikevaihto_;
    Euro verohuojennukseen_;
    int suhteutuskuukaudet_ = 12;
    Euro huojennus_;

    QList<RaporttiRivi> marginaaliRivit_;
    QVariantMap marginaaliAlijaamat_;

    Tosite* tosite_;


public:
    static bool debetistaKoodilla(int alvkoodi);

    friend class AlvLaskelmaTest;
};

#endif // ALVLASKELMA_H
