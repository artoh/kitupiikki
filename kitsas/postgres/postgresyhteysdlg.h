/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef POSTGRESYHTEYSDLG_H
#define POSTGRESYHTEYSDLG_H

#include <QDialog>
#include "postgresyhteys.h"

namespace Ui {
class PostgresYhteysDlg;
}

class PostgresYhteysDlg : public QDialog
{
    Q_OBJECT
public:
    explicit PostgresYhteysDlg(QWidget *parent = nullptr);
    ~PostgresYhteysDlg() override;

    void asetaYhteys(const PostgresYhteys& yhteys);
    PostgresYhteys yhteys() const;

private:
    Ui::PostgresYhteysDlg *ui;
};

#endif // POSTGRESYHTEYSDLG_H
