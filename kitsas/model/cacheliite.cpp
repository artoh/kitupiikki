#include "cacheliite.h"

CacheLiite::CacheLiite()
{

}

CacheLiite::CacheLiite(LiiteTila tila) :
    tila_{tila}
{

}

void CacheLiite::setData(const QByteArray &data)
{
    data_ = data;
    tila_ = HAETTU;
}
