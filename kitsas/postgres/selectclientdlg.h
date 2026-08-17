/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef SELECTCLIENTDLG_H
#define SELECTCLIENTDLG_H

#include <QDialog>
#include "postgresyhteys.h"

namespace Ui {
class SelectClientDlg;
}

class SelectClientDlg : public QDialog
{
    Q_OBJECT
public:
    explicit SelectClientDlg(const PostgresYhteys& palvelin, QWidget *parent = nullptr);
    ~SelectClientDlg() override;

    bool joAvattu() const { return joAvattu_; }

private slots:
    void paivitaLista();
    void valintaMuuttui();
    void avaaValittu();
    void uusiAsiakas();

private:
    PostgresYhteys valittuYhteys() const;

    Ui::SelectClientDlg *ui;
    PostgresYhteys palvelin_;
    bool joAvattu_ = false;
};

#endif // SELECTCLIENTDLG_H
