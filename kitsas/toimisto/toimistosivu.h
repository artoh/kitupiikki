#ifndef TOIMISTOSIVU_H
#define TOIMISTOSIVU_H

#include <kitupiikkisivu.h>

namespace Ui {
    class Toimisto;
}


class ToimistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    ToimistoSivu(QWidget *parent = nullptr);
    ~ToimistoSivu();

private:
    Ui::Toimisto* ui;

};

#endif // TOIMISTOSIVU_H
