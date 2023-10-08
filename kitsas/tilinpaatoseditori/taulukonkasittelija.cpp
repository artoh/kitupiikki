#include "taulukonkasittelija.h"

#include <QDebug>

QString TaulukonKasittelija::processTable(const QString &table)
{
    TaulukonKasittelija kasittelija;
    return kasittelija.process(table);
}

Euro TaulukonKasittelija::sum(int column) const
{
    Euro value;
    for(auto& row: rows_)
        value = value + row.sum(column);
    return value;
}

bool TaulukonKasittelija::isColumnEmpty(int column) const
{
    return emptyColumns_.value(column);
}

TaulukonKasittelija::TaulukonKasittelija()
{

}

QString TaulukonKasittelija::process(const QString &table)
{
    makeTable(table);
    checkEmptyColumns();
    return getTable();
}

void TaulukonKasittelija::checkEmptyColumns()
{
    emptyColumns_.fill(true,10);
    for(const auto& row: rows_) {
        const QVector<Cell>& cells = row.cells();
        for(int i=0; i < cells.size(); i++) {
            const Cell& cell = cells.at(i);
            if( !cell.isEmpty() && !cell.isZero() && !cell.isSum()) {
                emptyColumns_[i] = false;
            }
        }
    }
}

void TaulukonKasittelija::makeTable(const QString &table)
{
    tag_ = table.left( table.indexOf('>') + 1 );
    int position = table.indexOf("<tr>");
    while( position >= 0) {
        const int endPosition = table.indexOf("</tr>", position);
        const QString row = table.mid(position+4, endPosition-position-4);
        rows_.append(Row(row));
        position = table.indexOf("<tr>", endPosition);
    }
}

QString TaulukonKasittelija::getTable()
{
    QString out = tag_;
    bool previousEmpty = false;
    for(const auto& row: rows_) {
        if(row.isEmpty()){
            if( !previousEmpty) {
                out.append(row.content(this));
                previousEmpty = true;
            }
        } else {
            if( !row.isZero()) {
                out.append( row.content(this));
                previousEmpty = false;
            }
        }
    }
    out.append("</table>");

    qDebug() << out;

    return out;
}



TaulukonKasittelija::Cell::Cell()
{

}

TaulukonKasittelija::Cell::Cell(const QString &cell)
{
    int position = cell.indexOf(">");
    int closePosition = cell.lastIndexOf("<");

    tag_ = cell.left(position + 1);
    content_ = cell.mid(position + 1, closePosition - position - 1);

    qDebug() << " CELL TAG " << tag_ << " CONTENT " << content_;
}

QString TaulukonKasittelija::Cell::content(TaulukonKasittelija *processor, int column) const
{
    QString cleaned = this->cleaned();
    QString content = this->content_;

    if( isSum()) {
        return tag_ + content.replace("^^^^", processor->sum(column).display(true) + "</td>");
    } else {
        return tag_ + content.replace("--","&ndash;") + "</td>";
    }
}

bool TaulukonKasittelija::Cell::isEmpty() const
{
    return cleaned().isEmpty();
}

bool TaulukonKasittelija::Cell::isZero() const
{
    return cleaned() == "0,00 €";
}

bool TaulukonKasittelija::Cell::isSum() const
{
    return cleaned() == "^^^^";
}

Euro TaulukonKasittelija::Cell::sum() const
{
    QString clean = cleaned();
    if( clean.endsWith("€")) {
        QString value = clean.replace(",",".").replace("--","-").replace("−","-");
        value = value.remove(emptyCleaner__);
        return Euro::fromString( value );
    } else {
        return Euro::Zero;
    }
}

QString TaulukonKasittelija::Cell::cleaned() const
{
    QString cleaned = content_;
    return cleaned.remove(tagCleaner__).trimmed();
}

QRegularExpression TaulukonKasittelija::Cell::tagCleaner__ = QRegularExpression("<[^>]*>");
QRegularExpression TaulukonKasittelija::Cell::emptyCleaner__ = QRegularExpression("[^0-9,\\.,\\-]");


TaulukonKasittelija::Row::Row()
{

}

TaulukonKasittelija::Row::Row(const QString &content)
{
    int position = content.indexOf("<td");
    while( position >= 0) {
        const int endPosition = content.indexOf("</td>", position);
        const QString cellData = content.mid(position, endPosition-position+5);
        cells_.append(Cell(cellData));
        position = content.indexOf("<td", endPosition);
    }
}

bool TaulukonKasittelija::Row::isEmpty() const
{
    for( const auto& cell : cells_)
        if( !cell.isEmpty())
            return false;
    return true;
}

bool TaulukonKasittelija::Row::isZero() const
{
    for(int i=1; i < cells_.size(); i++)
        if( !cells_.value(i).isZero() )
            return false;
    return true;
}

QString TaulukonKasittelija::Row::content(TaulukonKasittelija *processer) const
{
    QString out = "<tr>";
    for(int i=0; i < cells_.size(); i++) {
        if( !processer->isColumnEmpty(i)) {
            out.append( cells_.at(i).content(processer, i));
        }
    }
    return out + "</tr>";
}

Euro TaulukonKasittelija::Row::sum(int column) const
{
    Cell cell = cells_.value(column);
    return cell.sum();
}

QVector<TaulukonKasittelija::Cell> TaulukonKasittelija::Row::cells() const
{
    return cells_;
}




