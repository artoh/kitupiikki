#ifndef BANNERDIALOG_H
#define BANNERDIALOG_H

#include <QDialog>

class BannerModel;


namespace Ui {
class BannerDialog;
}

class BannerDialog : public QDialog
{
    Q_OBJECT

public:
    BannerDialog(QWidget *parent, BannerModel* model);
    ~BannerDialog();

    void muokkaa(const QModelIndex& indeksi);
    void uusi();

    void vaihdaKuva();
    void tarkasta();

    void accept() override;


private:
    Ui::BannerDialog *ui;

    BannerModel* model_;
    QImage kuva_;
    QString uuid_;
};

#endif // BANNERDIALOG_H
