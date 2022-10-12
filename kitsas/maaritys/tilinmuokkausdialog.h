/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#ifndef TILINMUOKKAUSDIALOG_H
#define TILINMUOKKAUSDIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>

#include "ui_tilinmuokkaus.h"
#include "db/tilimodel.h"

/**
 * @brief Yhtä tiliä muokkaava dialogi
 */
class TilinMuokkausDialog : public QDialog
{
    Q_OBJECT
public:    
    /**
     * @brief Dlg tilin muokkaamiseen
     * @param indeksi jos uusi, otsikon indeksi: muuten muokattavan indeksi
     * @param uusi
     */
    enum Tila {
        MUOKKAA,
        UUSIOTSIKKO,
        UUSITILI
    };

    enum {
        PERUS,
        ERITTELY,
        POISTO,
        ALV,
        OHJE
    };

    TilinMuokkausDialog(QWidget* parent, int indeksi, Tila tila);

    ~TilinMuokkausDialog();

protected:
    void lataa();

protected slots:
    void veroEnablePaivita();
    /**
     * @brief Näyttää tälle tilityypille oikeat valinnat
     */
    void naytettavienPaivitys();



    /**
     * @brief Tarkistaa, ettei numero vain ole jo käytössä
     * @param nroTekstina
     */
    void tarkasta();

    void ibanCheck();
    void numeroCheck();
    void accept();
    void poista();

    void viennitSaapuu(QVariant* data);

    bool tyyppiVaroitus();

protected:
    void alustaOhjeet();
    void alustalaajuus();


protected:

    Ui::tilinmuokkausDialog *ui;
    TiliModel *model_;
    QModelIndex index_;
    QSortFilterProxyModel *proxy_;
    QSortFilterProxyModel *veroproxy_;

    QPushButton *poistaNappi_=nullptr;

    Tila tila_;
    Tili* vanhempi_ = nullptr;
    Tili* tili_ = nullptr;
    QString minNumero_;
    QString maxnumero_;
    int indeksi_ = -1;
    int taso_ = 0;

    QStringList varoTyypit;

};

#endif // TILINMUOKKAUSDIALOG_H
