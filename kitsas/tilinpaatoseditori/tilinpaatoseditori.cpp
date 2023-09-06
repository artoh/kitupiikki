/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QDesktopServices>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSqlQuery>
#include <QJsonDocument>
#include <QVariant>
#include <QNetworkReply>

#include "tilinpaatoseditori.h"
#include "tilinpaatostulostaja.h"
#include "db/kirjanpito.h"
#include "tpaloitus.h"
#include "naytin/naytinikkuna.h"
#include "arkisto/tilinpaattaja.h"

TilinpaatosEditori::TilinpaatosEditori(const Tilikausi& tilikausi, QWidget *parent)
    : QMainWindow(parent),
      tilikausi_(tilikausi)
{
    editori_ = new MRichTextEdit;
    setCentralWidget( editori_);
    setWindowTitle( tr("Tilinpäätöksen liitetiedot %1").arg(tilikausi.kausivaliTekstina()));

    setWindowIcon( QIcon(":/pic/Possu64.png"));

    TilinPaattaja* paattaja = qobject_cast<TilinPaattaja*>(parent);
    if(paattaja) {
        connect( this, &TilinpaatosEditori::tallennettu, paattaja, &TilinPaattaja::paivitaDialogi);
    }

    luoAktiot();
    luoPalkit();

    setAttribute(Qt::WA_DeleteOnClose);

    lataa();
}

TilinpaatosEditori::~TilinpaatosEditori()
{

}


void TilinpaatosEditori::esikatselu()
{    
    TilinpaatosTulostaja *tp = new TilinpaatosTulostaja(tilikausi_, editori_->toHtml(),
                                                        raportit_,  kp()->asetukset()->asetus(AsetusModel::TilinpaatosKieli), pvmAktio_->isChecked(), this);
    tp->nayta();
}

void TilinpaatosEditori::luoAktiot()
{
    tallennaAktio_ = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tallenna"), this);
    connect( tallennaAktio_, SIGNAL(triggered(bool)), this, SLOT(tallenna()));

    esikatseleAction_ = new QAction( QIcon(":/pic/print.png"), tr("Esikatsele"), this);
    connect( esikatseleAction_, SIGNAL(triggered(bool)), this, SLOT(esikatselu()));

    aloitaUudelleenAktio_ = new QAction( QIcon(":/pic/uusitiedosto.png"), tr("Aloita uudelleen"), this);
    connect( aloitaUudelleenAktio_, SIGNAL(triggered(bool)), this, SLOT(aloitaAlusta()));

    valmisAktio_ = new QAction( QIcon(":/pic/ok.png"), tr("Valmis"), this);
    connect( valmisAktio_, &QAction::triggered, this, &TilinpaatosEditori::valmis);

    pvmAktio_ = new QAction( QIcon(":/pic/calendar.png"), tr("Näytä pvm"), this);
    pvmAktio_->setCheckable(true);
    pvmAktio_->setChecked( kp()->asetukset()->onko(AsetusModel::NaytaTilinpaatoksenLaatimisPvm) );
    pvmAktio_->setToolTip(tr("Tulostuspäivämäärä tilinpäätöksen oikeassa yläkulmassa"));

    ohjeAktio_ = new QAction( QIcon(":/pic/ohje.png"), tr("Ohje"), this);
    connect( ohjeAktio_, SIGNAL(triggered(bool)), this, SLOT(ohje()));

    undoAction_ = new QAction( QIcon(":/pic/edit-undo.png"), tr("Kumoa"), this);
    redoAction_ = new QAction( QIcon(":/pic/edit-redo.png"), tr("Toista"), this);

    cutAction_ = new QAction( QIcon(":/pic/edit-cut.png"), tr("Leikkaa"), this);
    copyAction_ = new QAction( QIcon(":/pic/edit-copy.png"), tr("Kopioi"), this);
    pasteAction_ = new QAction( QIcon(":/pic/edit-paste.png"), tr("Liitä"), this);

    boldAction_ = new QAction( QIcon(":/pic/lihavoi.png"), tr("Lihavoi"), this);
    italicAction_ = new QAction( QIcon(":/pic/format-italic.png"), tr("Kursivoi"), this);

}

void TilinpaatosEditori::luoPalkit()
{
    tilinpaatosTb_ = addToolBar( tr("&Tilinpäätös"));
    tilinpaatosTb_->addAction( aloitaUudelleenAktio_ );
    tilinpaatosTb_->addSeparator();
    tilinpaatosTb_->addAction( esikatseleAction_ );
    tilinpaatosTb_->addAction( tallennaAktio_ );
    tilinpaatosTb_->addSeparator();
    tilinpaatosTb_->addAction( valmisAktio_);
    tilinpaatosTb_->addSeparator();
    tilinpaatosTb_->addAction( pvmAktio_ );
    tilinpaatosTb_->addSeparator();
    tilinpaatosTb_->addAction( ohjeAktio_ );
    tilinpaatosTb_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    muokkausTb_ = addToolBar( tr("&Muokkaus"));
    muokkausTb_->addAction(undoAction_);
    muokkausTb_->addAction(redoAction_);

    muokkausTb_->addSeparator();

    muokkausTb_->addAction(cutAction_);
    muokkausTb_->addAction(copyAction_);
    muokkausTb_->addAction(pasteAction_);

    muokkausTb_->addSeparator();

    tekstiTyyppiCombo_ = new QComboBox();

    tekstiTyyppiCombo_->addItems( QStringList() << tr("Tavallinen teksti") << tr("Otsikko") << tr("Alaotsikko") << tr("Kirjoituskone"));
    muokkausTb_->addWidget(tekstiTyyppiCombo_);

    muokkausTb_->addSeparator();
    muokkausTb_->addAction( boldAction_ );
    muokkausTb_->addAction( italicAction_);

    muokkaudTb_->addSeparator();





}

void TilinpaatosEditori::uusiTp()
{
    QStringList pohja =  kp()->asetukset()->lista("tppohja/" + kp()->asetukset()->asetus(AsetusModel::TilinpaatosKieli));

    QStringList valinnat = kp()->asetukset()->asetus(AsetusModel::TilinpaatosValinnat).split(",");
    QRegularExpression tunnisteRe("#(?<tunniste>-?\\w+)(?<pois>(\\s-\\w+)*).*");
    tunnisteRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression raporttiRe("@(.+)(:\\w*)?[!](.+)@");
    
    QRegularExpression poisRe("-(?<pois>\\w+)");
    poisRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);

    QString teksti;
    raportit_.clear();

    bool tulosta = true;

    foreach (QString rivi, pohja)
    {
        if( rivi.startsWith('#') )
        {
            QRegularExpressionMatch tmats = tunnisteRe.match(rivi);
            if( tmats.hasMatch())
            {
                // Tulostetaan, jos haluttu tunniste valittu ei ei-ehdolla ei valittu ;)
                QString tunniste = tmats.captured(1);
                if( tunniste.startsWith('-'))
                    tulosta = !valinnat.contains( tunniste.mid(1));
                else
                    tulosta = valinnat.contains(tunniste);

                // Kuitenkin jos rivillä myös -eiehto, niin ei kuitenkaan tulosteta
                QString pois = tmats.captured("pois");
                if( !pois.isEmpty())
                {
                    QStringList poisTunnukset;
                    QRegularExpressionMatchIterator iter = poisRe.globalMatch(pois);
                    while( iter.hasNext())
                    {
                        QString poistunnus = iter.next().captured(1);
                        if( valinnat.contains(poistunnus))
                        {
                            tulosta = false;
                            break;
                        }
                    }
                }
                
            }
            else
                // Pelkkä # tai kelvoton ehto niin tulostetaan joka tapauksessa
                tulosta = true;
        }
        else if( rivi.startsWith('@') && tulosta)
        {
            // Näillä tulostetaan erityisiä kenttiä
            if( rivi.startsWith("@henkilosto@"))
                teksti.append( henkilostotaulukko(rivi.mid(12)));
            else if( rivi == "@tulos@")
                teksti.append( QString(" %L1 € ").arg( tilikausi_.tulos() ) );
            else
            {
                QRegularExpressionMatch rmats = raporttiRe.match(rivi);
                if( rmats.hasMatch())
                {
                    raportit_.append( rmats.captured() );
                }
            }
        }
        else if( tulosta )
            teksti.append(rivi);
    }
    editori_->setFont( QFont("FreeSans", 12));
    editori_->setText(teksti);
}

void TilinpaatosEditori::lataa()
{
    KpKysely* haku = kpk(QString("/liitteet/0/TPTEKSTI_%1").arg(tilikausi_.paattyy().toString(Qt::ISODate)));
    connect( haku, &KpKysely::vastaus, this, &TilinpaatosEditori::tekstiSaapuu);
    connect( haku, &KpKysely::virhe, this, &TilinpaatosEditori::eitekstia);
    haku->kysy();
}

void TilinpaatosEditori::tekstiSaapuu(QVariant *data)
{
    QString teksti = QString::fromUtf8(data->toByteArray());    
    QString raporttirivi = teksti.left( teksti.indexOf("\n"));
    QRegularExpression raporttiRe("@.*?@");
    QRegularExpressionMatchIterator matsi = raporttiRe.globalMatch(raporttirivi);
    raportit_.clear();
    while(matsi.hasNext()) {
        QRegularExpressionMatch mats = matsi.next();
        raportit_.append(mats.captured());
    }

    editori_->setText( teksti.mid(teksti.indexOf("\n")+1));
    tallennettu_ = editori_->toHtml();
}

void TilinpaatosEditori::eitekstia(int virhe)
{
    tallennettu_ = editori_->toHtml();
    if( virhe == QNetworkReply::ContentNotFoundError) {
        if( !aloitaAlusta()) {
            close();
        }
    } else {
        QMessageBox::critical(this, tr("Tilinpäätöstekstin lataaminen"), tr("Tilinpäätöstekstin lataaminen epäonnistui"));
    }
}

void TilinpaatosEditori::closeEvent(QCloseEvent *event)
{

    if( editori_->toHtml() != tallennettu_)
    {
        QMessageBox::StandardButton vastaus =
                QMessageBox::question(this, tr("Tilinpäätöstä muokattu"),
                                      tr("Tallennetaanko muokattu tilinpäätös?"),
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                      QMessageBox::Cancel);
        if( vastaus == QMessageBox::Yes)
        {
            tallenna();
            event->accept();
        }
        else if( vastaus == QMessageBox::Cancel)
            event->ignore();
        else    // Jos halutaan kuitenkin sulkea...
            event->accept();
    }
    else
        event->accept();

}

QString TilinpaatosEditori::henkilostotaulukko(const QString &teksti)
{
    Tilikausi verrokki;
    if( kp()->tilikaudet()->indeksiPaivalle( tilikausi_.paattyy()))
        verrokki = kp()->tilikaudet()->tilikausiIndeksilla(  kp()->tilikaudet()->indeksiPaivalle(tilikausi_.paattyy()) - 1 );

    QString txt = tr("<table width=100%><tr><td></td><td align=center>%1</td>").arg(tilikausi_.kausivaliTekstina());
    if( verrokki.alkaa().isValid() )
        txt.append( QString("<td align=center>%1</td>").arg(verrokki.kausivaliTekstina()) );

    txt.append(tr("</tr><tr><td>%1</td><td align=center>%2</td>").arg(teksti).arg( kp()->tilikaudet()->tilikausiPaivalle( tilikausi_.paattyy() ).henkilosto()));
    if( verrokki.alkaa().isValid())
        txt.append( QString("<td align=center>%1</td>").arg(verrokki.henkilosto()));
    txt.append("</tr></table>");
    return txt;
}


bool TilinpaatosEditori::aloitaAlusta()
{
    TpAloitus tpaloitus(tilikausi_);
    if( tpaloitus.exec() == QDialog::Accepted)
    {
        uusiTp();   // Aloitetaan alusta
    }
    else
    {
        emit tallennettu();
        return false;
    }
    return true;
}

void TilinpaatosEditori::tallenna()
{    

    QString teksti = raportit_.join(" ") + "\n" + editori_->toHtml();
    tallennettu_ = editori_->toHtml();

    KpKysely *tallennus = kpk(QString("/liitteet/0/TPTEKSTI_%1").arg(tilikausi_.paattyy().toString(Qt::ISODate)), KpKysely::PUT);
    QMap<QString,QString> meta;
    meta.insert("Content-type","text/plain");
    meta.insert("Filename",tilikausi_.arkistoHakemistoNimi() + ".txt");
    tallennus->lahetaTiedosto(teksti.toUtf8(),meta);

    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle(tilikausi_.paattyy());
    kp()->asetukset()->aseta("tilinpaatos/" + tilikausi_.paattyy().toString(Qt::ISODate), teksti);
    kausi.set("tilinpaatos", QDateTime::currentDateTime());
    kausi.tallenna();

    kp()->asetukset()->aseta(AsetusModel::NaytaTilinpaatoksenLaatimisPvm, pvmAktio_->isChecked());

    TilinpaatosTulostaja *tp = new TilinpaatosTulostaja(tilikausi_, editori_->toHtml(), raportit_,
                                                        kp()->asetukset()->asetus(AsetusModel::TilinpaatosKieli),
                                                        pvmAktio_->isChecked(),this);
    connect( tp, &TilinpaatosTulostaja::tallennettu, this, &TilinpaatosEditori::tallennettu);
    connect( tp, &TilinpaatosTulostaja::tallennettu, [] { emit kp()->onni(tr("Tilinpäätös tallennettu")); });
    tp->tallenna();

}

void TilinpaatosEditori::valmis()
{
    connect(this, &TilinpaatosEditori::tallennettu, this, &TilinpaatosEditori::close);
    tallenna();
}


void TilinpaatosEditori::ohje()
{
    kp()->ohje("tilikaudet/tilinpaatos/liitetiedot/");
}
