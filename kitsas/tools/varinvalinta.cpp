#include "varinvalinta.h"
#include <QColorDialog>
#include <QMouseEvent>

#include <QImage>
#include <QPixmap>

VarinValinta::VarinValinta(QWidget *parent) :
    QLabel(parent)
{

}

void VarinValinta::setColor(const QString &variteksti)
{
    QStringList lista = variteksti.split(",");
    vari_.setRgb( lista.value(0).toInt(), lista.value(1).toInt(), lista.value(2).toInt() );
    paivita();
}

QString VarinValinta::color() const
{
    return QString("%1,%2,%3").arg(vari_.red()).arg(vari_.green()).arg(vari_.blue());
}

void VarinValinta::paivita()
{
    QImage image(100,20,QImage::Format_RGB32);
    image.fill(vari_);

    setPixmap( QPixmap::fromImage(image) );
}

void VarinValinta::vaihda()
{
    vari_ = QColorDialog::getColor( vari_, this, tr("Valitse väri"), QColorDialog::DontUseNativeDialog );
    paivita();
    emit variVaihtui(vari_);
}

void VarinValinta::mousePressEvent(QMouseEvent *event)
{
    if( event->button() == Qt::LeftButton)
        vaihda();
}
