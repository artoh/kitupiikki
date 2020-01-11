/*
   Copyright (C) 2019 Arto Hyvättinen

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
#ifndef VANHATUONTIDLG_H
#define VANHATUONTIDLG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QMap>

namespace Ui {
class VanhatuontiDlg;
}

class VanhatuontiDlg : public QDialog
{
    Q_OBJECT

public:
    explicit VanhatuontiDlg(QWidget *parent = nullptr);
    ~VanhatuontiDlg();

protected:
    enum { VALITSE, VIRHE, VALINNAT, SIJAINTI, ODOTA, VALMIS};

    void alustaTuonti();
    void tuoTiedostosta();
    void haeTilikartta(const QString& polku);
    void avaaTietokanta(const QString& tiedostonnimi);
    void alustaValinnat();
    void alustaSijainti();
    void valitseHakemisto();
    void tuo();

    void siirraAsetukset();
    void siirraTilikaudet();
    void taydennaTilit();
    void siirraKohdennukset();
    void siirraTuotteet();
    void siirraAsiakkaat();
    void siirraTositteet();
    void siirraLiiteet();
    QVariantList tilikaudet() const;

    int tilimuunto(int tilinumero) const;

private:
    Ui::VanhatuontiDlg *ui;
    QSqlDatabase kpdb_;
    QMap<QString,QString> kitupiikkiAsetukset_;
    QStringList muokatutAsetukset_;
    QVariantMap kitsasAsetukset_;
    QVariantList kitsasTilit_;
    QMap<int,int> tilinMuunto_;
    QMap<int,int> tiliIdlla_;
};

#endif // VANHATUONTIDLG_H
