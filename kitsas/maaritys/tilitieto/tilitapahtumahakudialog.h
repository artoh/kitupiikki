#ifndef TILITIETO_TILITAPAHTUMAHAKUDIALOG_H
#define TILITIETO_TILITAPAHTUMAHAKUDIALOG_H

#include <QDialog>

namespace Ui {
    class TiliTapahtumaHakuDialog;
}

namespace Tilitieto {
class TilitietoPalvelu;


class TiliTapahtumaHakuDialog : public QDialog
{
    Q_OBJECT
public:
    TiliTapahtumaHakuDialog(TilitietoPalvelu* palvelu, QWidget* parent = nullptr);
    ~TiliTapahtumaHakuDialog();

    void accept() override;
    void nayta(int yhteysIndeksi);

protected:
    void tarkastaPvmVali();

    Ui::TiliTapahtumaHakuDialog *ui;
    TilitietoPalvelu* palvelu_;
};

} // namespace Tilitieto

#endif // TILITIETO_TILITAPAHTUMAHAKUDIALOG_H
