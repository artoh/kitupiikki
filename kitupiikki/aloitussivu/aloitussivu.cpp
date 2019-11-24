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

#include <QFile>
#include <QStringList>
#include <QSqlQuery>
#include <QSslSocket>
#include <QNetworkRequest>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QListWidget>
#include <QMessageBox>

#include <QRegularExpression>

#include <QDialog>
#include <QDebug>

#include <QSettings>
#include <QSysInfo>

#include "ui_aboutdialog.h"
#include "ui_muistiinpanot.h"

#include "aloitussivu.h"
#include "db/kirjanpito.h"
#include "alv/alvsivu.h"
#include "versio.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvilogindlg.h"
#include "sqlite/sqlitemodel.h"

#include "uusikirjanpito/uusivelho.h"
#include "maaritys/tilikarttapaivitys.h"

#include <QJsonDocument>
#include <QTimer>
#include <QSortFilterProxyModel>

#include "tilaus/tilauswizard.h"
#include "versio.h"

AloitusSivu::AloitusSivu(QWidget *parent) :
    KitupiikkiSivu(parent)
{

    ui = new Ui::Aloitus;
    ui->setupUi(this);

    // Tässä julkaisussa pilvi ei käytössä
#ifndef KITSAS_DEVEL
    ui->tkpilviTab->setTabEnabled(1, false);
#endif

    ui->selain->setOpenLinks(false);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &AloitusSivu::uusiTietokanta);
    connect( ui->avaaNappi, &QPushButton::clicked, this, &AloitusSivu::avaaTietokanta);
    connect( ui->tietojaNappi, SIGNAL(clicked(bool)), this, SLOT(abouttiarallaa()));
    connect( ui->tilikausiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(siirrySivulle()));
    connect(ui->varmistaNappi, &QPushButton::clicked, this, &AloitusSivu::varmuuskopioi);
    connect(ui->muistiinpanotNappi, &QPushButton::clicked, this, &AloitusSivu::muistiinpanot);
    connect(ui->poistaNappi, &QPushButton::clicked, this, &AloitusSivu::poistaListalta);

    connect( ui->tilikausiCombo, &QComboBox::currentTextChanged, this, &AloitusSivu::haeSaldot);    

    connect( ui->selain, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkki(QUrl)));

    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(kirjanpitoVaihtui()));
    connect( kp(), SIGNAL( perusAsetusMuuttui()), this, SLOT(kirjanpitoVaihtui()));

    connect( ui->loginButton, &QPushButton::clicked, this, &AloitusSivu::pilviLogin);
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &AloitusSivu::kirjauduttu);
    connect( kp()->pilvi(), &PilviModel::loginvirhe, this, &AloitusSivu::loginVirhe);
    connect( ui->logoutButton, &QPushButton::clicked, this, &AloitusSivu::pilviLogout );
    connect( ui->rekisteroiButton, &QPushButton::clicked, this, &AloitusSivu::rekisteroi);
    connect(ui->salasanaButton, &QPushButton::clicked, this, &AloitusSivu::rekisteroi);

    connect( ui->viimeisetView, &QListView::clicked,
             [] (const QModelIndex& index) { kp()->sqlite()->avaaTiedosto( index.data(SQLiteModel::PolkuRooli).toString() );} );

    connect( ui->pilviView, &QListView::clicked,
             [](const QModelIndex& index) { kp()->pilvi()->avaaPilvesta( index.data(PilviModel::IdRooli).toInt() ); } );

    connect( ui->emailEdit, &QLineEdit::textChanged, this, &AloitusSivu::validoiEmail );
    connect( ui->salaEdit, &QLineEdit::textChanged, this, &AloitusSivu::validoiLoginTiedot);

    connect( ui->tilausButton, &QPushButton::clicked,
             [] () { TilausWizard *tilaus = new TilausWizard(); tilaus->nayta(); });


    connect( kp(), &Kirjanpito::logoMuuttui, this, &AloitusSivu::logoMuuttui);

    QSortFilterProxyModel* sqliteproxy = new QSortFilterProxyModel(this);
    sqliteproxy->setSourceModel( kp()->sqlite());
    ui->viimeisetView->setModel( sqliteproxy );
    sqliteproxy->setSortRole(Qt::DisplayRole);

    ui->pilviView->setModel( kp()->pilvi() );
    ui->tkpilviTab->setCurrentIndex( kp()->settings()->value("TietokonePilviValilehti").toInt() );
    ui->vaaraSalasana->setVisible(false);

    if( kp()->settings()->contains("CloudKey"))
        QTimer::singleShot(250, [](){ kp()->pilvi()->kirjaudu(); });


}

AloitusSivu::~AloitusSivu()
{
    kp()->settings()->setValue("TietokonePilviValilehti", ui->tkpilviTab->currentIndex() );
    delete ui;
}


void AloitusSivu::siirrySivulle()
{

    if( ui->tilikausiCombo->currentIndex() < 0)
        ui->tilikausiCombo->setCurrentIndex( kp()->tilikaudet()->indeksiPaivalle( QDate::currentDate() ) );
    if( ui->tilikausiCombo->currentIndex() < 0)
        ui->tilikausiCombo->setCurrentIndex( ui->tilikausiCombo->count()-1 );

    if( !sivulla )
    {
        connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &AloitusSivu::siirrySivulle);
        sivulla = true;
    }

    // Päivitetään aloitussivua
    if( kp()->yhteysModel() )
    {        
        haeSaldot();
    }
    else
    {
        QFile tttiedosto(":/aloitus/tervetuloa.html");
        tttiedosto.open(QIODevice::ReadOnly);
        QTextStream in(&tttiedosto);
        in.setCodec("Utf8");
        QString teksti = in.readAll();
        teksti.replace("<INFO>", paivitysInfo);

        ui->selain->setHtml( teksti );
    }

}

bool AloitusSivu::poistuSivulta(int /* minne */)
{
    disconnect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &AloitusSivu::siirrySivulle);
    sivulla = false;
    return true;
}

void AloitusSivu::kirjanpitoVaihtui()
{
    bool avoinna = kp()->yhteysModel();

    ui->nimiLabel->setVisible(avoinna && !kp()->asetukset()->onko("LogossaNimi") );
    ui->tilikausiCombo->setVisible(avoinna );
    ui->logoLabel->setVisible( avoinna && !kp()->logo().isNull());
    ui->varmistaNappi->setEnabled(avoinna);
    ui->muistiinpanotNappi->setEnabled(avoinna);
    ui->poistaNappi->setEnabled( avoinna );

    if( avoinna )
    {
        // Kirjanpito avattu
        ui->nimiLabel->setText( kp()->asetukset()->asetus("Nimi"));

        ui->tilikausiCombo->setModel( kp()->tilikaudet() );
        ui->tilikausiCombo->setModelColumn( 0 );

        // Valitaan nykyinen tilikausi
        // Pohjalle kuitenkin viimeinen tilikausi, jotta joku on aina valittuna
        ui->tilikausiCombo->setCurrentIndex( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );

        for(int i=0; i < kp()->tilikaudet()->rowCount(QModelIndex());i++)
        {
            Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla(i);
            if( kausi.alkaa() <= kp()->paivamaara() && kausi.paattyy() >= kp()->paivamaara())
            {
                ui->tilikausiCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    if( !kp()->asetus("Tilikartta").isEmpty() )
        kp()->settings()->setValue("Tilikartta", kp()->asetus("Tilikartta"));


    if( paivitysInfo.isEmpty())
        pyydaInfo();

    ui->pilviKuva->setVisible( qobject_cast<PilviModel*>( kp()->yhteysModel()  ) );

    siirrySivulle();
}

void AloitusSivu::linkki(const QUrl &linkki)
{
    if( linkki.scheme() == "ohje")
    {
        kp()->ohje( linkki.path() );
    }
    else if( linkki.scheme() == "selaa")
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->tilikausiCombo->currentIndex() );
        QString tiliteksti = linkki.fileName();
        emit selaus( tiliteksti.toInt(), kausi );
    }
    else if( linkki.scheme() == "ktp")
    {
        QString toiminto = linkki.path().mid(1);

        if( toiminto == "uusi")
            uusiTietokanta();
        else
            emit ktpkasky(toiminto);
    }
    else if( linkki.scheme().startsWith("http"))
    {        
        Kirjanpito::avaaUrl( linkki );
    }
}




void AloitusSivu::uusiTietokanta()
{
    UusiVelho velho;
    if( velho.exec() ) {
        if( velho.field("pilveen").toBool())
            kp()->pilvi()->uusiPilvi(velho.data());
        else {
            if(kp()->sqlite()->uusiKirjanpito(velho.polku(), velho.data())) {
                kp()->sqlite()->avaaTiedosto(velho.polku());
            }
        }
    }
}

void AloitusSivu::avaaTietokanta()
{
    QString polku = QFileDialog::getOpenFileName(this, "Avaa kirjanpito",
                                                 QDir::homePath(),"Kirjanpito (*.kitsas)");
    if( !polku.isEmpty())
        kp()->sqlite()->avaaTiedosto( polku );

}

void AloitusSivu::abouttiarallaa()
{
    Ui::AboutDlg aboutUi;
    QDialog aboutDlg;
    aboutUi.setupUi( &aboutDlg);
    connect( aboutUi.aboutQtNappi, &QPushButton::clicked, qApp, &QApplication::aboutQt);

    QString versioteksti = tr("<b>Versio %1</b><br>Käännetty %2")
            .arg( qApp->applicationVersion())
            .arg( buildDate().toString("dd.MM.yyyy"));

    QString kooste(KITSAS_BUILD);
    if( !kooste.isEmpty())
        versioteksti.append("<br>Kooste " + kooste);

    aboutUi.versioLabel->setText(versioteksti);

    aboutDlg.exec();
}

void AloitusSivu::infoSaapui()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    QString info = QString::fromUtf8( reply->readAll() );
    if( info.startsWith("KITUPIIKKI"))
    {
        // Tarkistetaan tunniste, jotta palvelinvirhe ei tuota sontaa näytölle
        paivitysInfo = info.mid(11);
        siirrySivulle();
    }
    reply->deleteLater();
}

void AloitusSivu::varmuuskopioi()
{
    QString tiedosto = kp()->sqlite()->tiedostopolku();
    QFileInfo info(tiedosto);
    QString polku = QString("%1/%2-%3.kitsas")
            .arg(QDir::homePath())
            .arg(info.baseName())
            .arg( QDate::currentDate().toString("yyMMdd"));

    QString tiedostoon = QFileDialog::getSaveFileName(this, tr("Varmuuskopioi kirjanpito"), polku, tr("Kirjanpito (*.kitsas)") );
    if( tiedostoon == tiedosto)
    {
        QMessageBox::critical(this, tr("Virhe"), tr("Tiedostoa ei saa kopioida itsensä päälle!"));
        return;
    }
    if( !tiedostoon.isEmpty() )
    {
        QFile kirjanpito( tiedosto);
        if( kirjanpito.copy(tiedostoon) )
            QMessageBox::information(this, kp()->asetukset()->asetus("Nimi"), tr("Kirjanpidon varmuuskopiointi onnistui."));
        else
            QMessageBox::critical(this, tr("Virhe"), tr("Tiedoston varmuuskopiointi epäonnistui."));
    }
}

void AloitusSivu::muistiinpanot()
{
    QDialog dlg(this);
    Ui::Muistiinpanot ui;
    ui.setupUi(&dlg);

    ui.editori->setPlainText( kp()->asetukset()->asetus("Muistiinpanot") );
    if( dlg.exec() == QDialog::Accepted )
        kp()->asetukset()->aseta("Muistiinpanot", ui.editori->toPlainText());

    siirrySivulle();
}

void AloitusSivu::poistaListalta()
{

    if( QMessageBox::question(this, tr("Poista kirjanpito luettelosta"),
                              tr("Haluatko poistaa tämän kirjanpidon viimeisten kirjanpitojen luettelosta?\n"
                                 "Kirjanpitoa ei poisteta levyltä."),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    kp()->sqlite()->poistaListalta( kp()->sqlite()->tiedostopolku() );
}

void AloitusSivu::pyydaInfo()
{

    if( ! kp()->settings()->contains("Keksi"))
    {
        kp()->settings()->setValue("Keksi", Kirjanpito::satujono(10) );
    }

    QString kysely = QString("http://paivitysinfo.kitupiikki.info/?v=%1&os=%2&u=%3&b=%4&d=%5&k=%6")
            .arg( qApp->applicationVersion() )
            .arg( QSysInfo::prettyProductName())
            .arg( kp()->settings()->value("Keksi").toString() )
            .arg( KITSAS_BUILD )
            .arg( buildDate().toString(Qt::ISODate) )
            .arg( kp()->settings()->value("Tilikartta").toString());

    QNetworkRequest pyynto = QNetworkRequest( QUrl(kysely));
    pyynto.setAttribute( QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy  );
    QNetworkReply *reply = kp()->networkManager()->get( pyynto );
    connect( reply, &QNetworkReply::finished, this, &AloitusSivu::infoSaapui);
}

void AloitusSivu::saldotSaapuu(QVariant *data)
{
    saldot_ = data->toMap();

    QString txt("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/aloitus/aloitus.css\"></head><body>");
    txt.append( paivitysInfo );

    txt.append( vinkit() );

    // Ei tulosteta tyhjiä otsikoita vaan possu jos ei kirjauksia
    if( kp()->asetukset()->onko("EkaTositeKirjattu") )
        txt.append(summat());
    else
        txt.append("<p><img src=qrc:/pic/kitsas150.png></p>");

    ui->selain->setHtml(txt);
}

QDate AloitusSivu::buildDate()
{
    QString koostepaiva(__DATE__);      // Tämä päivittyy aina versio.h:ta muutettaessa
    return QDate::fromString( koostepaiva.mid(4,3) + koostepaiva.left(3) + koostepaiva.mid(6), Qt::RFC2822Date);
}

void AloitusSivu::pilviLogin()
{
    kp()->pilvi()->kirjaudu( ui->emailEdit->text(), ui->salaEdit->text(), ui->muistaCheck->isChecked() );
}

void AloitusSivu::kirjauduttu()
{

    ui->salaEdit->clear();
    ui->pilviPino->setCurrentIndex(LISTA);
    ui->kayttajaLabel->setText( kp()->pilvi()->kayttajaNimi() );

    int pilvia = kp()->pilvi()->omatPilvet();
    int pilvetmax = kp()->pilvi()->pilviMax();

    ui->planLabel->setText(  tr("%1\n%2/%3 kirjanpitoa")
                            .arg( kp()->pilvi()->planname())
                            .arg( pilvia )
                            .arg( pilvetmax ));

    ui->oikeudetButton->setVisible( kp()->pilvi()->oikeudet() == "owner" );
    ui->pilviPoistaButton->setVisible( kp()->pilvi()->oikeudet() == "owner" );

    ui->tilausButton->setText( kp()->pilvi()->plan() ? tr("Tilaukseni") : tr("Tilaa pilvipalvelut!") );
}

void AloitusSivu::loginVirhe()
{
    ui->vaaraSalasana->setVisible(!ui->salaEdit->text().isEmpty());
    ui->salaEdit->clear();
}

void AloitusSivu::validoiLoginTiedot()
{
    ui->loginButton->setEnabled(kelpoEmail_ && ui->salaEdit->text().length() > 4);
    ui->salaEdit->setEnabled(kelpoEmail_);
    ui->muistaCheck->setEnabled(kelpoEmail_ && ui->salaEdit->text().length() > 4);
    ui->salasanaButton->setEnabled(kelpoEmail_);
    ui->rekisteroiButton->setEnabled(!kelpoEmail_);
}

void AloitusSivu::validoiEmail()
{
    kelpoEmail_ = false;
    validoiLoginTiedot();

    QRegularExpression emailRe(R"(^([\w-]*(\.[\w-]+)?)+@(\w+\.\w+)(\.\w+)*$)");
    if( emailRe.match( ui->emailEdit->text()).hasMatch() ) {
        // Tarkistetaan sähköposti ja toimitaan sen mukaan
        QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users/" + ui->emailEdit->text() ));
        request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
        QNetworkReply *reply =  kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &AloitusSivu::emailTarkastettu);

    } else {
     validoiLoginTiedot();
    }
}


void AloitusSivu::emailTarkastettu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    kelpoEmail_ =  reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 ;
    validoiLoginTiedot();
}

void AloitusSivu::rekisteroi()
{
    QVariantMap map;

    map.insert("email", ui->emailEdit->text());
    QNetworkAccessManager *mng = kp()->networkManager();

    QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users") );

    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &AloitusSivu::rekisterointiLahti);
}

void AloitusSivu::rekisterointiLahti()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QMessageBox::critical(this, tr("Rekisteröityminen epäonnistui"),
            tr("Rekisteröinnin lähettäminen palvelimelle epäonnistui "
               "tietoliikennevirheen %1 takia.\n\n"
               "Yritä myöhemmin uudelleen").arg( reply->error() ));
        return;
    }
    if( ui->rekisteroiButton->isEnabled()) {
        QMessageBox::information(this, tr("Rekisteröintiviesti lähetetty"),
                                 tr("Viimeistele rekisteröityminen sähköpostiisi "
                                    "lähetetyn linkin avulla."));
        ui->salaEdit->setEnabled(true);
        ui->rekisteroiButton->setEnabled(false);
    } else
        QMessageBox::information(this, tr("Salasanan palauttaminen"),
                                 tr("Sähköpostiisi on lähetetty linkki, jonka avulla "
                                    "voit vaihtaa salasanan."));
}

void AloitusSivu::pilviLogout()
{
    ui->salaEdit->clear();
    kp()->pilvi()->kirjauduUlos();
    ui->pilviPino->setCurrentIndex(KIRJAUDU);
}

void AloitusSivu::logoMuuttui()
{
    double skaala = (1.0 * kp()->logo().width() ) / kp()->logo().height();
    ui->logoLabel->setPixmap( QPixmap::fromImage( kp()->logo().scaled( qRound( 64 * skaala),64,Qt::KeepAspectRatio) ) );
    ui->logoLabel->show();
}

void AloitusSivu::haeSaldot()
{
    QDate saldopaiva = ui->tilikausiCombo->currentData(TilikausiModel::PaattyyRooli).toDate();
    KpKysely *kysely = kpk("/saldot");
    if( kysely ) {
        kysely->lisaaAttribuutti("pvm", saldopaiva);
        connect( kysely, &KpKysely::vastaus, this, &AloitusSivu::saldotSaapuu);
        kysely->kysy();
    }
}

QString AloitusSivu::vinkit()
{
    QString vinkki;


    if( TilikarttaPaivitys::onkoPaivitettavaa() )
    {
        vinkki.append(tr("<table class=info width=100%><tr><td><h3><a href=ktp:/maaritys/paivita>Päivitä tilikartta</a></h3>Tilikartasta saatavilla uudempi versio %1"
                         "</td></tr></table>").arg( TilikarttaPaivitys::paivitysPvm().toString("dd.MM.yyyy") ) );

    }

    // Ensin tietokannan alkutoimiin
    if( !kp()->asetukset()->onko("EkaTositeKirjattu") )
    {
        vinkki.append("<table class=vinkki width=100%><tr><td>");
        vinkki.append("<h3>Kirjanpidon aloittaminen</h3><ol>");
        vinkki.append("<li>Tarkista <a href=ktp:/maaritys/perus>perusvalinnat, logo ja arvonlisävelvollisuus</a> <a href='ohje:/maaritykset/perusvalinnat'>(Ohje)</a></li>");
        vinkki.append("<li>Tutustu <a href=ktp:/maaritys/tilit>tilikarttaan</a> ja tee tarpeelliset muutokset <a href='ohje:/maaritykset/tilikartta'>(Ohje)</a></li>");
        vinkki.append("<li>Lisää tarvitsemasi <a href=ktp:/maaritys/kohdennukset>kohdennukset</a> <a href='ohje:/maaritykset/kohdennukset'>(Ohje)</a></li>");
        if( kp()->asetukset()->luku("Tilinavaus")==2)
            vinkki.append("<li>Tee <a href=ktp:/maaritys/tilinavaus>tilinavaus</a> <a href='ohje:/maaritykset/tilinavaus'>(Ohje)</a></li>");
        vinkki.append("<li>Voit aloittaa <a href=ktp:/kirjaa>kirjausten tekemisen</a> <a href='ohje:/kirjaus'>(Ohje)</a></li>");
        vinkki.append("</ol></td></tr></table>");

    }
    else if( kp()->asetukset()->luku("Tilinavaus")==2 && kp()->asetukset()->pvm("TilinavausPvm") <= kp()->tilitpaatetty() )
        vinkki.append(tr("<table class=vinkki width=100%><tr><td><h3><a href=ktp:/maaritys/tilinavaus>Tee tilinavaus</a></h3><p>Syötä viimeisimmältä tilinpäätökseltä tilien "
                      "avaavat saldot %1 järjestelmään <a href='ohje:/maaritykset/tilinavaus'>(Ohje)</a></p></td></tr></table>").arg( kp()->asetukset()->pvm("TilinavausPvm").toString("dd.MM.yyyy") ) );

    // Muistutus arvonlisäverolaskelmasta
    if(  kp()->asetukset()->onko("AlvVelvollinen") )
    {
        QDate kausialkaa = kp()->asetukset()->pvm("AlvIlmoitus").addDays(1);
        QDate kausipaattyy = kp()->asetukset()->pvm("AlvIlmoitus").addDays(1).addMonths( kp()->asetukset()->luku("AlvKausi")).addDays(-1);
        QDate erapaiva = AlvSivu::erapaiva(kausipaattyy);

        qlonglong paivaaIlmoitukseen = kp()->paivamaara().daysTo( erapaiva );
        if( paivaaIlmoitukseen < 0)
        {
            vinkki.append( tr("<table class=varoitus width=100%><tr><td width=100%>"
                              "<h3><a href=ktp:/alvilmoitus>Arvonlisäveroilmoitus myöhässä</a></h3>"
                              "Arvonlisäveroilmoitus kaudelta %1 - %2 olisi pitänyt antaa %3 mennessä.</td></tr></table>")
                           .arg(kausialkaa.toString("dd.MM.yyyy")).arg(kausipaattyy.toString("dd.MM.yyyy"))
                           .arg(erapaiva.toString("dd.MM.yyyy")));

        }
        else if( paivaaIlmoitukseen < 30)
        {
            vinkki.append( tr("<table class=vinkki width=100%><tr><td>"
                              "<h3><a href=ktp:/alvilmoitus>Tee arvonlisäverotilitys</a></h3>"
                              "Arvonlisäveroilmoitus kaudelta %1 - %2 on annettava %3 mennessä.</td></tr></table>")
                           .arg(kausialkaa.toString("dd.MM.yyyy")).arg(kausipaattyy.toString("dd.MM.yyyy"))
                           .arg(erapaiva.toString("dd.MM.yyyy")));
        }
    }


    // Uuden tilikauden aloittaminen
    if( kp()->paivamaara().daysTo(kp()->tilikaudet()->kirjanpitoLoppuu()) < 30 )
    {
        vinkki.append(tr("<table class=vinkki width=100%><tr><td>"
                      "<h3><a href=ktp:/uusitilikausi>Aloita uusi tilikausi</a></h3>"
                      "<p>Tilikausi päättyy %1, jonka jälkeiselle ajalle ei voi tehdä kirjauksia ennen kuin uusi tilikausi aloitetaan.</p>"
                      "<p>Voit tehdä kirjauksia myös aiempaan tilikauteen, kunnes se on päätetty</p></td></tr></table>").arg( kp()->tilikaudet()->kirjanpitoLoppuu().toString("dd.MM.yyyy") ));

    }

    // Tilinpäätöksen laatiminen
    for(int i=0; i<kp()->tilikaudet()->rowCount(QModelIndex()); i++)
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla(i);
        if( kausi.paattyy().daysTo(kp()->paivamaara()) > 1 &&
                                   kausi.paattyy().daysTo( kp()->paivamaara()) < 5 * 30
                && ( kausi.tilinpaatoksenTila() == Tilikausi::ALOITTAMATTA || kausi.tilinpaatoksenTila() == Tilikausi::KESKEN) )
        {
            vinkki.append(QString("<table class=vinkki width=100%><tr><td>"
                          "<h3><a href=ktp:/tilinpaatos>Aika laatia tilinpäätös tilikaudelle %1</a></h3>").arg(kausi.kausivaliTekstina()));

            if( kausi.tilinpaatoksenTila() == Tilikausi::ALOITTAMATTA)
            {
                vinkki.append("<p>Tee loppuun kaikki tilikaudelle kuuluvat kirjaukset ja laadi sen jälkeen <a href=ktp:/tilinpaatos>tilinpäätös</a>.</p>");
            }
            else
            {
                vinkki.append("<p>Viimeiste ja vahvista <a href=ktp:/arkisto>tilinpäätös</a>.</p>");
            }
            vinkki.append("<p>Katso <a href=ohje:/tilikaudet/tilinpaatos>ohjeet</a> tilinpäätöksen laatimisesta</p></td></tr></table>");
        }
    }

    /* Tarkistetaan, että alv-tilit paikallaan
    if( kp()->asetukset()->onko("AlvVelvollinen") &&
        ( !kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).onkoValidi() ||
          !kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).onkoValidi() ||
          !kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).onkoValidi()))
    {
        vinkki.append( tr("<table class=varoitus width=100%><tr><td>"
                          "<h3><a href=ktp:/maaritys/Tilikartta>Tilikartta puutteellinen</a></h3>"
                          "<p>Tilikartassa pitää olla tilit alv-kirjauksille, alv-vähennyksille ja verovelalle.</td></tr></table>") );

    } */

    // Viimeisenä muistiinpanot
    if( kp()->asetukset()->onko("Muistiinpanot") )
    {
        vinkki.append(" <table class=memo width=100%><tr><td><pre>");
        vinkki.append( kp()->asetukset()->asetus("Muistiinpanot"));
        vinkki.append("</pre></td></tr></table");
    }

    return vinkki;
}

QString AloitusSivu::summat()
{
    QString txt;

    Tilikausi tilikausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->tilikausiCombo->currentIndex() );
    txt.append(tr("<p><h2 class=kausi>Tilikausi %1 - %2 </h1>").arg(tilikausi.alkaa().toString("dd.MM.yyyy"))
             .arg(tilikausi.paattyy().toString("dd.MM.yyyy")));


    txt.append("<table width=100%>");

    QMapIterator<QString,QVariant> iter(saldot_);
    while( iter.hasNext()) {
        iter.next();
        int tilinumero = iter.key().toInt();
        double saldo = iter.value().toDouble();
        Tili *tili = kp()->tilit()->tili( tilinumero );
        if( !tili )
            continue;

        txt.append( QString("<tr><td><a href=\"selaa:%1\">%2</a></td><td class=euro>%L3 €</td></tr>")
                    .arg(tilinumero)
                    .arg(tili->nimiNumero())
                    .arg(saldo,0,'f',2) );

    }
    txt.append("</table>");

    return txt;

}


