#ifndef TOFFEELOGIN_H
#define TOFFEELOGIN_H

#include <QDialog>


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

private:
    void vaihdaKieli();
    void logged();

    Ui::ToffeeLogin *ui;    
    LoginService* loginService;

};

#endif // TOFFEELOGIN_H
