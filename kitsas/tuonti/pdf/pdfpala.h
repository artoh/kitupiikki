#ifndef TUONTI_PDFPALA_H
#define TUONTI_PDFPALA_H

#include <QString>
#include <QPdfSelection>

namespace Tuonti {

class PdfPala
{
public:
    PdfPala();    
    PdfPala(PdfPala* kopioitava);
    PdfPala(const QPdfSelection& selection);

    ~PdfPala();

    int yla() const  { return yla_;}
    int ala() const { return ala_;}
    int vasen() const { return vasen_;}
    int oikea() const { return oikea_;}
    int kirjainLeveys() const { return kirjainLeveys_;}
    int korkeus() const { return oikea_ - vasen_;}

    QString teksti() const { return teksti_;}

    PdfPala* seuraava() { return seuraava_; }
    PdfPala* osapala() { return osapala_;}

    void lisaaJalkeen( PdfPala* lisattava );
    void yhdistaEdelliseen(PdfPala* edellinen);
    void asetaSeuraava( PdfPala* seuraava);

protected:
    int yla_;
    int ala_;
    int vasen_;
    int oikea_;
    int kirjainLeveys_ = 0;
    QString teksti_;

    PdfPala* seuraava_ = nullptr;
    PdfPala* osapala_ = nullptr;

};

} // namespace Tuonti

#endif // TUONTI_PDFPALA_H
