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
#include <QLocale>
#include <QSplashScreen>
#include <QTextCodec>
#include <QIcon>
#include <QTranslator>
#include <QFontDatabase>
#include <QFont>

#include "db/kirjanpito.h"
#include "sqlite/sqlitemodel.h"

#include "kitupiikkiikkuna.h"
#include "versio.h"
#include "kieli/kielet.h"

#include <QDebug>
#include <QDir>
#include <QStyleFactory>
#include <QSettings>
#include <QDialog>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QCommandLineParser>

#include "ui_tervetuloa.h"
#include "maaritys/ulkoasumaaritys.h"
#include "pilvi/pilvimodel.h"

#include "tools/kitsaslokimodel.h"


void lisaaLinuxinKaynnistysValikkoon()
{
    // Poistetaan vanha, jotta päivittyisi
    QFile::remove( QDir::home().absoluteFilePath(".local/share/applications/Kitsas.desktop") );
    // Kopioidaan kuvake
    QDir::home().mkpath( ".local/share/icons" );
    QFile::copy(":/pic/Possu64.png", QDir::home().absoluteFilePath(".local/share/icons/Kitsas.png"));
    // Lisätään työpöytätiedosto
    QFile desktop( QDir::home().absoluteFilePath(".local/share/applications/Kitsas.desktop") );
    desktop.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QTextStream out(&desktop);
    out.setCodec("UTF-8");

    out << "[Desktop Entry]\nVersion=1.0\nType=Application\nName=Kitsas " << qApp->applicationVersion() << "\n";
    out << "Icon=" << QDir::home().absoluteFilePath(".local/share/icons/Kitsas.png") << "\n";
    out << "Exec=" << qApp->applicationFilePath() << "\n";
    out << "TryExec=" << qApp->applicationFilePath() << "\n";
    out << "GenericName=Kirjanpito\n";
    out << Kirjanpito::tr("Comment=Avoimen lähdekoodin kirjanpitäjä\n");
    out << "Categories=Office;Finance;Qt;\nTerminal=false";
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KitsasLokiModel::alusta();
    Kielet::alustaKielet(":/tr/tulkki.json");

#if defined (Q_OS_WIN) || defined (Q_OS_MACX)
    a.setStyle(QStyleFactory::create("Fusion"));
#else
    // #120 GNOME-ongelmien takia ei käytetä Linuxissa natiiveja dialogeja
    a.setAttribute(Qt::AA_DontUseNativeDialogs);
#endif
    
    a.setApplicationName("Kitsas");
    a.setApplicationVersion(KITSAS_VERSIO);
    a.setOrganizationDomain("kitsas.fi");
    
    a.setOrganizationName("Kitsas oy");
#ifndef Q_OS_MACX
    a.setWindowIcon( QIcon(":/pic/Possu64.png"));
#endif
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);

    QLocale::setDefault(QLocale(QLocale::Finnish, QLocale::Finland));

    // Qt:n vakioiden kääntämiseksi
    // Käytetään ohjelmaan upotettua käännöstiedostoa, jotta varmasti mukana  

    QTranslator translator;
    translator.load("fi.qm",":/tr/");

    a.installTranslator(&translator);

    QCommandLineParser parser;
    parser.addOptions({
                          {"api",
                           "Pilvipalvelun osoite",
                           "url",
                           KITSAS_API},
                          {"log",
                          "Lokitiedosto",
                          "tiedostopolku",
                          QString()}
                      });
    parser.process(a);

    PilviModel::asetaPilviLoginOsoite( parser.value("api") );
    KitsasLokiModel::setLoggingToFile( parser.value("log") );

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
    QFontDatabase::addApplicationFont(":/lasku/code128_XL.ttf");

    // Fonttimääritykset
    UlkoasuMaaritys::oletusfontti__ = a.font();
    QString fonttinimi = kp()->settings()->value("Fontti").toString();
    if( !fonttinimi.isEmpty()) {
        a.setFont( QFont( fonttinimi, kp()->settings()->value("FonttiKoko").toInt()) );
    }

    // Jos versio on muuttunut, näytetään tervetulodialogi    
    if( kp()->settings()->value("ViimeksiVersiolla").toString() != a.applicationVersion()  )
    {
        QDialog tervetuloDlg;
        Ui::TervetuloDlg tervetuloUi;
        tervetuloUi.setupUi(&tervetuloDlg);
        tervetuloUi.versioLabel->setText("Versio " + a.applicationVersion());
        tervetuloUi.esiKuva->setVisible( a.applicationVersion().contains('-'));
        tervetuloUi.esiVaro->setVisible( a.applicationVersion().contains('-'));


#ifndef Q_OS_LINUX
    tervetuloUi.valikkoonCheck->setVisible(false);
#endif        
        if( tervetuloDlg.exec() != QDialog::Accepted )
            return 0;

#ifdef Q_OS_LINUX
        // Ohjelman lisääminen käynnistysvalikkoon Linuxilla
        if( tervetuloUi.valikkoonCheck->isChecked())
            lisaaLinuxinKaynnistysValikkoon();

#endif
        kp()->settings()->setValue("ViimeksiVersiolla", a.applicationVersion());
    }
    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap( QPixmap(":/pic/splash.png"));
    splash->show();


    KitupiikkiIkkuna ikkuna;
    ikkuna.show();

    // Avaa argumenttina olevan tiedostonnimen
    if( !parser.positionalArguments().isEmpty() && QFile(parser.positionalArguments().value(0)).exists())
        kirjanpito.sqlite()->avaaTiedosto( parser.positionalArguments().value(0) );

    splash->finish( &ikkuna );
    delete splash;

    return a.exec();
}
