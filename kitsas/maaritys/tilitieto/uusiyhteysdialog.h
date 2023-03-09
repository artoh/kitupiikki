#ifndef TILITIETO_UUSIYHTEYSDIALOG_H
#define TILITIETO_UUSIYHTEYSDIALOG_H

#include <QDialog>


namespace Ui {
   class Uusiyhteys;
}

namespace Tilitieto {

class TilitietoPalvelu;

class UusiYhteysDialog : public QDialog
{
    Q_OBJECT
public:
    UusiYhteysDialog(TilitietoPalvelu* palvelu);
    ~UusiYhteysDialog();

    void lisaaValtuutus();

    void vahvista(const QString& linkki, int pankkiId);


protected:
    enum {
        VALITSEPANKKI = 0,
        LINKKI = 1
    };

    void seuraava();
    void valmis();
    void pankkiValittu();
    void asetaLogo(int pankkiId);


    Ui::Uusiyhteys *ui;
    TilitietoPalvelu* palvelu_;
};

} // namespace Tilitieto

#endif // TILITIETO_UUSIYHTEYSDIALOG_H
