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

#include "uusikirjanpito.h"
#include "tilikausisivu.h"
#include <QDate>

TilikausiSivu::TilikausiSivu()
{
    setTitle("Tilikauden tiedot");

    ui = new Ui::TilikausiSivu;
    ui->setupUi(this);

    int tamavuosi = QDate::currentDate().year();

    ui->ekaAlkaa->setDate(QDate(tamavuosi,1,1));
    ui->ekaPaattyy->setDate(QDate(tamavuosi,12,31));

    ui->edellinenAlkoi->setDate(QDate(tamavuosi-1,1,1));
    ui->edellinenPaattyi->setDate(QDate(tamavuosi-1,12,31));

    connect( ui->ekaAlkaa, SIGNAL(dateChanged(QDate)),
             this, SLOT(alkuPaivaMuuttui(QDate)));

    connect( ui->ekaPaattyy, &QDateEdit::dateChanged, this, &TilikausiSivu::loppuPaivaMuuttui);

    registerField("alkaa", ui->ekaAlkaa);
    registerField("paattyy", ui->ekaPaattyy);
    registerField("onekakausi",ui->aloittavaTilikausiCheck);
    registerField("edalkoi", ui->edellinenAlkoi);
    registerField("edpaattyi", ui->edellinenPaattyi);
}

TilikausiSivu::~TilikausiSivu()
{
    delete ui;
}

int TilikausiSivu::nextId() const
{
    // Jos käytössä ei ole kirjausperusteskriptejä, hypätään
    // suoraan sijaintisivulle

    QString polku = field("tilikartta").toString();
    QMap<QString,QStringList> ktk = UusiKirjanpito::lueKtkTiedosto(polku);

    if( ktk.keys().contains("Kirjaamisperuste/Maksuperuste"))
        return UusiKirjanpito::KIRJAUSPERUSTESIVU;
    else
        return UusiKirjanpito::SIJAINTISIVU;
}

bool TilikausiSivu::isComplete() const
{
    return( ui->ekaAlkaa->date() < ui->ekaPaattyy->date() && ui->ekaPaattyy->date() < ui->ekaAlkaa->date().addMonths(18)  );
}

void TilikausiSivu::alkuPaivaMuuttui(const QDate &date)
{
    ui->ekaPaattyy->setDate( QDate( date.year(), 12, 31)  );

    ui->edellinenPaattyi->setDate( date.addDays(-1));
    ui->edellinenAlkoi->setDate( date.addYears(-1));

    ui->edellinenAlkoi->setMaximumDate(date.addDays(-2));
    ui->edellinenAlkoi->setMinimumDate(date.addMonths(-18));

    loppuPaivaMuuttui();
}

void TilikausiSivu::loppuPaivaMuuttui()
{

    if( ui->ekaAlkaa->date() < ui->ekaPaattyy->date() && ui->ekaPaattyy->date() < ui->ekaAlkaa->date().addMonths(18)  )
        ui->ekaPaattyy->setStyleSheet("color: black;");
    else
        ui->ekaPaattyy->setStyleSheet("color: red;");

    emit completeChanged();
}
