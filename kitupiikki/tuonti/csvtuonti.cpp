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

#include <QTextCodec>
#include <QRegularExpression>
#include <QFile>


#include "csvtuonti.h"
#include "tuontisarakedelegaatti.h"
#include "tilimuuntomodel.h"

#include "kirjaus/tilidelegaatti.h"


#include "validator/ibanvalidator.h"
#include "validator/viitevalidator.h"

#include "kirjaus/kirjauswg.h"
#include "ui_tilimuuntodlg.h"

CsvTuonti::CsvTuonti(KirjausWg *wg)
    : Tuonti( wg ), ui( new Ui::CsvTuonti)
{
    ui->setupUi(this);
}

CsvTuonti::~CsvTuonti()
{
    delete ui;
}

bool CsvTuonti::tuoTiedosto(const QString &tiedostonnimi)
{
    QFile tiedosto(tiedostonnimi);
    tiedosto.open(QIODevice::ReadOnly | QIODevice::Text);

    return tuoCsv( tiedosto.readAll() );
}

bool CsvTuonti::tuoCsv(const QByteArray &data)
{
    tuoListaan( data );
    if( csv_.count() < 2)
        return false;


    ui->tuontiTable->setRowCount( muodot_.count() );
    ui->tiliEdit->suodataTyypilla("AR.*");

    TuontiSarakeDelegaatti* delegaatti = new TuontiSarakeDelegaatti();
    ui->tuontiTable->setItemDelegateForColumn(2, delegaatti);

    QStringList otsikot = csv_.first();

    connect( ui->kirjausRadio, SIGNAL(toggled(bool)), delegaatti, SLOT(asetaTyyppi(bool)));
    connect( ui->kirjausRadio, SIGNAL(toggled(bool)), this, SLOT(tarkistaTiliValittu()));
    connect( ui->tiliEdit, SIGNAL(editingFinished()), this, SLOT(tarkistaTiliValittu()));

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
        if( ui->kirjausRadio->isChecked())
        {
            QMap<int,int> muuntotaulukko;

            if( ui->muuntoRadio->isChecked())
            {
                // Tuodaan kirjauksia
                // Ensiksi muuntotaulukko
                QMap<int,QString> tilinimet;
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
                        if( tuonti == TILINUMERO)
                        {
                            QRegularExpressionMatch mats = tiliRe.match(tieto);
                            tilinro = mats.captured(1).toInt();
                            if( mats.captured(2).length() > 2)
                                tilinimi = mats.captured(2);
                        }
                        else if( tuonti == TILINIMI)
                            tilinimi = tieto;
                    }
                    tilinimet.insert(tilinro, tilinimi);
                }

                TiliMuuntoModel muuntomodel( tilinimet );

                QDialog muuntoDlg;
                Ui::TiliMuunto mUi;
                mUi.setupUi(&muuntoDlg);

                mUi.muuntoView->setModel( &muuntomodel);
                mUi.muuntoView->setItemDelegateForColumn( TiliMuuntoModel::UUSI , new TiliDelegaatti());
                mUi.muuntoView->horizontalHeader()->setStretchLastSection(true);
                connect( mUi.ohjeNappi, &QPushButton::clicked, []{ kp()->ohje("kirjaus/tuonti");});


                if( muuntoDlg.exec() != QDialog::Accepted )
                    return false;

                // Muuntotaulun haku
                muuntotaulukko = muuntomodel.muunnettu();

            }

            QRegularExpression numRe("\\d+");

            for(int r=1; r < csv_.count(); r++)
            {

                VientiRivi rivi;
                QString tositetunnus;
                QString selite;


                for( int c=0; c < muodot_.count(); c++)
                {
                    if( c >= csv_.at(r).count() )
                        continue;

                    int tuonti = ui->tuontiTable->item(c,2)->data(Qt::EditRole).toInt();
                    QString tieto = csv_.at(r).at(c);

                    if( tuonti == PAIVAMAARA )
                        if( muodot_.at(c) == SUOMIPVM)
                            rivi.pvm = QDate::fromString(tieto, "d.M.yyyy");
                        else
                            rivi.pvm = QDate::fromString(tieto, Qt::ISODate);
                    else if( tuonti == TOSITETUNNUS)
                        tositetunnus = tieto;
                    else if( tuonti == SELITE && !tieto.isEmpty())
                    {
                        if( !selite.isEmpty())
                            selite.append(" ");
                        selite.append(tieto);
                    }
                    else if( tuonti == TILINUMERO)
                    {
                        int nro = numRe.match(tieto).captured().toInt();
                        if( muuntotaulukko.isEmpty())
                            rivi.tili = kp()->tilit()->tiliNumerolla( nro );
                        else
                            rivi.tili = kp()->tilit()->tiliNumerolla( muuntotaulukko.value( nro) );
                    }
                    else if( tuonti == DEBETEURO)
                        rivi.debetSnt = tieto.remove('.').remove(',').toLongLong();
                    else if( tuonti == KREDITEURO)
                        rivi.kreditSnt = tieto.remove('.').remove(',').toLongLong();
                    else if( tuonti == KOHDENNUS)
                        rivi.kohdennus = kp()->kohdennukset()->kohdennus(tieto);
                }

                if( !tositetunnus.isEmpty())
                    rivi.selite = QString("%1 : %2").arg(tositetunnus).arg(selite);
                else
                    rivi.selite = selite;

                kirjausWg()->model()->vientiModel()->lisaaVienti(rivi);
            }

        }
        else
        {
            // Tiliote
            // Aluksi ei tiedetä aikaväliä
            tiliote( ui->tiliEdit->valittuTili());

            QDate alkaa;
            QDate loppuu;

            for(int r=1; r < csv_.count(); r++)
            {

                QDate pvm;
                qlonglong sentit;
                QString iban;
                QString viite;
                QString arkistotunnus;
                QString selite;


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
                        else
                            pvm = QDate::fromString(tieto, Qt::ISODate);
                    }
                    else if( tuonti == IBAN)
                    {
                        tieto.remove(' ');
                        // Jos suomalainen IBAN, poimitaan vain se. Tämä siksi, että
                        // samalla kentällä voi olla myös BIC-kenttä
                        if( tieto.startsWith("FI"))
                          tieto = tieto.left(18);
                        iban = tieto;
                    }
                    else if( tuonti == VIITE)
                        viite = tieto;
                    else if( tuonti == RAHAMAARA)
                        sentit = tieto.remove('.').remove(',').toLongLong();
                    else if( tuonti == SELITE && !tieto.isEmpty())
                    {
                        if( selite.isEmpty())
                            selite.append(" ");
                        selite.append(tieto);
                    }
                    else if( tuonti == ARKISTOTUNNUS)
                        arkistotunnus = tieto;

                }
                if( !alkaa.isValid() || pvm < alkaa)
                    alkaa = pvm;
                if( !loppuu.isValid() || pvm > loppuu)
                    loppuu = pvm;

                oterivi(pvm, sentit, iban, viite, arkistotunnus, selite);
            }
            tiliote(ui->tiliEdit->valittuTili(), alkaa, loppuu);
        }
    }


    return false;   // CSV-tiedostoa ei tallenneta tositteeksi
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

    for(int i=0; i < data.length(); i++)
    {
        QChar mki = data.at(i);
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
    if( puolipisteet > pilkut && puolipisteet > sarkaimet)
        return QChar(';');
    else if( sarkaimet > pilkut && sarkaimet > puolipisteet)
        return QChar('\t');
    else
        return QChar(',');
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
    case RAHA:
        return tr("Rahamäärä");
    case TILI:
        return tr("IBAN-tilinumero");
    case VIITE:
        return tr("Viitenumero");
    case SUOMIPVM:
    case ISOPVM:
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

    default:
        return QString();
    }
}

void CsvTuonti::paivitaOletukset()
{
    QStringList otsikot = csv_.first();

    bool pvmkaytetty = false;

    if( ui->kirjausRadio->isChecked())
    {
        for(int i=0; i < muodot_.count(); i++)
        {
            QString otsikko = otsikot.at(i);
            Sarakemuoto muoto = muodot_.at(i);

            if( (muoto == SUOMIPVM || muoto == ISOPVM) && !pvmkaytetty )
            {
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, PAIVAMAARA);
                pvmkaytetty = true;
            }
            else if( muoto == RAHA && otsikko.contains("debet", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, DEBETEURO);
            else if( muoto == RAHA && otsikko.contains("kredit", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, KREDITEURO);
            else if( otsikko.contains("tosite", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, TOSITETUNNUS);
            else if( otsikko.contains("selite", Qt::CaseInsensitive) ||
                     otsikko.contains("selitys", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, SELITE);
            else if( (otsikko=="Nro" && muoto == LUKU) ||
                     (otsikko.contains("tili", Qt::CaseInsensitive) && muoto == LUKUTEKSTI) )
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, TILINUMERO);
            else if( otsikko == "Kohdennus" )
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, KOHDENNUS);
            else if( otsikko == "Tili" && muoto == TEKSTI)
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, TILINIMI);
            else
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, EITUODA);
        }
    }
    else
    {
        bool rahakaytetty = false;

        for(int i=0; i < muodot_.count(); i++)
        {
            QString otsikko = otsikot.at(i);
            Sarakemuoto muoto = muodot_.at(i);

            if( (muoto == SUOMIPVM || muoto==ISOPVM) && !pvmkaytetty)
            {
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, PAIVAMAARA);
                pvmkaytetty = true;
            }
            else if( muoto == RAHA && !rahakaytetty)
            {
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, RAHAMAARA);
                rahakaytetty = true;
            }
            else if( muoto == TILI)
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, IBAN);
            else if( muoto == VIITE)
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, VIITENRO);
            else if( otsikko.contains("arkisto", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, ARKISTOTUNNUS);
            else if( otsikko.contains("selite", Qt::CaseInsensitive) ||
                     otsikko.contains("selitys", Qt::CaseInsensitive))
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, SELITE);
            else
                ui->tuontiTable->item(i,2)->setData(Qt::EditRole, EITUODA);
        }
    }
}

void CsvTuonti::tarkistaTiliValittu()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                ui->kirjausRadio->isChecked() || ui->tiliEdit->valittuTili().onkoValidi());
}

int CsvTuonti::tuoListaan(const QByteArray &data)
{
    QString kaikki = haistettuKoodattu(data);
    QStringList listana = kaikki.split(QRegularExpression("\\r?\\n"));
    if( listana.isEmpty())
        return 0;
    QChar erotin = haistaErotin(listana.first());

    // Nyt sitten luodaan rivi kerrallaan listaa
    for(QString rivi : listana)
    {
        QStringList nykyinenRivi;
        QString nykyinenSana;
        bool lainattuna;

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
            csv_.append(nykyinenRivi);
        }
    }
    // Tämän jälkeen sitten analysoidaan listaa eli mitä sisältää
    QRegularExpression suomipvmRe("^[0123]?\\d\\.[01]?\\d\\.\\d{4}$");
    QRegularExpression isopvmRe("^\\d{4}-[01]\\d-[0123]\\d$");
    QRegularExpression rahaRe("^[+-]?\\d*[.,]\\d{2}$");
    QRegularExpression lukuTekstiRe("^\\d+\\s.*");
    QRegularExpression lukuRe("^[+-]?\\d+$");


    // Muototauluun luetaan datasarakkeiden muoto
    // Jos yhdelläkin rivillä ei ole samassa muodossa, tulee muodoksi TEKSTI
    muodot_.resize( csv_.first().length() );

    for( int r = 1; r < csv_.count(); r++)
    {
        QStringList rivi = csv_.at(r);

        for(int i=0; i < qMin(rivi.length(), csv_.first().length()); i++)
        {
            QString teksti = rivi.at(i);
            Sarakemuoto muoto = TEKSTI;

            if( teksti.isEmpty())
                muoto = TYHJA;
            else if( teksti.count(suomipvmRe)  )
                muoto = SUOMIPVM;
            else if( teksti.count(isopvmRe))
                muoto = ISOPVM;
            // Tilinumeron kanssa samaan kenttään on voitu tunkea IBAN-joten kokeillaan
            // myös vähän muokatuilla versioilla
            else if( IbanValidator::kelpaako(teksti) ||
                     IbanValidator::kelpaako(teksti.left(18)) ||
                     IbanValidator::kelpaako(teksti.left(teksti.indexOf(QChar(' '))) )  )
                muoto = TILI;
            else if( ViiteValidator::kelpaako(teksti ) )
                muoto = VIITE;
            else if( teksti.contains(rahaRe))
                muoto = RAHA;
            else if( teksti.contains(lukuRe))
                muoto = LUKU;
            else if( teksti.contains(lukuTekstiRe))
                muoto = LUKUTEKSTI;

            if(muodot_[i] != TEKSTI && muodot_[i] != muoto && muoto != TYHJA)
            {
                if( muodot_[i] == TYHJA || (muodot_[i]==LUKU && muoto == LUKUTEKSTI))
                    muodot_[i] = muoto;
                else
                    muodot_[i] = TEKSTI;
            }
        }
    }

    return csv_.count();
}

