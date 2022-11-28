#ifndef TOIMISTOSIVU_H
#define TOIMISTOSIVU_H

#include <kitupiikkisivu.h>

#include "groupmember.h"

namespace Ui {
    class Toimisto;
}

class GroupTreeModel;
class GroupData;
class BookData;
class QAction;
class QMenu;
class QActionGroup;

class ToimistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    ToimistoSivu(QWidget *parent = nullptr);
    ~ToimistoSivu();

    void siirrySivulle() override;        

protected:
    enum Lohko { RYHMALOHKO, KAYTTAJALOHKO, KIRJANPITOLOHKO };
    enum { RYHMATAB, KAYTTAJATAB, KIRJANPITO_TIEDOT, KIRJANPITO_SUORAT, KIRJANPITO_RYHMAT, KIRJANPITO_LOKI };
    enum { PAA_KIRJANPIDOT, PAA_JASENET };

    void nodeValittu(const QModelIndex& index);
    void kirjaValittu(const QModelIndex& index);
    void kayttajaValittu(const QModelIndex& index);
    void kirjanKayttajaValittu(const QModelIndex& index);

    void vaihdaLohko(Lohko lohko);
    void paaTabVaihtui(int tab);

    void toimistoVaihtui();
    void kirjaVaihtui();
    void lisaaRyhma();
    void muokkaaRyhma();
    void lisaaToimisto();
    void lisaaOikeus();
    void muokkaaOikeus();
    void poistaOikeus();

    void muokkaaRyhmaOikeuksia();

    void uusiKayttajaRyhmaan();

    void uusiKirjanpito();
    void vaihdaTuote();
    void poistaKirjanpito();
    void siirraKirjanpito();

    void pikavalinnat();
private:    

    Ui::Toimisto* ui;

    QAction* pikavalinnatAktio_;
    QAction* muokkaaRyhmaAktio_;
    QAction* poistaRyhmaAktio_;    
    QAction* siirraKirjaAktio_;
    QAction* poistaKirjaAktio_;
    QAction* tukiKirjautumisAktio_;


    GroupTreeModel* groupTree_;
    GroupData* groupData_;
    BookData* bookData_;

    GroupMember userInfo_;

    QActionGroup* tuoteRyhma_;
    QMenu* tuoteMenu_;
};

#endif // TOIMISTOSIVU_H
