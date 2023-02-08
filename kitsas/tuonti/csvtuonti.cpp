/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "csvtuonti.h"
#include "tuontisarakedelegaatti.h"
#include "tilimuuntomodel.h"
#include "kirjaus/tilidelegaatti.h"
#include "db/kirjanpito.h"

#include "validator/ibanvalidator.h"
#include "validator/viitevalidator.h"

#include "ui_tilimuuntodlg.h"
#include "db/tositetyyppimodel.h"
#include "model/tositevienti.h"
#include "tuontiapu.h"

#include <QRegularExpression>
#include <QFile>
#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtCore5Compat/QTextCodec>
#else
#include <QTextCodec>
#endif


namespace Tuonti {



CsvTuonti::CsvTuonti()
    : ui( new Ui::CsvTuonti)
{
    ui->setupUi(this);
}

CsvTuonti::~CsvTuonti()
{
    delete ui;
}

QVariantMap CsvTuonti::tuo(const QByteArray &data)
{
    CsvTuonti tuonti;
    return tuonti.tuonti(data);
}

QVariantMap CsvTuonti::tuonti(const QByteArray &data)
{
    tuoListaan( data );
    if( csv_.count() < 2)
        return QVariantMap();

    ui->tuontiTable->setRowCount( muodot_.count() );
    ui->tiliCombo->suodataTyypilla("ARP");

    TuontiSarakeDelegaatti* delegaatti = new TuontiSarakeDelegaatti();
    ui->tuontiTable->setItemDelegateForColumn(2, delegaatti);

    QStringList otsikot = csv_.first();

    connect( ui->kirjausRadio, SIGNAL(toggled(bool)), delegaatti, SLOT(asetaTyyppi(bool)));
    connect( ui->kirjausRadio, SIGNAL(toggled(bool)), this, SLOT(tarkistaTiliValittu()));
    connect( ui->tiliCombo, &TiliCombo::tiliValittu, this, &CsvTuonti::tarkistaTiliValittu);

    // Arvioidaan, onko tiliote vai kirjaus
    if( otsikot.contains("arkistointitunnus", Qt::CaseInsensitive) ||
        muodot_.contains(TILI) || muodot_.contains(VIITE))
        ui->oteRadio->setChecked(true);


    // Täytetään ruutua
    for(int i=0; i < muodot_.count(); i++)
    {
        // Otsake
        QTableWidgetItem *otsikkoItem = new QTableWidgetItem( otsikot.at(i) );
        otsikkoItem->setFlags(Qt::ItemIsEnabled);
        ui->tuontiTable->setItem(i,0,otsikkoItem);

        QTableWidgetItem *tyyppiItem = new QTableWidgetItem( tyyppiTeksti( muodot_.at(i) ) );
        tyyppiItem->setFlags(Qt::ItemIsEnabled);
        ui->tuontiTable->setItem(i,1,tyyppiItem);

        QTableWidgetItem *tuontiItem = new QTableWidgetItem( 0 );
        tuontiItem->setData(TyyppiRooli, muodot_.at(i));
        ui->tuontiTable->setItem(i,2,tuontiItem);

        if( i < csv_.at(1).count() )
        {
            QTableWidgetItem *esimItem = new QTableWidgetItem( csv_.at(1).at(i));
            esimItem->setFlags(Qt::ItemIsEnabled);
            ui->tuontiTable->setItem(i,3,esimItem);
        }
    }
    paivitaOletukset();
    connect( ui->kirjausRadio, SIGNAL(toggled(bool)), this, SLOT(paivitaOletukset()));
    connect( ui->ohjeNappi, &QPushButton::clicked, []{ kp()->ohje("kirjaus/tuonti");});


    if( exec() == QDialog::Accepted )
    {
        if( ui->kirjausRadio->isChecked())  // Tuo kirjauksia
            return kirjaukset();
        else
            return tiliote();
    }    
    return QVariantMap();

}

QVariantMap CsvTuonti::kirjaukset()
{

    QMap<QString,int> muuntotaulukko;

    if( ui->muuntoRadio->isChecked())
    {
        // Tuodaan kirjauksia
        // Ensiksi muuntotaulukko
        QList<QPair<int,QString>> tilinimet;
        QRegularExpression tiliRe("(\\d+)\\s?(.*)");

        for(int r=1; r < csv_.count(); r++)
        {
            int tilinro = 0;
            QString tilinimi;
            for( int c=0; c < muodot_.count(); c++)
            {
                if( c  >= csv_.at(r).count() )
                    continue;   // Rivimäärä ei täsmää

                int tuonti = ui->tuontiTable->item(c,2)->data(Qt::EditRole).toInt();
                QString tieto = csv_.at(r).at(c);
                if( tuonti == DEBETTILI || tuonti == KREDITTILI) {
                    // #1126 Lisätään tilimuuntoon myös debet- ja kredit-tilit
                    QRegularExpressionMatch mats = tiliRe.match(tieto);
                    tilinro = mats.captured(1).toInt();
                    if( mats.captured(2).length() > 2)
                        tilinimi = mats.captured(2);
                    if( !tilinimet.contains(qMakePair(tilinro, tilinimi)))
                        tilinimet.append(qMakePair(tilinro, tilinimi));

                }
                if( tuonti == TILINUMERO )
                {
                    QRegularExpressionMatch mats = tiliRe.match(tieto);
                    tilinro = mats.captured(1).toInt();
                    if( mats.captured(2).length() > 2)
                        tilinimi = mats.captured(2);
                }
                else if( tuonti == TILINIMI)
                    tilinimi = tieto;
            }
            if( !tilinimet.contains(qMakePair(tilinro, tilinimi)))
                tilinimet.append(qMakePair(tilinro, tilinimi));
        }

        TiliMuuntoModel muuntomodel( tilinimet );

        QDialog muuntoDlg;
        Ui::TiliMuunto mUi;
        mUi.setupUi(&muuntoDlg);

        mUi.muuntoView->setModel( &muuntomodel);
        TiliDelegaatti* delegaatti = new TiliDelegaatti(this);
        delegaatti->etsiKayttoon(false);
        mUi.muuntoView->setItemDelegateForColumn( TiliMuuntoModel::UUSI , delegaatti);
        mUi.muuntoView->horizontalHeader()->setStretchLastSection(true);
        connect( mUi.ohjeNappi, &QPushButton::clicked, []{ kp()->ohje("kirjaus/tuonti");});


        if( muuntoDlg.exec() != QDialog::Accepted )
            return QVariantMap();

        // Muuntotaulun haku
        muuntotaulukko = muuntomodel.muunnettu();

    }
    QVariantMap map;
    map.insert("tyyppi", TositeTyyppi::TUONTI);
    QVariantList viennit;


    QRegularExpression numRe("\\d+");

    for(int r=1; r < csv_.count(); r++)
    {
        TositeVienti vienti;
        int kreditTili = 0;

        QString tositetunnus;
        QString selite;


        for( int c=0; c < muodot_.count(); c++)
        {
            if( c >= csv_.at(r).count() )
                continue;

            int tuonti = ui->tuontiTable->item(c,2)->data(Qt::EditRole).toInt();
            QString tieto = csv_.at(r).at(c);
            qlonglong sentit = TuontiApu::sentteina(tieto);

            if( tuonti == PAIVAMAARA )
                if( muodot_.at(c) == SUOMIPVM)
                    vienti.setPvm( QDate::fromString(tieto, "d.M.yyyy"));
                else if( muodot_.at(c) == ISOPVM )
                    vienti.setPvm( QDate::fromString(tieto, Qt::ISODate));
                else if(muodot_.at(c) == NDEAPVM )
                    vienti.setPvm( QDate::fromString(tieto, "yyyy/MM/dd"));
                else
                    vienti.setPvm( QDate::fromString(tieto, Qt::RFC2822Date));
            else if( tuonti == TOSITETUNNUS)
                tositetunnus = tieto;
            else if( tuonti == SELITE && !tieto.isEmpty())
            {
                if( !selite.isEmpty())
                    selite.append(" ");
                selite.append(tieto);
            }
            else if( tuonti == TILINUMERO || tuonti == DEBETTILI || tuonti == KREDITTILI)
            {
                int nro = numRe.match(tieto).captured().toInt();
                if(nro && !muuntotaulukko.isEmpty())
                    nro = muuntotaulukko.value(QString::number(nro));
                if( tuonti == KREDITTILI)
                    kreditTili = nro;
                else
                    vienti.setTili(nro);
            }
            else if( tuonti == DEBETEURO)
               vienti.setDebet(sentit);
            else if( tuonti == KREDITEURO)
                vienti.setKredit(sentit);
            else if( tuonti == RAHAMAARA)
            {
                if( sentit > 0)
                    vienti.setDebet(sentit);
                else
                    vienti.setKredit(0 - sentit);
            }
            else if( tuonti == KOHDENNUS)
                vienti.setKohdennus(tieto.toInt());
            else if( (tuonti == BRUTTOALVP || tuonti == ALVPROSENTTI) && sentit )
            {
                vienti.setAlvProsentti( sentit / 100.0 );
            }
            else if( tuonti == ALVKOODI && sentit)
            {
                vienti.setAlvKoodi(static_cast<int>(sentit / 100));
            }
            else if( tuonti == RAHASENTIT && tieto.toLongLong() < 100) {
                if( vienti.debetSnt())
                    vienti.setDebet( vienti.debetSnt() + tieto.toLongLong());
                else if(vienti.kreditSnt())
                    vienti.setKredit( vienti.kreditSnt() + tieto.toLongLong());

            }
        }

        if( !tositetunnus.isEmpty())
            vienti.setSelite( QString("%1 : %2").arg(tositetunnus).arg(selite));
        else
            vienti.setSelite( selite );

        if( vienti.alvProsentti() > 1e-5 && !vienti.alvKoodi())
        {
            if( vienti.debet() > 1e-6 )
                vienti.setAlvKoodi(AlvKoodi::OSTOT_BRUTTO);
            else
                vienti.setAlvKoodi(AlvKoodi::MYYNNIT_BRUTTO);
        }
        if(vienti.tili())
            viennit.append(vienti);

        if( kreditTili) {
            vienti.setTili(kreditTili);
            if( vienti.debet() > 1e-5)
                vienti.setKredit(vienti.debet());
            else
                vienti.setDebet(vienti.kredit());
            viennit.append(vienti);
        }
    }
    map.insert("viennit", viennit);
    return map;
}

QVariantMap CsvTuonti::tiliote()
{
    QVariantMap map;
    QVariantList tapahtumat;

    map.insert("tili", ui->tiliCombo->valittuTilinumero() );
    QDate alkaa;
    QDate loppuu;

    for(int r=1; r < csv_.count(); r++)
    {

        QVariantMap rivi;
        QDate pvm;

        for( int c=0; c < muodot_.count(); c++)
        {
            int tuonti = ui->tuontiTable->item(c,2)->data(Qt::EditRole).toInt();
            if( c >= csv_.at(r).count())
                continue;

            QString tieto = csv_.at(r).at(c);

            if( tuonti == PAIVAMAARA )
            {
                if( muodot_.at(c) == SUOMIPVM)
                    pvm = QDate::fromString(tieto, "d.M.yyyy");
                else if( muodot_.at(c) == ISOPVM )
                    pvm = QDate::fromString(tieto, Qt::ISODate);
                else
                    pvm = QDate::fromString(tieto, Qt::RFC2822Date);
                rivi.insert("pvm", pvm);
            }
            else if( tuonti == IBAN)
            {
                tieto.remove(' ');
                // Jos suomalainen IBAN, poimitaan vain se. Tämä siksi, että
                // samalla kentällä voi olla myös BIC-kenttä
                if( tieto.startsWith("FI"))
                  tieto = tieto.left(18);
                rivi.insert("iban", tieto);
            }
            else if( tuonti == VIITENRO ) {
                tieto.remove(' ');
                rivi.insert("viite", tieto);
            } else if( tuonti == RAHAMAARA)
            {
                rivi.insert("euro", Euro(TuontiApu::sentteina(tieto)).toString() );
            }
            else if( tuonti == RAHASENTIT) {
                Euro euro( rivi.value("euro").toString());
                if( euro.cents() > 0)
                    euro = euro + Euro(tieto.toLongLong());
                else
                    euro = euro - Euro(tieto.toLongLong());
                rivi.insert("euro", euro.toString());
            }
            else if( tuonti == SELITE && !tieto.isEmpty())
            {
                QString selite = rivi.value("selite").toString();
                if( !selite.isEmpty())
                    selite.append(" ");
                selite.append(tieto);
                rivi.insert("selite", selite);
            }
            else if( tuonti == ARKISTOTUNNUS)
                rivi.insert("arkistotunnus", tieto);
            else if( tuonti == KTOKOODI)
                rivi.insert("ktokoodi", tieto.toInt());
            else if( tuonti == SAAJAMAKSAJA)
                rivi.insert("saajamaksaja", tieto);

        }
        if( !alkaa.isValid() || pvm < alkaa)
            alkaa = pvm;
        if( !loppuu.isValid() || pvm > loppuu)
            loppuu = pvm;

        tapahtumat.append(rivi);

    }
    map.insert("tyyppi", 400);
    map.insert("alkupvm", alkaa);
    map.insert("loppupvm", loppuu);

    map.insert("tapahtumat", tapahtumat);

    return map;
}

QString CsvTuonti::haistettuKoodattu(const QByteArray &data)
{
    // Haistelee koodausta ääkkösten avulla
    // Vaihtoehtoina utf8, Latin1, 8859-15

    QRegularExpression skandit("[äöÄÖ€]");

    QString utf8 = QString::fromUtf8(data);
    if( utf8.contains(skandit))
        return utf8;

    QString latin1 = QString::fromLatin1(data);
    if( latin1.contains(skandit))
        return latin1;

    QString iso15 = QTextCodec::codecForName("ISO-8859-15")->toUnicode(data);
    if( iso15.contains(skandit))
        return iso15;

    // Ellei muuta, niin oletuksena tulee utf8
    return utf8;
}

QChar CsvTuonti::haistaErotin(const QString &data)
{
    // Päättelee, mikä on CSV-erottimena

    int pilkut = 0;
    int puolipisteet = 0;
    int sarkaimet = 0;

    bool lainattu = false;

    for(const QChar& mki : data)
    {
        if( mki == QChar('\"'))
            lainattu = !lainattu;
        if( !lainattu)
        {
            if( mki == QChar(','))
                pilkut++;
            else if( mki == QChar(';'))
                puolipisteet++;
            else if( mki == QChar('\t'))
                sarkaimet++;
        }
    }
    if( puolipisteet >= pilkut && puolipisteet > sarkaimet)
        return QChar(';');
    else if( sarkaimet > pilkut && sarkaimet > puolipisteet)
        return QChar('\t');
    else
        return QChar(',');
}

QList<QStringList> CsvTuonti::csvListana(const QByteArray &data)
{
    QList<QStringList> csv;

    QString kaikki = haistettuKoodattu(data);
    QStringList listana = kaikki.split(QRegularExpression("\\r?\\n"));
    if( listana.isEmpty())
        return {};
    QChar erotin = haistaErotin(listana.first());

    // Nyt sitten luodaan rivi kerrallaan listaa
    for(QString rivi : listana)
    {
        QStringList nykyinenRivi;
        QString nykyinenSana;
        bool lainattuna = false;

        for(int i = 0; i < rivi.length(); i++)
        {
            QChar merkki = rivi.at(i);
            if( merkki == QChar('"'))
            {
                if( lainattuna && rivi.length() > i+1 && rivi.at(i+1) == QChar('"'))
                {
                    nykyinenSana.append('"');
                    i++;
                }
                else
                    lainattuna = !lainattuna;
            }
            else if( !lainattuna && merkki == erotin )
            {
                // Erotin löytyi, sana tuli valmiiksi
                nykyinenRivi.append(nykyinenSana);
                nykyinenSana.clear();
            }
            else
            {
                // Muuten merkki lisätään paikalleen
                nykyinenSana.append(merkki);
            }
        }
        // Lopuksi viimeinen sana riville ja rivi tauluun
        if( nykyinenRivi.length())
        {
            nykyinenRivi.append(nykyinenSana);
            csv.append(nykyinenRivi);
        }
    }
    return csv;
}

QString CsvTuonti::tyyppiTeksti(int muoto)
{
    switch (muoto) {
    case TEKSTI:
        return tr("Teksti");
    case LUKUTEKSTI:
        return tr("Luku ja teksti");
    case LUKU:
        return tr("Luku");
    case ALLESATA:
        return tr("Pieni luku");
    case RAHA:
        return tr("Rahamäärä");
    case TILI:
        return tr("IBAN-tilinumero");
    case VIITE:
        return tr("Viitenumero");
    case SUOMIPVM:
    case ISOPVM:
    case USPVM:
    case NDEAPVM:
        return tr("Päivämäärä");        
    default:
        return QString();
    }
}

QString CsvTuonti::tuontiTeksti(int tuominen)
{
    switch (tuominen) {
    case PAIVAMAARA:
        return tr("Päivämäärä");
    case TOSITETUNNUS:
        return tr("Tositteen tunnus");
    case TILINUMERO:
        return tr("Tilin numero");
    case DEBETEURO:
        return tr("Debet euroa");
    case KREDITEURO:
        return tr("Kredit euroa");
    case SELITE:
        return tr("Selite");
    case RAHAMAARA:
        return tr("Määrä euroa");
    case IBAN:
        return tr("IBAN-tilinumero");
    case VIITENRO:
        return tr("Viitenumero");
    case ARKISTOTUNNUS:
        return tr("Arkistotunnus");
    case KOHDENNUS:
        return tr("Kohdennus");
    case TILINIMI:
        return tr("Tilin nimi");
    case BRUTTOALVP:
        return tr("Alv % Bruttokirjaus");
    case SAAJAMAKSAJA:
        return tr("Saajan/Maksajan nimi");
    case KTOKOODI:
        return tr("Tapahtuman lajikoodi");
    case ALVPROSENTTI:
        return tr("Alv %");
    case ALVKOODI:
        return tr("Alvkoodi");
    case DEBETTILI:
        return tr("Debet-tili");
    case KREDITTILI:
        return tr("Kredit-tili");
    case RAHASENTIT:
        return tr("Sentit");
    default:
        return QString();
    }
}

bool CsvTuonti::onkoCsv(const QByteArray &data)
{
    QByteArray kooditestiin = data.left(1024);
    int ulkona = 0;

    for( char merkki : kooditestiin)
    {
        if( merkki < 10 )
            ulkona++;
    }

    if( ulkona > kooditestiin.length() / 3)
        return false;

    QByteArray testattava = data.left(4096);

    QList<QStringList> lista = CsvTuonti::csvListana(testattava);

    // Kelpo CSV:ssä on alussakin vähintään 2 riviä, ja riveillä sama pituus
    if( lista.count() < 2 )
        return false;

    for(int i=1; i < lista.count()-1; i++)        
        if( lista.at(i).length() < 2 ||  lista.at(i).length() != lista.at(i-1).length())
            // OP:n csv:ssä vain otsikkorivi päättyy puolipisteeseen.
            // Siksi hyväksytään se, että rivi voi päättyä taikka olla päättymättä välimerkkiin
            if( !(lista.at(i-1).last().isEmpty() && lista.at(i-1).length() == lista.at(i).length()+1   ) &&
                !(lista.at(i).last().isEmpty() && lista.at(i).length() == lista.at(i+1).length() ))
                return false;

    return true;
}

void CsvTuonti::paivitaOletukset()
{
    QStringList otsikot = csv_.first();

    bool pvmkaytetty = false;

    if( ui->kirjausRadio->isChecked())  // Kirjauksia
    {
        for(int i=0; i < muodot_.count(); i++)
        {
            QString otsikko = otsikot.at(i).toLower() ;
            Sarakemuoto muoto = muodot_.at(i);

            if( (muoto == SUOMIPVM || muoto == ISOPVM || muoto == USPVM)  && !pvmkaytetty )
            {
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, PAIVAMAARA);
                pvmkaytetty = true;
            }
            else if( muoto == RAHA  && otsikko.contains("debet"))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, DEBETEURO);
            else if( muoto == RAHA  && otsikko.contains("kredit"))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, KREDITEURO);
            else if( otsikko.contains("tosite"))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, TOSITETUNNUS);
            else if( otsikko.contains("selite") ||
                     otsikko.contains("selitys") ||
                     otsikko.contains("kuvaus"))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, SELITE);
            else if( ((otsikko=="nro" || otsikko=="tilinumero" || otsikko=="tilin numero") && muoto == LUKU) ||
                     (otsikko.contains("tili") && muoto == LUKUTEKSTI) )
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, TILINUMERO);
            else if( otsikko == "kohdennus" )
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, KOHDENNUS);
            else if( (otsikko == "tili"  || otsikko == "luokka" ) && muoto == TEKSTI)
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, TILINIMI);
            else if( otsikko=="alv %")
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, BRUTTOALVP);
            else if( otsikko=="yhteensä" || otsikko.contains("euroa", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, RAHAMAARA);
            else if( otsikko=="alv%")
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, ALVPROSENTTI);
            else if( otsikko=="alvkoodi")
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, ALVKOODI);
            else if( otsikko.contains("debet",Qt::CaseInsensitive) && otsikko.contains("tili", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, DEBETTILI);
            else if( otsikko.contains("kredit",Qt::CaseInsensitive) && otsikko.contains("tili", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, KREDITTILI);
            else
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, EITUODA);
        }
    }
    else
    {
        bool rahakaytetty = false;
        bool senttikaytetty = false;
        bool kokoraha = false;

        for(int i=0; i < muodot_.count(); i++)
        {
            const QString& otsikko = otsikot.at(i);
            Sarakemuoto muoto = muodot_.at(i);

            if( (muoto == SUOMIPVM || muoto==ISOPVM || muoto == USPVM) && !pvmkaytetty)
            {
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, PAIVAMAARA);
                pvmkaytetty = true;
            }
            else if( (muoto == RAHA || muoto==LUKU || muoto==ALLESATA) && !rahakaytetty)
            {
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, RAHAMAARA);
                rahakaytetty = true;
                if( muoto == LUKU || muoto == ALLESATA)
                    kokoraha = true;    // Rahamäärä on kokonaisluku joten voidaan tuoda sentit erikseen
            }
            else if( muoto == TILI)
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, IBAN);
            else if( muoto == VIITE || otsikko.contains("viite", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, VIITENRO);
            else if( muoto == LUKU && otsikko.contains("laji", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, KTOKOODI);
            else if( otsikko.contains("arkisto", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, ARKISTOTUNNUS);
            else if((otsikko.contains("saaja", Qt::CaseInsensitive) ||
                     otsikko.contains("vastapuoli", Qt::CaseInsensitive) ||
                    otsikko.contains("maksaja", Qt::CaseInsensitive)) &&
                    !otsikko.contains("tili", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, SAAJAMAKSAJA);
            else if( otsikko.contains("selite", Qt::CaseInsensitive) ||
                     otsikko.contains("viesti", Qt::CaseInsensitive) ||
                     otsikko.contains("kuvaus", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, SELITE);
            else if( muoto == ALLESATA && rahakaytetty && !senttikaytetty && kokoraha) {
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, RAHASENTIT);
                senttikaytetty = true;
            } else
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, EITUODA);
        }
    }
}

void CsvTuonti::tarkistaTiliValittu()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                ui->kirjausRadio->isChecked() ||  ui->tiliCombo->valittuTilinumero() );
}

int CsvTuonti::tuoListaan(const QByteArray &data)
{
    csv_ = csvListana(data);

    // Tämän jälkeen sitten analysoidaan listaa eli mitä sisältää
    QRegularExpression suomipvmRe("^[0123]?\\d\\.[01]?\\d\\.\\d{4}$");
    QRegularExpression isopvmRe("^\\d{4}-[01]\\d-[0123]\\d$");
    QRegularExpression uspvmRe(R"(^\d\d [A-Z][a-z][a-z] 20\d\d( \d\d:\d\d:\d\d)?$)");
    QRegularExpression ndeaPvmRe(R"(\d{4}/[01]\d/[0123]\d)");
    QRegularExpression rahaRe("^[+-]?\\d+[.,]?\\d{0,2}$");
    QRegularExpression lukuTekstiRe("^\\d+\\s.*");
    QRegularExpression lukuRe("^[+-]?\\d+$");
    QRegularExpression valiRe("\\s");
    valiRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);


    // Muototauluun luetaan datasarakkeiden muoto
    // Jos yhdelläkin rivillä ei ole samassa muodossa, tulee muodoksi TEKSTI
    muodot_.resize( csv_.first().length() );

    for( int r = 1; r < csv_.count(); r++)
    {
        QStringList rivi = csv_.at(r);

        for(int i=0; i < qMin(rivi.length(), csv_.first().length()); i++)
        {
            const QString& teksti = rivi.at(i);
            QString valeitta = teksti;
            valeitta.remove(valiRe);

            Sarakemuoto muoto = TEKSTI;

            if( valeitta.isEmpty())
                muoto = TYHJA;
            else if( teksti.count(suomipvmRe)  )
                muoto = SUOMIPVM;
            else if( teksti.count(isopvmRe))
                muoto = ISOPVM;
            else if( teksti.contains(uspvmRe))
                muoto = USPVM;
            else if( teksti.contains(ndeaPvmRe))
                muoto = NDEAPVM;
            // Tilinumeron kanssa samaan kenttään on voitu tunkea IBAN-joten kokeillaan
            // myös vähän muokatuilla versioilla
            else if( !valeitta.startsWith("RF") &&
                     ( IbanValidator::kelpaako(valeitta) ||
                     IbanValidator::kelpaako(valeitta.left(18)) ||
                     IbanValidator::kelpaako(teksti.left(teksti.indexOf(QChar(' '))) ))  )
                muoto = TILI;
            else if( ViiteValidator::kelpaako(valeitta ) )
                muoto = VIITE;
            else if( teksti.contains(lukuRe)) {
                if( teksti.toLongLong() > -1 && teksti.toLongLong() < 100)
                    muoto = ALLESATA;
                else
                    muoto = LUKU;
            } else if( valeitta.contains(rahaRe))
                muoto = RAHA;
            else if( teksti.contains(lukuTekstiRe))
                muoto = LUKUTEKSTI;


            if(muodot_[i] != TEKSTI && muodot_[i] != muoto && muoto != TYHJA)
            {
                if( muodot_[i] == TYHJA || (muodot_[i]==LUKU && muoto == LUKUTEKSTI) ||                        
                        (muodot_[i]==LUKU && muoto==RAHA) ||
                        (muodot_[i]==ALLESATA && muoto==LUKU) ||
                        (muodot_[i]==ALLESATA && muoto==RAHA))
                    muodot_[i] = muoto;
                else if( muodot_[i]==RAHA && ( muoto==LUKU || muoto == ALLESATA || muoto==VIITE))
                    muodot_[i] = RAHA;
                else if( (muodot_[i] == LUKU && muoto == VIITE ) || (muodot_[i] == VIITE && muoto == LUKU) )
                    muodot_[i] = LUKU;
                else
                    muodot_[i] = TEKSTI;
            }
        }
    }

    return csv_.count();
}

}

