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

#include <QApplication>
#include <QGuiApplication>
#include <QSplashScreen>
#include <QIcon>
#include <QFontDatabase>
#include <QFont>

#include "db/kirjanpito.h"
#include "sqlite/sqlitemodel.h"

#include "kitupiikkiikkuna.h"
#include "versio.h"
#include "kieli/kielet.h"

#include <QDebug>

#include <QStyleFactory>
#include <QSettings>

#include <QFileInfo>
#include <QCommandLineParser>

#include "aloitussivu/tervetulodialogi.h"
#include "maaritys/ulkoasumaaritys.h"
#include "pilvi/pilvimodel.h"

#include "tools/kitsaslokimodel.h"
#include "aloitussivu/toffeelogin.h"

#include "laskutus/laskunuusinta.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationVersion(KITSAS_VERSIO);
    a.setOrganizationDomain("kitsas.fi");
    a.setOrganizationName("Kitsas oy");

    KitsasLokiModel::alusta();  

#if defined (Q_OS_WIN) || defined (Q_OS_MACX)
    a.setStyle(QStyleFactory::create("Fusion"));
#else
    // #120 GNOME-ongelmien takia ei käytetä Linuxissa natiiveja dialogeja
    a.setAttribute(Qt::AA_DontUseNativeDialogs);
#endif
        
    QCommandLineParser parser;
    parser.addOptions({
                          {"api",
                           "Pilvipalvelun osoite",
                           "url",
                           KITSAS_API},
                          {"log",
                          "Lokitiedosto",
                          "tiedostopolku",
                          QString()},
                          {"pro",
                          "Kirjautuminen suoraan pilveen"},
                           {"demo",
                           "Demo-tila"},
                            {"noweb","Käytä aina ulkoista selainta"}
                      });
    parser.addVersionOption();
    parser.process(a);

    a.setApplicationName( parser.isSet("pro") || PRO_VERSIO ? "Kitsas Pro" : "Kitsas");
#ifndef Q_OS_MACX
    a.setWindowIcon( parser.isSet("pro") || PRO_VERSIO ? QIcon(":/pic/propossu-64.png") : QIcon(":/pic/Possu64.png") );
#endif


    Kielet::alustaKielet(":/tr/tulkki.json");

    PilviModel::asetaPilviLoginOsoite( parser.value("api") );
    KitsasLokiModel::setLoggingToFile( parser.value("log") );
    a.setProperty("noweb", parser.isSet("noweb"));

    QStringList argumentit = qApp->arguments();

    // Windowsin asentamattomalla versiolla
    // asetukset kirjoitetaan kitupiikki.ini -tiedostoon

#if defined  (Q_OS_WIN) && defined (KITSAS_PORTABLE)

    QFileInfo info(argumentit.at(0));
    QString polku = info.absoluteDir().absolutePath();

    Kirjanpito kirjanpito(polku);
#else
    Kirjanpito kirjanpito;
#endif

    Kirjanpito::asetaInstanssi(&kirjanpito);

    QFontDatabase::addApplicationFont(":/aloitus/FreeSans.ttf");
    QFontDatabase::addApplicationFont(":/aloitus/FreeMono.ttf");
    QFontDatabase::addApplicationFont(":/lasku/code128_XL.ttf");

    // Fonttimääritykset
    UlkoasuMaaritys::oletusfontti__ = a.font();
    QString fonttinimi = kp()->settings()->value("Fontti").toString();
    if( !fonttinimi.isEmpty()) {
        a.setFont( QFont( fonttinimi, kp()->settings()->value("FonttiKoko").toInt()) );
    }

    if( parser.isSet("pro") ||  PRO_VERSIO ) {
        PilviKayttaja::asetaVersioMoodi(PilviKayttaja::PRO);
        ToffeeLogin loginDlg;
        if(loginDlg.keyExec() != QDialog::Accepted) {
            return 0;
        }        
    } else if( kp()->settings()->value("ViimeksiVersiolla").toString() != a.applicationVersion()  ) {
        TervetuloDialogi tervetuloa;
        if( tervetuloa.exec() != QDialog::Accepted)
            return 0;
        kp()->settings()->setValue("ViimeksiVersiolla", a.applicationVersion());
    }
    a.setProperty("demo", parser.isSet("demo"));

    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap( QPixmap(":/pic/splash_" + Kielet::instanssi()->uiKieli() + ".png"));
    splash->show();

    KitupiikkiIkkuna ikkuna;
    ikkuna.show();

    // Avaa argumenttina olevan tiedostonnimen
    if( !parser.positionalArguments().isEmpty() && QFile(parser.positionalArguments().value(0)).exists())
        kirjanpito.sqlite()->avaaTiedosto( parser.positionalArguments().value(0) );

    new LaskunUusinta(&kirjanpito);

    splash->finish( &ikkuna );
    delete splash;

    return a.exec();
}
