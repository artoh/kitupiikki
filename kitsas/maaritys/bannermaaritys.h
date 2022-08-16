#ifndef BANNERMAARITYS_H
#define BANNERMAARITYS_H

#include "maarityswidget.h"

class BannerModel;

namespace Ui {
  class BannerMaaritys;
}

class BannerMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    BannerMaaritys();
    ~BannerMaaritys();

    void uusiBanner();

    bool naytetaankoTallennus() override { return false; }

private:
    Ui::BannerMaaritys* ui;
    BannerModel* model;
};

#endif // BANNERMAARITYS_H
