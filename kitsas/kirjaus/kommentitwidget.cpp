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
#include "kommentitwidget.h"

#include "model/tosite.h"
#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include "pilvi/pilvimodel.h"

#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSettings>

KommentitWidget::KommentitWidget(Tosite *tosite, QWidget *parent)
    : QSplitter(parent), tosite_(tosite)
{
    setOrientation(Qt::Vertical);

    browser_ = new QTextBrowser();
    addWidget(browser_);

    edit_ = new QPlainTextEdit();
    edit_->setPlaceholderText(tr("Kirjoita lisättävä kommentti tähän"));
    button_ = new QPushButton(QIcon(":/pic/kupla-harmaa.png"), tr("Kommentoi"));
    button_->setAutoDefault(true);

    QHBoxLayout *leiska = new QHBoxLayout();
    leiska->addWidget(edit_);
    leiska->addWidget(button_);

    QWidget *alaWg = new QWidget();
    alaWg->setLayout(leiska);
    addWidget(alaWg);

    restoreState(kp()->settings()->value("KommenttiSplitter").toByteArray());

    connect( tosite_, &Tosite::ladattu, this, &KommentitWidget::lataa );
    connect( edit_, &QPlainTextEdit::textChanged, this, &KommentitWidget::paivita);
    connect( button_, &QPushButton::clicked, this, &KommentitWidget::tallenna);

    browser_->setStyleSheet("{p: margin-top: 4px;}");
}

KommentitWidget::~KommentitWidget()
{
    kp()->settings()->setValue("KommenttiSplitter", saveState());
}

void KommentitWidget::lataa()
{
    browser_->clear();
    edit_->clear();

    bool oikeus = kp()->yhteysModel() &&
            ( tosite_->id() ? kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_KOMMENTTI) :
                              kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_LUONNOS | YhteysModel::TOSITE_MUOKKAUS) );


    edit_->setEnabled(oikeus);
    button_->setEnabled(false);
    button_->setVisible(tosite_->id());

    QVariant data = tosite_->data(Tosite::KOMMENTIT);
    QVariantList lista = data.toList();

    bool eka = true;

    QString txt="<html><body>";

    for(auto &item: lista){
        if(!eka)
            txt.append("<hr>");

        QVariantMap map = item.toMap();
        QString nimi = map.value("nimi").toString();
        QString aika = map.value("aika").toDateTime().toString("dd.MM.yyyy hh.mm");
        QString teksti = map.value("teksti").toString().toHtmlEscaped().replace("\n","<br>");

        txt.append(QString("<h3>%1 %2</h3>").arg(nimi, aika));
        txt.append(QString("<p>%1</p>").arg(teksti));
        eka = false;
    }
    emit kommentteja(!lista.isEmpty());
    txt.append("</body></html");
    browser_->setHtml(txt);
    browser_->scrollToAnchor("loppu");
}

void KommentitWidget::paivita()
{
    QVariantMap map;
    if( edit_->toPlainText().isEmpty()) {
        tosite_->setData(Tosite::KOMMENTTI, QString());
    } else {
        map.insert("teksti", edit_->toPlainText());
        tosite_->setData(Tosite::KOMMENTTI, map);
    }

    bool oikeus = kp()->yhteysModel() &&            
            tosite_->id() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_KOMMENTTI);
    button_->setEnabled(oikeus && !edit_->toPlainText().isEmpty());
    emit kommentteja( !edit_->toPlainText().isEmpty() ||
                      !browser_->toPlainText().isEmpty());
}

void KommentitWidget::tallenna()
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tosite_->id()), KpKysely::POST);
    QVariantMap map;
    map.insert("teksti", edit_->toPlainText());
    button_->setEnabled(false);
    connect(kysely, &KpKysely::vastaus, this, &KommentitWidget::tallennettu);
    kysely->kysy(map);
}

void KommentitWidget::tallennettu()
{
    QString uusi;
    if(!browser_->toPlainText().isEmpty())
        uusi = "<hr>";
    uusi.append(QString("<h3>%1 %2</h3>").arg( kp()->pilvi()->kayttaja().nimi() , QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")));
    uusi.append(QString("<p>%1</p></body>").arg(edit_->toPlainText().replace("\n","<br>")));

    browser_->setHtml( browser_->toHtml().replace("</body>", uusi) );

    emit kommentteja(true);

    edit_->clear();
}
