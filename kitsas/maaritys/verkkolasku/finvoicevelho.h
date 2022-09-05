#ifndef FINVOICEVELHO_H
#define FINVOICEVELHO_H

#include <QWizard>
#include <QObject>

namespace Ui {
    class FinvoiceVelhoAlku;
    class FinvoiceVelhoTiedot;
    class FinvoiceVelhoEmail;
    class FinvoiceVelhoValmis;
}

class FinvoiceVelho : public QWizard
{
    Q_OBJECT
public:
    FinvoiceVelho(QWidget *parent = nullptr);
    enum Sivut {ALOITUS, TIEDOT, EMAIL, VALMIS};

    void kitsasKaytossa(bool onko);


protected:
    class Alkusivu : public QWizardPage {
    public:
        Alkusivu();
        ~Alkusivu();

        void initializePage() override;
        bool validatePage() override;
        void kitsasKaytossa(bool onko);


    protected:
        Ui::FinvoiceVelhoAlku *ui;
        bool kitsasKaytossa_ = false;
    };

    class TiedotSivu: public QWizardPage {
    public:
        TiedotSivu();
        ~TiedotSivu();

    protected:
        Ui::FinvoiceVelhoTiedot *ui;
    };

    class EmailSivu : public QWizardPage {
    public:
        EmailSivu();
        ~EmailSivu();
    protected:
        Ui::FinvoiceVelhoEmail *ui;
    };

    class ValmisSivu : public QWizardPage {
    public:
        ValmisSivu();
        ~ValmisSivu();
    protected:
        Ui::FinvoiceVelhoValmis *ui;
    };

    Alkusivu* alkusivu;
};

#endif // FINVOICEVELHO_H
