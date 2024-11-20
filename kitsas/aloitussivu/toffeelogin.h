#ifndef TOFFEELOGIN_H
#define TOFFEELOGIN_H

#include <QDialog>
#include "pilvi/pilvikayttaja.h"

namespace Ui {
class ToffeeLogin;
}

class LoginService;

class ToffeeLogin : public QDialog
{
    Q_OBJECT

public:
    explicit ToffeeLogin(QWidget *parent = nullptr);
    ~ToffeeLogin();

    int exec() override;
    void reject() override;
    int keyExec();

private:
    void vaihdaKieli();
    void logged(PilviKayttaja kayttaja);

    Ui::ToffeeLogin *ui;    
    LoginService* loginService;

    enum { FI, SV, EN };

};

#endif // TOFFEELOGIN_H
