
#ifndef MAKSATUSWIDGET_H
#define MAKSATUSWIDGET_H

#include <QWidget>

namespace Ui {
class Maksatus;
}

class Tosite;
class MaksutModel;

class MaksatusWidget : public QWidget
{
    Q_OBJECT
public:
    MaksatusWidget(Tosite* tosite, QWidget *parent = nullptr);
    ~MaksatusWidget();

    void reload();

signals:

private:
    void uusiMaksu();
    void itemClicked(const QModelIndex& item);

    void rejectPayment();


    Ui::Maksatus* ui;
    MaksutModel* model_;
    Tosite* tosite_;

};

#endif // MAKSATUSWIDGET_H
