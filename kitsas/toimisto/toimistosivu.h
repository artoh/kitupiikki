#ifndef TOIMISTOSIVU_H
#define TOIMISTOSIVU_H

#include <kitupiikkisivu.h>

#include "groupmember.h"
#include "qlistwidget.h"
#include "qsortfilterproxymodel.h"

namespace Ui {
    class Toimisto;
}

class GroupTreeModel;
class GroupData;
class BookData;
class QAction;
class QMenu;
class QActionGroup;
class GroupUserData;

class QWebEngineView;

class ToimistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    ToimistoSivu(QWidget *parent = nullptr);
    ~ToimistoSivu();

    void siirrySivulle() override;        

    QString ohjeSivunNimi() override {return "toimisto";}

protected:
    enum Lohko { RYHMALOHKO, KAYTTAJALOHKO, KIRJANPITOLOHKO };
    enum { RYHMATAB, KAYTTAJATAB, KIRJANPITO_TIEDOT, KIRJANPITO_SUORAT, KIRJANPITO_RYHMAT, KIRJANPITO_LOKI, OIKEUDET_SUORAT, OIKEUDET_RYHMAT, KIRJAT_RYHMAT };
    enum { PAA_KIRJANPIDOT, PAA_JASENET };

    enum { KIRJANPITO, RYHMA, KAYTTAJA};
    enum { TYYPPIROOLI = Qt::UserRole, IDROOLI = Qt::UserRole + 1, RYHMAROOLI = Qt::UserRole+2};

    void nodeValittu(const QModelIndex& index);
    void kirjaValittu(const QModelIndex& index);
    void kayttajaValittu(const QModelIndex& index);
    void kirjanKayttajaValittu(const QModelIndex& index);

    void vaihdaLohko(Lohko lohko);
    void paaTabVaihtui(int tab);

    void toimistoVaihtui(int bookId = 0);
    void kirjaVaihtui();
    void lisaaRyhma();
    void muokkaaRyhma();
    void lisaaToimisto();
    void lisaaOikeus();
    void muokkaaOikeus();
    void poistaOikeus();

    void lisaaVarmenne();
    void poistaVarmenne();

    void muokkaaRyhmaOikeuksia();

    void uusiKayttajaRyhmaan();

    void uusiKirjanpito();
    void vaihdaTuote();
    void poistaKirjanpito();
    void siirraKirjanpito();

    void kayttajaLadattu();

    void pikavalinnat();
    void haku(const QString& teksti);
    void hakuSaapuu(QVariant* data);
    void hakuValittu(QListWidgetItem* item);

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
    GroupUserData* userData_;

    QSortFilterProxyModel *treeSort_;

    GroupMember userInfo_;

    QActionGroup* tuoteRyhma_;
    QMenu* tuoteMenu_;

    QWebEngineView* webView_;
    QWidget* toimistoWidget_;
};

#endif // TOIMISTOSIVU_H
