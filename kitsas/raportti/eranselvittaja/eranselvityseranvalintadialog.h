#ifndef ERANSELVITYSERANVALINTADIALOG_H
#define ERANSELVITYSERANVALINTADIALOG_H

#include "raportti/eranselvittaja/eranselvityseramodel.h"
#include "tools/eranvalintadialog.h"

class EranSelvitysEranValintaDialog : public EranValintaDialog
{
public:
    EranSelvitysEranValintaDialog(EranSelvitysEraModel* model, QWidget* parent = nullptr);

    int valitseEra(int nykyinen);

    // EranValintaDialog interface

private:
    int nykyinen_;
};

#endif // ERANSELVITYSERANVALINTADIALOG_H
