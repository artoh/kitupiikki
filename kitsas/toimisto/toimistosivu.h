#ifndef TOIMISTOSIVU_H
#define TOIMISTOSIVU_H

#include <kitupiikkisivu.h>

namespace Ui {
    class Toimisto;
}

class GroupTreeModel;
class GroupData;
class BookData;


class ToimistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    ToimistoSivu(QWidget *parent = nullptr);
    ~ToimistoSivu();

    void siirrySivulle() override;        

protected:
    enum { RYHMATAB, KIRJANPITOTAB, KAYTTAJATAB };

    void nodeValittu(const QModelIndex& index);
    void kirjaValittu(const QModelIndex& index);

    void toimistoVaihtui();
    void kirjaVaihtui();

    void lisaaRyhma();
    void lisaaToimisto();

    void uusiKayttajaRyhmaan();

    void uusiKirjanpito();

private:    

    Ui::Toimisto* ui;

    GroupTreeModel* groupTree_;
    GroupData* groupData_;
    BookData* bookData_;

};

#endif // TOIMISTOSIVU_H
