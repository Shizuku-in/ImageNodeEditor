#pragma once

#include <QObject>
#include <QTranslator>

class LanguageManager : public QObject
{
    Q_OBJECT

public:
    static LanguageManager &instance();

    QString currentLanguage() const;
    QString defaultLanguage() const;
    bool setLanguage(const QString &language);

signals:
    void languageChanged();

private:
    explicit LanguageManager(QObject *parent = nullptr);
    bool loadChineseTranslator();

    QTranslator m_translator;
    QString m_currentLanguage;
    bool m_translatorInstalled = false;
};

