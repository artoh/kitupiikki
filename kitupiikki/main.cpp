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

#include "uusikp/uusikirjanpito.h"
#include "kitupiikkiikkuna.h"

#include <QDebug>
#include <QStyleFactory>
#include <QSettings>
#include <QDialog>

#include "ui_tervetuloa.h"

int main(int argc, char *argv[])
{   
    QApplication a(argc, argv);
    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap( QPixmap(":/pic/splash.png"));
    splash->show();

#ifdef Q_OS_WIN
    QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

    a.setApplicationName("Kitupiikki");
    a.setApplicationVersion("0.2.1-alpha");
    a.setOrganizationDomain("artoh.github.io");
    a.setOrganizationName("Kitupiikki Kirjanpito");
    a.setWindowIcon( QIcon(":/pic/Possu64.png"));


    QLocale::setDefault(QLocale(QLocale::Finnish, QLocale::Finland));

    QSettings settings;
    if( settings.value("ViimeksiVersiolla").toString() != a.applicationVersion())
    {
        QDialog tervetuloDlg;
        Ui::TervetuloDlg tervetuloUi;
        tervetuloUi.setupUi(&tervetuloDlg);
        tervetuloUi.versioLabel->setText("Versio " + a.applicationVersion());
        tervetuloUi.esiKuva->setVisible( a.applicationVersion().contains('-'));
        tervetuloUi.esiVaro->setVisible( a.applicationVersion().contains('-'));
        tervetuloUi.paivitysCheck->setChecked( settings.value("NaytaPaivitykset",true).toBool());
        tervetuloDlg.exec();
        settings.setValue("NaytaPaivitykset", tervetuloUi.paivitysCheck->isChecked());
        settings.setValue("ViimeksiVersiolla", a.applicationVersion());
    }


    KitupiikkiIkkuna ikkuna;
    ikkuna.show();
    splash->finish( &ikkuna );
    delete splash;


    return a.exec();
}
