#include "aloitusinfo.h"
#include "db/kirjanpito.h"

AloitusInfo::AloitusInfo()
{

}

QString AloitusInfo::toHtml() const
{
    QString ulos = QString("<table class=%1 width='100%'><tr>").arg(luokka_);

    if( !kuva_.isEmpty()) {
        ulos.append(QString("<td width='80px'><img src=\"qrc:/pic/%1\" width=64 height=64></td><td>").arg(kuva_));
    }

    ulos.append("\n<td class=content width='100%'><h3>");
    if(!linkki_.isEmpty()){
        ulos.append(QString("<a href=\"%1\">").arg(linkki_));
    }

    ulos.append(otsikko_);

    if( !linkki_.isEmpty()) {
        ulos.append("</a>");
    }

    ulos.append("</h3>\n<p>");

    ulos.append(teksti_);

    if( !ohjelinkki_.isEmpty()) {
        ulos.append(QString(" <a href=\"ohje:/%1\">%2</a>").arg(ohjelinkki_, Kirjanpito::tr("Ohje")));
    }
    ulos.append("</p></td>");

    if( !notifyId_.isEmpty()) {
        ulos.append("<td width='32px' class='sulkuruksi'><a href='close:" + notifyId_ + "'><img src=\'qrc:/pic/close.svg\' width=32 height=32></a></td>");
    }

    ulos.append("</tr></table>\n");
    return ulos;
}

AloitusInfo::AloitusInfo(const QVariantMap &map)
{
    luokka_ = map.value("type").toString();
    otsikko_ = map.value("title").toString();
    teksti_ = map.value("info").toString();
    linkki_ = map.value("link").toString();
    kuva_ = map.value("image").toString();
    ohjelinkki_ = map.value("help").toString();

}

AloitusInfo::AloitusInfo(const QString &luokka, const QString &otsikko, const QString &teksti, const QString &linkki, const QString kuva, const QString ohjelinkki, const QString id) :
    luokka_{luokka}, otsikko_{otsikko}, teksti_{teksti}, linkki_{linkki}, kuva_{kuva}, ohjelinkki_{ohjelinkki}, notifyId_{id}
{

}
