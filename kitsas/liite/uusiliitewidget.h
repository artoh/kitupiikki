#ifndef UUSILIITEWIDGET_H
#define UUSILIITEWIDGET_H

#include <QWidget>


class LiitteetModel;

namespace Ui {
class TositeWg;
}

class UusiLiiteWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UusiLiiteWidget(QWidget *parent = nullptr);
    ~UusiLiiteWidget();

    void setModel(LiitteetModel* model);    
    void naytaPohja(bool naytetaanko);
signals:
    int lataaPohja(int tositeId);

protected:
    void tarkistaLeikepoyta();
    void valitseTiedosto();
    void nollaaPohjat();
    void leikepoydalta();

    Ui::TositeWg *ui;

    LiitteetModel* model_ = nullptr;
    bool naytaPohja_ = true;

};

#endif // UUSILIITEWIDGET_H
