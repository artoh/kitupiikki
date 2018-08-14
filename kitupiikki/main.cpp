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

#include "db/kirjanpito.h"
#include "kitupiikkiikkuna.h"

#include <QDebug>
#include <QDir>
#include <QStyleFactory>
#include <QSettings>
#include <QDialog>
#include <QFile>
#include <QTextStream>


#include "ui_tervetuloa.h"

#include "arkisto/tararkisto.h"

int main(int argc, char *argv[])
{   
    QApplication a(argc, argv);

    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap( QPixmap(":/pic/splash.png"));
    splash->show();

#if defined (Q_OS_WIN) || defined (Q_OS_MACX)
    a.setStyle(QStyleFactory::create("Fusion"));
#else
    // #120 GNOME-ongelmien takia ei käytetä Linuxissa natiiveja dialogeja
    a.setAttribute(Qt::AA_DontUseNativeDialogs);
#endif

    a.setApplicationName("Kitupiikki");
    a.setApplicationVersion("1.1-beta.1");
    a.setOrganizationDomain("kitupiikki.info");
    a.setOrganizationName("Kitupiikki Kirjanpito");
#ifndef Q_OS_MACX
    a.setWindowIcon( QIcon(":/pic/Possu64.png"));
#endif

    QLocale::setDefault(QLocale(QLocale::Finnish, QLocale::Finland));

    // Qt:n vakioiden kääntämiseksi
    // Käytetään ohjelmaan upotettua käännöstiedostoa, jotta varmasti mukana  
    QTranslator translator;
    translator.load("fi.qm",":/aloitus/");

    a.installTranslator(&translator);

    // Jos versio on muuttunut, näytetään tervetulodialogi
    QSettings settings;
    if( settings.value("ViimeksiVersiolla").toString() != a.applicationVersion()  )
    {
        QDialog tervetuloDlg;
        Ui::TervetuloDlg tervetuloUi;
        tervetuloUi.setupUi(&tervetuloDlg);
        tervetuloUi.versioLabel->setText("Versio " + a.applicationVersion());
        tervetuloUi.esiKuva->setVisible( a.applicationVersion().contains('-'));
        tervetuloUi.esiVaro->setVisible( a.applicationVersion().contains('-'));
        tervetuloUi.paivitysCheck->setChecked( settings.value("NaytaPaivitykset",true).toBool());

#ifndef Q_OS_LINUX
    tervetuloUi.valikkoonCheck->setVisible(false);
#endif
        tervetuloDlg.exec();

#ifdef Q_OS_LINUX
        // Ohjelman lisääminen käynnistysvalikkoon Linuxilla
        if( tervetuloUi.valikkoonCheck->isChecked())
        {
            // Poistetaan vanha, jotta päivittyisi
            QFile::remove( QDir::home().absoluteFilePath(".local/share/applications/Kitupiikki.desktop") );
            // Kopioidaan kuvake
            QDir::home().mkpath( ".local/share/icons" );
            QFile::copy(":/pic/Possu64.png", QDir::home().absoluteFilePath(".local/share/icons/Kitupiikki.png"));
            // Lisätään työpöytätiedosto
            QFile desktop( QDir::home().absoluteFilePath(".local/share/applications/Kitupiikki.desktop") );
            desktop.open(QIODevice::WriteOnly | QIODevice::Truncate);
            QTextStream out(&desktop);
            out.setCodec("UTF-8");
            out << "[Desktop Entry]\nVersion=1.0\nType=Application\nName=Kitupiikki " << a.applicationVersion() << "\n";
            out << "Icon=" << QDir::home().absoluteFilePath(".local/share/icons/Kitupiikki.png") << "\n";
            out << "Exec=" << a.applicationFilePath() << "\n";
            out << "TryExec=" << a.applicationFilePath() << "\n";
            out << "GenericName=Kirjanpito\n";
            out << a.tr("Comment=Avoimen lähdekoodin kirjanpitäjä\n");
            out << "Categories=Office;Finance;Qt;\nTerminal=false";
        }
#endif

        settings.setValue("NaytaPaivitykset", tervetuloUi.paivitysCheck->isChecked());
        settings.setValue("ViimeksiVersiolla", a.applicationVersion());
    }

    Kirjanpito kirjanpito;
    Kirjanpito::asetaInstanssi(&kirjanpito);


    // Viivakoodifontti
    QFontDatabase::addApplicationFont(":/code128_XL.ttf");

    KitupiikkiIkkuna ikkuna;

    ikkuna.show();
    splash->finish( &ikkuna );

    if( argc > 1)
        kirjanpito.avaaTietokanta( argv[1] );

    delete splash;


    return a.exec();
}
