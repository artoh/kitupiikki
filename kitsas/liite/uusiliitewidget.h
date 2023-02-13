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

signals:

protected:
    void valitseTiedosto();

    Ui::TositeWg *ui;

    LiitteetModel* model_ = nullptr;

};

#endif // UUSILIITEWIDGET_H
