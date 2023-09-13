#ifndef YKSITYISTILIENPAATTAJA_H
#define YKSITYISTILIENPAATTAJA_H

#include <QDialog>
#include "db/tilikausi.h"
#include "raportti/raportinkirjoittaja.h"

namespace Ui {
class YksityistilienPaattaja;
}

class YksityistilienPaattaja : public QDialog
{
    Q_OBJECT

public:
    explicit YksityistilienPaattaja(QWidget *parent = nullptr);
    ~YksityistilienPaattaja();

    void alusta(const Tilikausi& kausi, const QVariantMap& data);

    void accept() override;

signals:
    void tallennettu();

protected:
    RaportinKirjoittaja selvitys();
    void kirjattu();


private:
    Ui::YksityistilienPaattaja *ui;
    QVariantMap data_;
    Tilikausi kausi_;

};

#endif // YKSITYISTILIENPAATTAJA_H
