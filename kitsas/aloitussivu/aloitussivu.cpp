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
#include "sqlite/sqlitemodel.h"

#include "uusikirjanpito/uusivelho.h"
#include "maaritys/tilikarttapaivitys.h"
#include "alv/alvilmoitustenmodel.h"
#include "kitupiikkituonti/vanhatuontidlg.h"
#include "pilvi/pilveensiirto.h"
#include "tilaus/planmodel.h"

#include <QJsonDocument>
#include <QTimer>
#include <QSortFilterProxyModel>

#include "tilaus/tilauswizard.h"
#include "versio.h"

#include "luotunnusdialogi.h"
#include "salasananvaihto.h"

#include "tools/kitsaslokimodel.h"

#include <QSslError>
#include <QClipboard>

AloitusSivu::AloitusSivu(QWidget *parent) :
    KitupiikkiSivu(parent)
{

    ui = new Ui::Aloitus;
    ui->setupUi(this);
    ui->selain->setOpenLinks(false);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &AloitusSivu::uusiTietokanta);
    connect( ui->avaaNappi, &QPushButton::clicked, this, &AloitusSivu::avaaTietokanta);
    connect( ui->kitupiikkituontiNappi, &QPushButton::clicked, this, &AloitusSivu::tuoKitupiikista);
    connect( ui->tietojaNappi, SIGNAL(clicked(bool)), this, SLOT(abouttiarallaa()));
    connect( ui->tilikausiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(haeSaldot()));
    connect(ui->varmistaNappi, &QPushButton::clicked, this, &AloitusSivu::varmuuskopioi);
    connect(ui->muistiinpanotNappi, &QPushButton::clicked, this, &AloitusSivu::muistiinpanot);
    connect(ui->poistaNappi, &QPushButton::clicked, this, &AloitusSivu::poistaListalta);

    connect( ui->tilikausiCombo, &QComboBox::currentTextChanged, this, &AloitusSivu::haeSaldot);    

    connect( ui->selain, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkki(QUrl)));

    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(kirjanpitoVaihtui()));
    connect( kp()->asetukset(), &AsetusModel::asetusMuuttui, this, &AloitusSivu::kirjanpitoVaihtui);

    connect( ui->loginButton, &QPushButton::clicked, this, &AloitusSivu::pilviLogin);
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &AloitusSivu::kirjauduttu);
    connect( kp()->pilvi(), &PilviModel::loginvirhe, this, &AloitusSivu::loginVirhe);
    connect( ui->logoutButton, &QPushButton::clicked, this, &AloitusSivu::pilviLogout );
    connect( ui->rekisteroiButton, &QPushButton::clicked, [] {
        LuoTunnusDialogi dialogi;
        dialogi.exec();
    });
    connect(ui->salasanaButton, &QPushButton::clicked, this, &AloitusSivu::vaihdaUnohtunutSalasana);

    connect( ui->viimeisetView, &QListView::clicked,
             [] (const QModelIndex& index) { kp()->sqlite()->avaaTiedosto( index.data(SQLiteModel::PolkuRooli).toString() );} );

    connect( ui->pilviView, &QListView::clicked,
             [](const QModelIndex& index) { kp()->pilvi()->avaaPilvesta( index.data(PilviModel::IdRooli).toInt() ); } );

    connect( ui->emailEdit, &QLineEdit::textChanged, this, &AloitusSivu::validoiEmail );
    connect( ui->salaEdit, &QLineEdit::textChanged, this, &AloitusSivu::validoiLoginTiedot);

    connect( ui->tilausButton, &QPushButton::clicked,
             [] () { TilausWizard *tilaus = new TilausWizard(); tilaus->nayta(); });

    connect( ui->kopioiPilveenNappi, &QPushButton::clicked, this, &AloitusSivu::siirraPilveen);
    connect( ui->pilviPoistaButton, &QPushButton::clicked, this, &AloitusSivu::poistaPilvesta);

    connect( kp(), &Kirjanpito::logoMuuttui, this, &AloitusSivu::logoMuuttui);

    connect( ui->tukileikeNappi, &QPushButton::clicked, [this] { qApp->clipboard()->setText( this->ui->tukiOhje->toPlainText() ); });
    connect( ui->vaihdaSalasanaButton, &QPushButton::clicked, this, &AloitusSivu::vaihdaSalasanaUuteen);

    connect( ui->bugiNappi, &QPushButton::clicked, [] { KitsasLokiModel::instanssi()->copyAll(); });


    QSortFilterProxyModel* sqliteproxy = new QSortFilterProxyModel(this);
    sqliteproxy->setSourceModel( kp()->sqlite());
    ui->viimeisetView->setModel( sqliteproxy );
    sqliteproxy->setSortRole(Qt::DisplayRole);

    ui->pilviView->setModel( kp()->pilvi() );
    ui->tkpilviTab->setCurrentIndex( kp()->settings()->value("TietokonePilviValilehti").toInt() );
    ui->vaaraSalasana->setVisible(false);
    ui->palvelinvirheLabel->setVisible(false);

    ui->inboxFrame->setVisible(false);
    ui->inboxFrame->installEventFilter(this);
    ui->outboxFrame->setVisible(false);
    ui->outboxFrame->installEventFilter(this);

    if( kp()->settings()->contains("CloudKey")) {
        ui->pilviPino->setCurrentIndex(SISAANTULO);
        qApp->processEvents();
        QTimer::singleShot(250, [](){ kp()->pilvi()->kirjaudu();} );
    }

    pyydaInfo();
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
        connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &AloitusSivu::haeSaldot);
        sivulla = true;
    }

    // Päivitetään aloitussivua
    if( kp()->yhteysModel() )
    {        
        haeSaldot();
        haeInOutBox();
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
    ui->muistiinpanotNappi->setEnabled( kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
}

void AloitusSivu::paivitaSivu()
{
    if( kp()->yhteysModel() )
    {
        QString txt("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/aloitus/aloitus.css\"></head><body>");
        txt.append( paivitysInfo );


        int saldoja = saldot_.count();

        if( saldoja == 0) {
            // Lataaminen on kesken..
            txt.append("<h1>" + tr("Avataan kirjanpitoa...") + "</h1>");
        } else if( saldoja < 3) {
            // Ei vielä tilejä avattu, possu
            txt.append( vinkit() );
            txt.append("<p><img src=qrc:/pic/kitsas150.png></p>");
        } else {
            txt.append( vinkit() );
            txt.append(summat());
        }

        ui->selain->setHtml(txt);
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
    disconnect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &AloitusSivu::haeSaldot);
    sivulla = false;
    return true;
}

void AloitusSivu::kirjanpitoVaihtui()
{
    saldot_.clear();

    bool avoinna = kp()->yhteysModel();

    ui->nimiLabel->setVisible(avoinna && !kp()->asetukset()->onko("LogossaNimi") );
    ui->tilikausiCombo->setVisible(avoinna );
    ui->logoLabel->setVisible( avoinna && !kp()->logo().isNull());
    ui->varmistaNappi->setEnabled(avoinna);
    ui->muistiinpanotNappi->setEnabled(avoinna);
    ui->poistaNappi->setEnabled( avoinna );
    ui->pilviPoistaButton->setVisible( avoinna &&
                                       qobject_cast<PilviModel*>(kp()->yhteysModel()) &&
                                       kp()->pilvi()->onkoOikeutta(PilviModel::OMISTAJA) );

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
    } else {
        ui->inboxFrame->hide();
        ui->outboxFrame->hide();
    }

    if( !kp()->asetus("Tilikartta").isEmpty() )
        kp()->settings()->setValue("Tilikartta", kp()->asetus("Tilikartta"));

    ui->pilviKuva->setVisible( qobject_cast<PilviModel*>( kp()->yhteysModel()  ) );
    ui->kopioiPilveenNappi->setVisible(qobject_cast<SQLiteModel*>(kp()->yhteysModel()));

    tukiInfo();
    haeSaldot();
    paivitaSivu();
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
    UusiVelho velho( parentWidget() );
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

void AloitusSivu::tuoKitupiikista()
{
    VanhatuontiDlg dlg(this);
    dlg.exec();
}

void AloitusSivu::abouttiarallaa()
{
    Ui::AboutDlg aboutUi;
    QDialog aboutDlg;
    aboutUi.setupUi( &aboutDlg);
    connect( aboutUi.aboutQtNappi, &QPushButton::clicked, qApp, &QApplication::aboutQt);

    QString versioteksti = QString("<b>Versio %1</b><br>Käännetty %2")
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
   if( !reply->error()) {

        QVariantList lista = QJsonDocument::fromJson( reply->readAll() ).toVariant().toList();
        paivitysInfo.clear();


        kp()->settings()->setValue("TilastoPaivitetty", QDate::currentDate());

        for( auto item : lista) {
            QVariantMap map = item.toMap();
            paivitysInfo.append(QString("<table width=100% class=%1><tr><td><h3>%2</h3><p>%3</p></td></tr></table>")
                                .arg(map.value("type").toString())
                                .arg(map.value("title").toString())
                                .arg(map.value("info").toString()));
        }

        paivitaSivu();
    }

    reply->deleteLater();
}

void AloitusSivu::varmuuskopioi()
{
    QString tiedosto = kp()->sqlite()->tiedostopolku();

    // Suljetaan tiedostoyhteys, jotta saadaan varmasti varmuuskopioitua kaikki
    kp()->sqlite()->sulje();

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
    // Avataan tiedosto uudestaan
    kp()->sqlite()->avaaTiedosto(tiedosto);
}

void AloitusSivu::muistiinpanot()
{
    QDialog dlg(this);
    Ui::Muistiinpanot ui;
    ui.setupUi(&dlg);

    ui.editori->setPlainText( kp()->asetukset()->asetus("Muistiinpanot") );
    if( dlg.exec() == QDialog::Accepted )
        kp()->asetukset()->aseta("Muistiinpanot", ui.editori->toPlainText());

    paivitaSivu();
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

void AloitusSivu::poistaPilvesta()
{
    if( !kp()->onkoHarjoitus()) {
        QMessageBox::critical(this, tr("Kirjanpidon poistaminen"),
                              tr("Turvallisuussyistä voit poistaa vain harjoittelutilassa olevan kirjanpidon.\n\n"
                                 "Poistaaksesi tämän kirjanpidon sinun pitää ensin asettaa Asetukset/Perusvalinnat-sivulla "
                                 "määritys Harjoituskirjanpito."));
        return;
    }

    if( QMessageBox::question(this, tr("Kirjanpidon poistaminen"),
                              tr("Haluatko todella poistaa tämän kirjanpidon %1 pysyvästi?").arg(kp()->asetus("Nimi")),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    kp()->pilvi()->poistaNykyinenPilvi();
}

void AloitusSivu::pyydaInfo()
{
    QVariantMap tilasto;
    if( kp()->settings()->contains("TilastoPaivitetty")) {
        tilasto.insert("lastasked", kp()->settings()->value("TilastoPaivitetty").toDate());
    }
    tilasto.insert("application", qApp->applicationName());
    tilasto.insert("version", qApp->applicationVersion());
    tilasto.insert("build", KITSAS_BUILD);
    tilasto.insert("os", QSysInfo::prettyProductName());

    // Lista niistä tilikartoista, joita on käytetty viimeisimmän kuukauden aikana
    QStringList kartat;
    kp()->settings()->beginGroup("tilastokartta");
    for(QString kartta : kp()->settings()->allKeys()) {
        QDate kaytetty = kp()->settings()->value(kartta).toDate();
        if( kaytetty > QDate::currentDate().addMonths(-1)) {
            kartat.append(kartta);
        }
    }
    kp()->settings()->endGroup();
    tilasto.insert("maps", kartat);

    QByteArray ba = QJsonDocument::fromVariant(tilasto).toJson();
    QString osoite = kp()->pilvi()->pilviLoginOsoite() + "/updateinfo";

    QNetworkRequest pyynto = QNetworkRequest( QUrl(osoite));
    pyynto.setRawHeader("Content-type","application/json");
    QNetworkReply *reply = kp()->networkManager()->post(pyynto, ba);
    connect( reply, &QNetworkReply::finished, this, &AloitusSivu::infoSaapui);
}

void AloitusSivu::saldotSaapuu(QVariant *data)
{
    saldot_ = data->toMap();
    paivitaSivu();
}

void AloitusSivu::inboxSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    int lkm = lista.count();

    ui->inboxFrame->setVisible(lkm);
    ui->inboxCount->setText(QString::number(lkm));
}

void AloitusSivu::outboxSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    int lkm = lista.count();

    ui->outboxFrame->setVisible(lkm);
    ui->outboxCount->setText(QString::number(lkm));
}

QDate AloitusSivu::buildDate()
{
    QString koostepaiva(__DATE__);      // Tämä päivittyy aina versio.h:ta muutettaessa
    return QDate::fromString( koostepaiva.mid(4,3) + koostepaiva.left(3) + koostepaiva.mid(6), Qt::RFC2822Date);
}

bool AloitusSivu::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && target == ui->inboxFrame) {
        ktpkasky("inbox");
        return true;
    } else if (event->type() == QEvent::MouseButtonPress && target == ui->outboxFrame) {
        ktpkasky("outbox");
        return true;
    }
    return false;
}

void AloitusSivu::pilviLogin()
{
    kp()->pilvi()->kirjaudu( ui->emailEdit->text(), ui->salaEdit->text(), ui->muistaCheck->isChecked() );
}

void AloitusSivu::kirjauduttu()
{    
    ui->salaEdit->clear();

    if( !kp()->pilvi()->kayttajaPilvessa()) {
        ui->pilviPino->setCurrentIndex(KIRJAUDU);
        return;
    }

    ui->pilviPino->setCurrentIndex(LISTA);
    ui->kayttajaLabel->setText( kp()->pilvi()->kayttajaNimi() );

    int pilvia = kp()->pilvi()->omatPilvet();
    int pilvetmax = kp()->pilvi()->pilviMax();

    if( !kp()->pilvi()->tilausvoimassa()) {
        ui->planLabel->setText(tr("Ei voimassaolevaa omaa tilausta.\n"));
    } else if (pilvia == 0 && pilvetmax == 0) {
        ui->planLabel->setText( kp()->pilvi()->planname() );
    } else if( kp()->pilvi()->plan() == PlanModel::TILITOIMISTOPLAN ) {
        // Tilitoimistoille joustava tilausten enimmäismäärä
        ui->planLabel->setText(  tr("%1\n%2 kirjanpitoa")
                            .arg( kp()->pilvi()->planname())
                            .arg( pilvia ));
    } else {
        ui->planLabel->setText(  tr("%1\n%2/%3 kirjanpitoa")
                            .arg( kp()->pilvi()->planname())
                            .arg( pilvia )
                            .arg( pilvetmax ));
    }

    if( kp()->pilvi()->plan() == 0 && kp()->pilvi()->kokeilujakso() >= QDate::currentDate()) {
        ui->kokeiluLabel->show();
        ui->kokeiluLabel->setText(tr("Kokeilujakso %1 saakka").arg(kp()->pilvi()->kokeilujakso().toString("dd.MM.yyyy")));
    } else {
        ui->kokeiluLabel->hide();
    }

    ui->tilausButton->setText( kp()->pilvi()->plan() ? tr("Tilaukseni") : tr("Tee tilaus") );

    tukiInfo();
}

void AloitusSivu::loginVirhe()
{
    ui->pilviPino->setCurrentIndex(KIRJAUDU);
    ui->vaaraSalasana->setVisible(!ui->salaEdit->text().isEmpty());
    ui->salaEdit->clear();
}

void AloitusSivu::validoiLoginTiedot()
{
    ui->loginButton->setEnabled(kelpoEmail_ && ui->salaEdit->text().length() > 4);
    ui->salaEdit->setEnabled(kelpoEmail_);
    ui->muistaCheck->setEnabled(kelpoEmail_ && ui->salaEdit->text().length() > 4);
    ui->salasanaButton->setEnabled(kelpoEmail_);    
}

void AloitusSivu::validoiEmail()
{
    kelpoEmail_ = false;
    validoiLoginTiedot();
    ui->palvelinvirheLabel->hide();

    QRegularExpression emailRe(R"(^[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+[.][A-Za-z]{2,64}$)");
    if( emailRe.match( ui->emailEdit->text()).hasMatch() ) {
        // Tarkistetaan sähköposti ja toimitaan sen mukaan
        QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users/" + ui->emailEdit->text() ));
        request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
        QNetworkReply *reply =  kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &AloitusSivu::emailTarkastettu);
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            [this](QNetworkReply::NetworkError code){ this->verkkovirhe(code); });
        connect( reply, &QNetworkReply::sslErrors, [] (const QList<QSslError>& errors) { for(auto virhe : errors) qDebug() << virhe.errorString();  });

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

void AloitusSivu::verkkovirhe(QNetworkReply::NetworkError virhe)
{
    if( virhe == QNetworkReply::ConnectionRefusedError)
        ui->palvelinvirheLabel->setText(tr("<p><b>Palvelin ei juuri nyt ole käytettävissä. Yritä myöhemmin uudelleen.</b>"));
    else if( virhe == QNetworkReply::TemporaryNetworkFailureError || virhe == QNetworkReply::NetworkSessionFailedError)
        ui->palvelinvirheLabel->setText(tr("<p><b>Verkkoon ei saada yhteyttä</b>"));
    else if(virhe < QNetworkReply::ContentAccessDenied )
        ui->palvelinvirheLabel->setText(tr("<p><b>Palvelinyhteydessä on virhe (%1)</b>").arg(virhe));
    else if( virhe == QNetworkReply::UnknownServerError)
        ui->palvelinvirheLabel->setText(tr("<p><b>Palvelu on tilapäisesti poissa käytöstä.</b>"));
    if( virhe < QNetworkReply::ContentAccessDenied || virhe == QNetworkReply::UnknownServerError) {

        ui->palvelinvirheLabel->show();
        QTimer::singleShot(30000, this, &AloitusSivu::validoiEmail);
    }
}

void AloitusSivu::vaihdaUnohtunutSalasana()
{
    QVariantMap map;

    map.insert("email", ui->emailEdit->text());
    QNetworkAccessManager *mng = kp()->networkManager();

    QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users") );

    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &AloitusSivu::salasananVaihtoLahti);
}

void AloitusSivu::salasananVaihtoLahti()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QMessageBox::critical(this, tr("Salasanan vaihtaminen epäonnistui"),
            tr("Salasanan vaihtopyynnön lähettäminen palvelimelle epäonnistui "
               "tietoliikennevirheen %1 takia.\n\n"
               "Yritä myöhemmin uudelleen").arg( reply->error() ));
        return;
    }
    QMessageBox::information(this, tr("Salasanan palauttaminen"),
                 tr("Sähköpostiisi on lähetetty linkki, jonka avulla "
                    "voit vaihtaa salasanan."));
}

void AloitusSivu::pilviLogout()
{
    ui->salaEdit->clear();
    kp()->pilvi()->kirjauduUlos();
    ui->vaaraSalasana->hide();
    ui->pilviPino->setCurrentIndex(KIRJAUDU);
}

void AloitusSivu::logoMuuttui()
{
    if( kp()->logo().isNull() )
        ui->logoLabel->hide();
    else {
        double skaala = (1.0 * kp()->logo().width() ) / kp()->logo().height();
        ui->logoLabel->setPixmap( QPixmap::fromImage( kp()->logo().scaled( qRound( 64 * skaala),64,Qt::KeepAspectRatio) ) );
        ui->logoLabel->show();
    }
}

void AloitusSivu::haeSaldot()
{
    if(sivulla) {
        QDate saldopaiva = ui->tilikausiCombo->currentData(TilikausiModel::PaattyyRooli).toDate();
        KpKysely *kysely = kpk("/saldot");
        if( kysely && saldopaiva.isValid()) {
            kysely->lisaaAttribuutti("pvm", saldopaiva);
            connect( kysely, &KpKysely::vastaus, this, &AloitusSivu::saldotSaapuu);
            kysely->kysy();
        }
    }
}

void AloitusSivu::haeInOutBox()
{
    if( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_SELAUS)) {
        ui->inboxFrame->hide();
        ui->outboxFrame->hide();
        return;
    }

    KpKysely* kysely = kpk("/myyntilaskut");
    if( kysely ) {
        kysely->lisaaAttribuutti("lahetettava");
        connect( kysely, &KpKysely::vastaus, this, &AloitusSivu::outboxSaapuu);
        kysely->kysy();
    }


    if( qobject_cast<PilviModel*>( kp()->yhteysModel()  ) ) {
        kysely = kpk("/kierrot/tyolista");
    } else {
        kysely = kpk("/tositteet");
        if( kysely ) {
            kysely->lisaaAttribuutti("saapuneet");
        }
    }

    if( kysely ) {
        connect( kysely, &KpKysely::vastaus, this, &AloitusSivu::inboxSaapuu);
        kysely->kysy();
    }
}

void AloitusSivu::siirraPilveen()
{
    PilveenSiirto *siirtoDlg = new PilveenSiirto();
    siirtoDlg->exec();
}

void AloitusSivu::tukiInfo()
{
    bool tilaaja = kp()->pilvi()->tilausvoimassa();
    ui->maksutonTukiLabel->setVisible( !tilaaja );
    ui->maksuTukiLabel->setVisible( tilaaja);
    ui->tukiOhje->setVisible( tilaaja );
    ui->tukileikeNappi->setVisible( tilaaja);

    if( tilaaja) {
        ui->tukiOhje->setPlainText( QString("version: %1 \n"
                               "os: %2 \n"
                               "user: %3 \n"
                               "plan: %4 \n"
                               "trial: %10 \n"
                               "db: %5 \n"
                               "map: %6 \n"
                               "mapdate: %7 \n"
                               "type: %8 \n"
                               "large: %9 \n"
                               "%11 \n"
                       ).arg(qApp->applicationVersion())
                        .arg(QSysInfo::prettyProductName())
                        .arg(kp()->pilvi()->kayttajaEmail())
                        .arg(kp()->pilvi()->planname())
                        .arg(kp()->kirjanpitoPolku())
                        .arg(kp()->asetus("Tilikartta"))
                        .arg(kp()->asetus("TilikarttaPvm"))
                        .arg(kp()->asetus("muoto"))
                        .arg(kp()->asetus("laajuus"))
                        .arg(kp()->pilvi()->kokeilujakso().toString("dd.MM.yyyy"))
                        #ifdef KITSAS_PORTABLE
                            .arg("portable")
                         #else
                            .arg("")
                        #endif

        );
        KpKysely* kysely = kpk("/info");
        if(kysely) {
            connect(kysely, &KpKysely::vastaus, this, &AloitusSivu::lisaTukiInfo);
            kysely->kysy();
        }
    }
}

void AloitusSivu::lisaTukiInfo(QVariant *data)
{
    QVariantMap map = data->toMap();
    for(QString& avain : map.keys()) {
        ui->tukiOhje->append(avain + ": " + map.value(avain).toString());
    }
}

void AloitusSivu::vaihdaSalasanaUuteen()
{
    Salasananvaihto dlg(this);
    dlg.exec();
}

QString AloitusSivu::vinkit()
{
    QString vinkki;

    // Mahdollinen varmuuskopio
    SQLiteModel *sqlite = qobject_cast<SQLiteModel*>(kp()->yhteysModel());
    QRegularExpression reku("(\\d{2})(\\d{2})(\\d{2}).kitsas");
    if( sqlite && sqlite->tiedostopolku().contains(reku) ) {
        QRegularExpressionMatch matsi = reku.match(sqlite->tiedostopolku());
        vinkki.append( tr("<table class=varoitus width=100%><tr><td width=100%>"
                          "<h3>Varmuuskopio käytössä?</h3>"
                          "Tämä tiedosto on todennäköisesti kirjanpitosi varmuuskopio päivämäärällä <b>%1.%2.20%3.</b> <br>"
                          "Tähän tiedostoon tehdyt muutokset eivät tallennu varsinaiseen kirjanpitoosi."
                          "</td></tr></table>")
                       .arg(matsi.captured(3)).arg(matsi.captured(2)).arg(matsi.captured(1)));
    }

    if( qobject_cast<PilviModel*>(kp()->yhteysModel()) && !kp()->pilvi()->pilviVat() && kp()->asetukset()->onko(AsetusModel::ALV)) {
        vinkki.append( tr("<table class=varoitus width=100%><tr><td width=100%>"
                          "<h3>Tilaus on tarkoitettu arvonlisäverottomaan toimintaan.</h3>"
                          "Pilvikirjanpidon omistajalla on tilaus, jota ei ole tarkoitettu arvonlisäverolliseen toimintaan. "
                          "Arvonlisäilmoitukseen liittyviä toimintoja ei siksi ole käytössä tälle kirjanpidolle. "
                          "</td></tr></table>"));
    }


    if( TilikarttaPaivitys::onkoPaivitettavaa() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) )
    {
        vinkki.append(tr("<table class=info width=100%><tr><td><h3><a href=ktp:/maaritys/paivita>Päivitä tilikartta</a></h3>Tilikartasta saatavilla uudempi versio %1"
                         "</td></tr></table>").arg( TilikarttaPaivitys::paivitysPvm().toString("dd.MM.yyyy") ) );

    }

    // Ensin tietokannan alkutoimiin
    if( saldot_.count() < 3)
    {
        vinkki.append("<table class=vinkki width=100%><tr><td>");
        vinkki.append("<h3>Kirjanpidon aloittaminen</h3><ol>");
        vinkki.append("<li>Tarkista <a href=ktp:/maaritys/perus>perusvalinnat, logo ja arvonlisäverovelvollisuus</a> <a href='ohje:/maaritykset/perusvalinnat'>(Ohje)</a></li>");
        vinkki.append("<li>Tutustu <a href=ktp:/maaritys/tilit>tilikarttaan</a> ja tee tarpeelliset muutokset <a href='ohje:/maaritykset/tilikartta'>(Ohje)</a></li>");
        vinkki.append("<li>Lisää tarvitsemasi <a href=ktp:/maaritys/kohdennukset>kohdennukset</a> <a href='ohje:/maaritykset/kohdennukset'>(Ohje)</a></li>");
        if( kp()->asetukset()->luku("Tilinavaus")==2)
            vinkki.append("<li>Tee <a href=ktp:/maaritys/tilinavaus>tilinavaus</a> <a href='ohje:/maaritykset/tilinavaus'>(Ohje)</a></li>");
        vinkki.append("<li>Voit aloittaa <a href=ktp:/kirjaa>kirjausten tekemisen</a> <a href='ohje:/kirjaus'>(Ohje)</a></li>");
        vinkki.append("</ol></td></tr></table>");

    }
    else if( kp()->asetukset()->luku("Tilinavaus")==2 && kp()->asetukset()->pvm("TilinavausPvm") <= kp()->tilitpaatetty() &&
             kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET))
        vinkki.append(tr("<table class=vinkki width=100%><tr><td><h3><a href=ktp:/maaritys/tilinavaus>Tee tilinavaus</a></h3><p>Syötä viimeisimmältä tilinpäätökseltä tilien "
                      "avaavat saldot %1 järjestelmään <a href='ohje:/maaritykset/tilinavaus'>(Ohje)</a></p></td></tr></table>").arg( kp()->asetukset()->pvm("TilinavausPvm").toString("dd.MM.yyyy") ) );

    // Muistutus arvonlisäverolaskelmasta
    if(  kp()->asetukset()->onko("AlvVelvollinen") && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ALV_ILMOITUS) )
    {
        QDate kausialkaa = kp()->alvIlmoitukset()->viimeinenIlmoitus().addDays(1);
        QDate laskennallinenalkupaiva = kausialkaa;
        int kausi = kp()->asetukset()->luku("AlvKausi");
        if( kausi == 1)
            laskennallinenalkupaiva = QDate( kausialkaa.year(), kausialkaa.month(), 1);
        else if( kausi == 3) {
            int kk = kausialkaa.month();
            if( kk < 4)
                kk = 1;
            else if( kk < 7)
                kk = 4;
            else if( kk < 10)
                kk = 7;
            kk = 10;
            laskennallinenalkupaiva = QDate( kausialkaa.year(), kk, 1);
        } else if( kausi == 12)
            laskennallinenalkupaiva = QDate( kausialkaa.year(), 1, 1);

        QDate kausipaattyy = laskennallinenalkupaiva.addMonths( kausi ).addDays(-1);
        QDate erapaiva = AlvIlmoitustenModel::erapaiva(kausipaattyy);

        qlonglong paivaaIlmoitukseen = kp()->paivamaara().daysTo( erapaiva );
        if( paivaaIlmoitukseen < 0)
        {
            vinkki.append( tr("<table class=varoitus width=100%><tr><td width=100%>"
                              "<h3><a href=ktp:/alvilmoitus>Arvonlisäveroilmoitus myöhässä</a></h3>"
                              "Arvonlisäveroilmoitus kaudelta %1 - %2 olisi pitänyt antaa %3 mennessä.</td></tr></table>")
                           .arg(kausialkaa.toString("dd.MM.yyyy")).arg(kausipaattyy.toString("dd.MM.yyyy"))
                           .arg(erapaiva.toString("dd.MM.yyyy")));

        }
        else if( paivaaIlmoitukseen < 12)
        {
            vinkki.append( tr("<table class=vinkki width=100%><tr><td>"
                              "<h3><a href=ktp:/alvilmoitus>Tee arvonlisäverotilitys</a></h3>"
                              "Arvonlisäveroilmoitus kaudelta %1 - %2 on annettava %3 mennessä.</td></tr></table>")
                           .arg(kausialkaa.toString("dd.MM.yyyy")).arg(kausipaattyy.toString("dd.MM.yyyy"))
                           .arg(erapaiva.toString("dd.MM.yyyy")));
        }
    }


    // Uuden tilikauden aloittaminen
    if( kp()->paivamaara().daysTo(kp()->tilikaudet()->kirjanpitoLoppuu()) < 30 && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET))
    {
        vinkki.append(tr("<table class=vinkki width=100%><tr><td>"
                      "<h3><a href=ktp:/uusitilikausi>Aloita uusi tilikausi</a></h3>"
                      "<p>Tilikausi päättyy %1, jonka jälkeiselle ajalle ei voi tehdä kirjauksia ennen kuin uusi tilikausi aloitetaan.</p>"
                      "<p>Voit tehdä kirjauksia myös aiempaan tilikauteen, kunnes se on päätetty</p></td></tr></table>").arg( kp()->tilikaudet()->kirjanpitoLoppuu().toString("dd.MM.yyyy") ));

    }

    // Tilinpäätöksen laatiminen
    if( kp()->yhteysModel()->onkoOikeutta(YhteysModel::TILINPAATOS)) {
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
                vinkki.append("<p>Katso <a href=\"ohje:/tilinpaatos/aloittaminen/\">ohjeet</a> tilinpäätöksen laatimisesta</p></td></tr></table>");
            }
        }
    }


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


    txt.append("<table width=100% class=saldot>");

    int rivi=0;

    QMapIterator<QString,QVariant> iter(saldot_);
    while( iter.hasNext()) {
        iter.next();
        int tilinumero = iter.key().toInt();
        double saldo = iter.value().toDouble();

        txt.append( QString("<tr class=%4><td><a href=\"selaa:%1\">%1 %2</a></td><td class=euro>%L3 €</td></tr>")
                    .arg(tilinumero)
                    .arg(kp()->tilit()->nimi(tilinumero).toHtmlEscaped())
                    .arg(saldo,0,'f',2)
                    .arg(rivi++ % 4 == 3 ? "tumma" : "vaalea"));

    }
    txt.append("</table>");

    return txt;

}


