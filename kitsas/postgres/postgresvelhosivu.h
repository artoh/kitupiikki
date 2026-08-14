/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef POSTGRESVELHOSIVU_H
#define POSTGRESVELHOSIVU_H

#include <QWizardPage>
#include "postgresyhteys.h"

class QLineEdit;
class QSpinBox;

class PostgresVelhoSivu : public QWizardPage
{
    Q_OBJECT
public:
    explicit PostgresVelhoSivu(QWidget *parent = nullptr);

    PostgresYhteys yhteys() const;
    bool validatePage() override;

private:
    QLineEdit *hostEdit_;
    QSpinBox *portSpin_;
    QLineEdit *databaseEdit_;
    QLineEdit *usernameEdit_;
    QLineEdit *passwordEdit_;
};

#endif // POSTGRESVELHOSIVU_H
