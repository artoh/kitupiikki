/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ulkoasumaaritys.h"
#include "db/kirjanpito.h"

#include "ui_ulkoasumaaritys.h"

#include <QSettings>
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QWidget>
#include <memory>
#include "kieli/kielet.h"
#include "saldodock/saldodock.h"
#include <QMessageBox>

namespace {

enum TeemaAsetus {
    JarjestelmanTeema = 0,
    VaaleaTeema = 1,
    TummaTeema = 2
};

QPalette fusionStandardPalette()
{
    std::unique_ptr<QStyle> fusionStyle(QStyleFactory::create("Fusion"));
    return fusionStyle ? fusionStyle->standardPalette() : QPalette();
}

void setDisabledGroup(QPalette &palette,
                      const QColor &text,
                      const QColor &window,
                      const QColor &base,
                      const QColor &button,
                      const QColor &highlight,
                      const QColor &highlightedText)
{
    palette.setColor(QPalette::Disabled, QPalette::WindowText, text);
    palette.setColor(QPalette::Disabled, QPalette::Text, text);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, text);
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, text);
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, text);
    palette.setColor(QPalette::Disabled, QPalette::Window, window);
    palette.setColor(QPalette::Disabled, QPalette::Base, base);
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, base);
    palette.setColor(QPalette::Disabled, QPalette::Button, button);
    palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, window);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, highlight);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, highlightedText);
    palette.setColor(QPalette::Disabled, QPalette::Light, button);
    palette.setColor(QPalette::Disabled, QPalette::Midlight, button);
    palette.setColor(QPalette::Disabled, QPalette::Mid, button);
    palette.setColor(QPalette::Disabled, QPalette::Dark, text);
    palette.setColor(QPalette::Disabled, QPalette::Shadow, text);
}

QPalette fusionDarkPalette()
{
    QPalette palette = fusionStandardPalette();
    const QColor window(53, 53, 53);
    const QColor base(35, 35, 35);
    const QColor alternate(45, 45, 45);
    const QColor button(53, 53, 53);
    const QColor text(Qt::white);
    const QColor highlight(42, 130, 218);
    const QColor disabledText(127, 127, 127);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternate);
    palette.setColor(QPalette::ToolTipBase, window);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, highlight);
    palette.setColor(QPalette::LinkVisited, QColor(156, 100, 220));
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Light, QColor(90, 90, 90));
    palette.setColor(QPalette::Midlight, QColor(70, 70, 70));
    palette.setColor(QPalette::Mid, QColor(60, 60, 60));
    palette.setColor(QPalette::Dark, QColor(25, 25, 25));
    palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
    palette.setColor(QPalette::PlaceholderText, disabledText);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    palette.setColor(QPalette::Accent, highlight);
#endif
    setDisabledGroup(palette, disabledText, window, base, button,
                     QColor(70, 70, 70), disabledText);
    return palette;
}

QPalette fusionLightPalette()
{
    QPalette palette = fusionStandardPalette();
    const QColor window(239, 239, 239);
    const QColor base(Qt::white);
    const QColor alternate(245, 245, 245);
    const QColor button(239, 239, 239);
    const QColor text(Qt::black);
    const QColor highlight(42, 130, 218);
    const QColor disabledText(127, 127, 127);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternate);
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(0, 122, 204));
    palette.setColor(QPalette::LinkVisited, QColor(102, 51, 153));
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Light, QColor(255, 255, 255));
    palette.setColor(QPalette::Midlight, QColor(225, 225, 225));
    palette.setColor(QPalette::Mid, QColor(185, 185, 185));
    palette.setColor(QPalette::Dark, QColor(120, 120, 120));
    palette.setColor(QPalette::Shadow, QColor(80, 80, 80));
    palette.setColor(QPalette::PlaceholderText, disabledText);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    palette.setColor(QPalette::Accent, highlight);
#endif
    setDisabledGroup(palette, disabledText, window, alternate, button,
                     QColor(200, 200, 200), QColor(80, 80, 80));
    return palette;
}

void refreshPaletteAwareWidgets(QApplication &app)
{
    const QPalette appPalette = app.palette();
    const QWidgetList widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets) {
        widget->setPalette(appPalette);

        const QString style = widget->styleSheet();
        if (style.contains("palette(", Qt::CaseInsensitive)) {
            widget->setStyleSheet(QString());
            widget->setStyleSheet(style);
        }

        if (QStyle *styleEngine = widget->style()) {
            styleEngine->unpolish(widget);
            styleEngine->polish(widget);
        }
        widget->update();
    }
}

} // namespace

UlkoasuMaaritys::UlkoasuMaaritys() :
    MaaritysWidget(),
    ui(new Ui::Ulkoasu)
{
    ui->setupUi(this);

    for(int i=8; i < 21; i++) {
        ui->kokoCombo->addItem(QString("%1 pt").arg(i), i);
    }

    ui->kokoCombo->setCurrentIndex(4);
    ui->fonttiCombo->setCurrentFont(QFont("FreeSans"));

    connect(ui->oletusfontti, &QRadioButton::clicked, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->omafontti, &QRadioButton::clicked, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->fonttiCombo, &QFontComboBox::currentFontChanged, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->kokoCombo, &QComboBox::currentTextChanged, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->saldotCheck, &QCheckBox::clicked, this, &UlkoasuMaaritys::naytaSaldot);
    connect(ui->jarjestelmaTeema, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaTeema);
    connect(ui->vaaleaTeema, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaTeema);
    connect(ui->tummaTeema, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaTeema);

    connect( ui->fiKieli, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaKieli);
    connect( ui->svKieli, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaKieli);
    connect( ui->enKieli, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaKieli);
    connect( ui->tilikarttaKieli, &KieliCombo::currentTextChanged, this, &UlkoasuMaaritys::vaihdaTilikarttaKieli);

    connect( ui->pikaPdfCheck, &QRadioButton::clicked, this, [] (bool checked) { kp()->settings()->setValue("PikaPdf", checked); });
}

UlkoasuMaaritys::~UlkoasuMaaritys()
{
    delete ui;
}

bool UlkoasuMaaritys::nollaa()
{

    QString fonttinimi = kp()->settings()->value("Fontti").toString();
    int koko = kp()->settings()->value("FonttiKoko").toInt();

    if( fonttinimi.isEmpty()) {
        ui->oletusfontti->setChecked(true);

    }
    else {
        ui->omafontti->setChecked(true);
        ui->fonttiCombo->setCurrentFont(QFont(fonttinimi));
        ui->kokoCombo->setCurrentText(QString("%1 pt").arg(koko));
    }

    ui->fonttiCombo->setEnabled( !fonttinimi.isEmpty() );
    ui->kokoCombo->setEnabled( !fonttinimi.isEmpty());

    ui->fiKieli->setChecked( Kielet::instanssi()->uiKieli() == "fi" );
    ui->svKieli->setChecked( Kielet::instanssi()->uiKieli() == "sv" );
    ui->enKieli->setChecked( Kielet::instanssi()->uiKieli() == "en" );

    ui->karttakieliGroup->setVisible( kp()->yhteysModel() );
    ui->tilikarttaKieli->valitse( Kielet::instanssi()->nykyinen() );

    ui->saldotCheck->setChecked( kp()->settings()->value("SaldoDock").toBool() );

    ui->pikaPdfCheck->setChecked( kp()->settings()->value("PikaPdf").toBool() );

    switch (teemaAsetus()) {
    case VaaleaTeema:
        ui->vaaleaTeema->setChecked(true);
        break;
    case TummaTeema:
        ui->tummaTeema->setChecked(true);
        break;
    case JarjestelmanTeema:
    default:
        ui->jarjestelmaTeema->setChecked(true);
        break;
    }

    return true;
}

void UlkoasuMaaritys::asetaFontti()
{
    if( ui->oletusfontti->isChecked()) {
        kp()->settings()->remove("Fontti");
        qApp->setFont( oletusfontti__ );
    } else {
        QFont fontti = ui->fonttiCombo->currentFont();
        qApp->setFont( QFont( fontti.family(), ui->kokoCombo->currentData().toInt() ) );
        kp()->settings()->setValue("Fontti", fontti.family());
        kp()->settings()->setValue("FonttiKoko", ui->kokoCombo->currentData().toInt());
    }
}

void UlkoasuMaaritys::naytaSaldot(bool naytetaanko)
{
    kp()->settings()->setValue("SaldoDock", naytetaanko);
    SaldoDock::dock()->alusta();
}

void UlkoasuMaaritys::vaihdaTeema()
{
    int teema = JarjestelmanTeema;
    if (ui->vaaleaTeema->isChecked()) {
        teema = VaaleaTeema;
    } else if (ui->tummaTeema->isChecked()) {
        teema = TummaTeema;
    }

    kp()->settings()->setValue("Teema", teema);
    if (QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance())) {
        asetaTeema(*app, teema);
    }
}

void UlkoasuMaaritys::vaihdaKieli()
{
    if( ui->svKieli->isChecked()) {
        Kielet::instanssi()->valitseUiKieli("sv");
        ui->tilikarttaKieli->valitse("sv");
    } else if( ui->enKieli->isChecked()) {
        Kielet::instanssi()->valitseUiKieli("en");
        ui->tilikarttaKieli->valitse("en");
    } else {
        Kielet::instanssi()->valitseUiKieli("fi");
        ui->tilikarttaKieli->valitse("fi");
    }

    QMessageBox::information(this, tr("Kieli vaihdettu"),
                             tr("Käynnistä kielen vaihtamisen jälkeen ohjelma uudelleen, "
                                "jotta valitsemasi kieli tulee käyttöön kaikissa näkymissä."));
}

void UlkoasuMaaritys::vaihdaTilikarttaKieli()
{
    Kielet::instanssi()->valitseKieli( ui->tilikarttaKieli->kieli() );
    QString kieliAvain = kp()->asetukset()->uid() + "/kieli";
    if( ui->tilikarttaKieli->kieli() == Kielet::instanssi()->uiKieli())
        kp()->settings()->remove(kieliAvain);
    else
        kp()->settings()->setValue(kieliAvain, ui->tilikarttaKieli->kieli());
}

int UlkoasuMaaritys::teemaAsetus()
{
    return kp() ? kp()->settings()->value("Teema", JarjestelmanTeema).toInt()
                : JarjestelmanTeema;
}

void UlkoasuMaaritys::asetaTeema(QApplication &app, int teemaAsetus)
{
    const bool dark = (teemaAsetus == TummaTeema) ||
                      (teemaAsetus == JarjestelmanTeema &&
                       QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    app.setPalette(dark ? fusionDarkPalette() : fusionLightPalette());
    refreshPaletteAwareWidgets(app);
}

void UlkoasuMaaritys::alustaTeema(QApplication &app)
{
    asetaTeema(app, teemaAsetus());
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                     &app, [&app](Qt::ColorScheme) {
        if (teemaAsetus() == JarjestelmanTeema) {
            asetaTeema(app, JarjestelmanTeema);
        }
    });
}

QFont UlkoasuMaaritys::oletusfontti__ = QFont();
