#ifndef TAULUKONKASITTELIJA_H
#define TAULUKONKASITTELIJA_H

#include <QVector>
#include <QRegularExpression>
#include "model/euro.h"

class TaulukonKasittelija
{
public:
    static QString processTable(const QString& table);

    Euro sum(int column) const;

protected:
    class Cell {
    public:
        Cell();
        Cell(const QString& cell);

        QString content(TaulukonKasittelija* processor, int column) const;
        bool isEmpty() const;
        bool isZero() const;
        Euro sum() const;
    protected:
        QString cleaned() const;

        QString tag_;
        QString content_;

        static QRegularExpression tagCleaner__;
        static QRegularExpression emptyCleaner__;
    };


    class Row {
    public:
        Row();
        Row(const QString& content);

        bool isEmpty() const;
        bool isZero() const;
        QString content(TaulukonKasittelija* processer) const;
        Euro sum(int column) const;

    protected:
        QVector<Cell> cells_;

    };

private:
    TaulukonKasittelija();
    QString process(const QString& table);

    void makeTable(const QString& table);
    QString getTable();

private:
    QString tag_;
    QList<Row> rows_;

};

#endif // TAULUKONKASITTELIJA_H
