#ifndef HUBWEBPAGE_H
#define HUBWEBPAGE_H

#include <QWebEnginePage>

class HubWebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit HubWebPage(QObject *parent = nullptr);

signals:
    void toimistoLinkki(const QString& id);

protected:
    void supportLogin(const QString& cloudId);
    void login(const QString& cloudId);

    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
};

#endif // HUBWEBPAGE_H
