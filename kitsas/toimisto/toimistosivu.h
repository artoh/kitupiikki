#ifndef TOIMISTOSIVU_H
#define TOIMISTOSIVU_H

#include <kitupiikkisivu.h>

namespace Ui {
    class Toimisto;
}

class GroupTreeModel;


class ToimistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    ToimistoSivu(QWidget *parent = nullptr);
    ~ToimistoSivu();

    void siirrySivulle() override;

private:
    Ui::Toimisto* ui;

    GroupTreeModel* groupTree_;

};

#endif // TOIMISTOSIVU_H
