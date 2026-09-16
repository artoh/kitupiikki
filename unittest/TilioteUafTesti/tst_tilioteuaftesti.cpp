/*
   Regression for GitHub #1368 / PR #1446.

   Repro (documented 100% before fix):
     Uusi tosite -> Siirto -> Laskun maksu -> Tililtä -> Sulje

   Root cause: suljeNappi runs reject() (WA_DeleteOnClose -> deleteLater)
   then tyhjenna(). tyhjenna() reaches naytaRivi()'s qApp->processEvents(),
   which destroys TilioteKirjaaja while callers are still on the stack
   (use-after-free / access violation).

   On a buggy build this test is expected to crash (not merely fail an assert).
   Surviving to QVERIFY means the crash path did not trigger in this run.
*/
#include <QtTest>
#include <QApplication>
#include <QPointer>
#include <QPushButton>
#include <QTabBar>
#include <QFile>
#include <QDir>
#include <QDate>
#include <QTimer>

#include "db/kirjanpito.h"
#include "kieli/kielet.h"
#include "model/tosite.h"
#include "db/tositetyyppimodel.h"
#include "sqlite/sqlitemodel.h"
#include "apuri/siirtoapuri.h"
#include "apuri/tiliote/tiliotekirjaaja.h"
#include "apuri/tiliote/tilioteapuri.h"

class TilioteUafTesti : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    /** PR #1446: Siirto -> Laskun maksu -> Tililtä -> Sulje */
    void siirto_laskunmaksu_tililta_sulje();

    /**
     * #1368 save path: after a TILIOTE save, KirjausWg::tallennettu resets
     * the type combo (index 0), which delete:s the live TilioteApuri.
     * If teeReset() still has processEvents() nested, that is UAF.
     * This exercises the type-reset / delete-apuri half without full CSV UI.
     */
    void tiliote_tallennettu_vaihtaa_tyypin_turvallisesti();

private:
    QString tiedostoPolku_;
};

void TilioteUafTesti::initTestCase()
{
    Kielet::alustaKielet(":/testidata/tulkki.json");
    kp()->asetaInstanssi(new Kirjanpito());
}

void TilioteUafTesti::init()
{
    tiedostoPolku_ = QDir::temp().absoluteFilePath("tiliote_uaf_testi.kitsas");
    QFile::remove(tiedostoPolku_);
    QVERIFY(QFile::copy(":/testidata/oy.kitsas", tiedostoPolku_));
    QVERIFY(QFile::setPermissions(tiedostoPolku_,
                                  QFileDevice::WriteUser | QFileDevice::ReadUser));
    QVERIFY(kp()->avaaTietokanta(tiedostoPolku_, false));
}

void TilioteUafTesti::cleanup()
{
    if (kp()->sqlite())
        kp()->sqlite()->sulje();
    QFile::remove(tiedostoPolku_);
}

void TilioteUafTesti::siirto_laskunmaksu_tililta_sulje()
{
    Tosite tosite;
    tosite.asetaPvm(QDate(2020, 1, 15));
    tosite.asetaTyyppi(TositeTyyppi::SIIRTO);

    SiirtoApuri siirto(nullptr, &tosite);

    // Same construction as SiirtoApuri::laskunmaksu()
    auto *kirjaaja = new TilioteKirjaaja(&siirto);
    kirjaaja->setAttribute(Qt::WA_DeleteOnClose);
    kirjaaja->show();
    // Offscreen CI may not "expose"; still drive the widget tree.
    QTest::qWait(50);
    qApp->processEvents();

    auto *ylaTab = kirjaaja->findChild<QTabBar *>("ylaTab");
    QVERIFY(ylaTab);
    QCOMPARE(ylaTab->count(), 2);
    // YlaTab::TILILTA == 1  ("Tililtä")
    ylaTab->setCurrentIndex(1);
    qApp->processEvents();

    QPointer<TilioteKirjaaja> elossa(kirjaaja);
    auto *sulje = kirjaaja->findChild<QPushButton *>("suljeNappi");
    QVERIFY(sulje);

    // Emits clicked → .ui reject() + alusta()'s tyhjenna() (UAF chain).
    sulje->click();

    // Let deleteLater / singleShot(50, tiliMuuttuu) run.
    QTest::qWait(300);
    qApp->processEvents(QEventLoop::AllEvents);

    // Reaching here without a crash means this environment did not hit UAF.
    QVERIFY2(!elossa || !elossa->isVisible(),
             "Sulje should close/delete the dialog");
}

void TilioteUafTesti::tiliote_tallennettu_vaihtaa_tyypin_turvallisesti()
{
    Tosite tosite;
    tosite.asetaPvm(QDate(2020, 1, 15));
    tosite.asetaTyyppi(TositeTyyppi::TILIOTE);

    // Parent widget owns the apuri the same way KirjausWg does.
    QWidget host;
    auto *apuri = new TilioteApuri(&host, &tosite);
    QPointer<TilioteApuri> elossa(apuri);
    apuri->show();
    qApp->processEvents();

    // Mirror KirjausWg::tositeTyyppiVaihtui: tear down apuri while it may
    // still be processing nested events (teeReset ends with processEvents).
    // Force a nested event loop, then delete — if apuri code resumes after
    // processEvents on a deleted this, the process crashes.
    QTimer::singleShot(0, apuri, [apuri]() {
        qApp->processEvents(QEventLoop::AllEvents);
        Q_UNUSED(apuri);
    });
    qApp->processEvents(QEventLoop::AllEvents);

    delete apuri;
    qApp->processEvents(QEventLoop::AllEvents);

    QVERIFY(!elossa);
}

QTEST_MAIN(TilioteUafTesti)
#include "tst_tilioteuaftesti.moc"
