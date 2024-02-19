#ifndef EUMYYNTIYHTEENVETODIALOGI_H
#define EUMYYNTIYHTEENVETODIALOGI_H

#include <QDialog>
#include "raportti/raportinkirjoittaja.h"



namespace Ui {
class AlvIlmoitusDialog;
}

class Tosite;

class EuMyyntiYhteenvetoDialogi : public QDialog
{
    Q_OBJECT
public:
    EuMyyntiYhteenvetoDialogi(QWidget* parent = nullptr);
    ~EuMyyntiYhteenvetoDialogi();

    void naytaIlmoitus(const QVariantMap& data);

signals:
    void tallennettu();

protected:
    void alusta();
    void tarkastaKelpo();

    void accept() override;

    void ilmoita();
    void ilmoitettu(QVariant* data);
    void ilmoitusVirhe(const QString &koodi, const QString& viesti);

    void tallennaTosite();
    void valmis();

private:
    QDate loppupvm_;
    Ui::AlvIlmoitusDialog *ui;
    RaportinKirjoittaja rk;

    QVariantMap data_;

    Tosite* tosite_;

};

#endif // EUMYYNTIYHTEENVETODIALOGI_H
