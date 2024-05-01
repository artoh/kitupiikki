#include "badges.h"

Badges::Badges()
{

}

Badges::Badges(const QStringList &badgeList)
{
    set(badgeList);
}

void Badges::set(const QStringList &badgeList)
{
    data_ = ( badgeList.contains("inbox") ? Badge::INBOX : 0 ) |
            ( badgeList.contains("outbox") ? Badge::OUTBOX : 0 ) |
            ( badgeList.contains("marked") ? Badge::MARKED : 0 ) |
            ( badgeList.contains("notify") || badgeList.contains("success") ? Badge::NOTIFICATION : 0) |
            ( badgeList.contains("info") ? Badge::INFORMATION : 0 ) |
            ( badgeList.contains("warn") ? Badge::WARN : 0 ) |
            ( badgeList.contains("error") ? Badge::ERROR : 0 ) |
            ( badgeList.contains("drafts") ? Badge::DRAFT : 0 )
        ;

}

int Badges::badges() const
{
    return data_;
}

bool Badges::hasBagde(Badge badge) const
{
    return data_ & badge;
}
