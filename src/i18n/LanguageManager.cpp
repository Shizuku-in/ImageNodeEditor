#include "i18n/LanguageManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QSettings>

LanguageManager &LanguageManager::instance()
{
    static LanguageManager manager;
    return manager;
}

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
{
}

QString LanguageManager::currentLanguage() const
{
    return m_currentLanguage.isEmpty() ? defaultLanguage() : m_currentLanguage;
}

QString LanguageManager::defaultLanguage() const
{
    const QString savedLanguage = QSettings().value("language").toString();
    if (!savedLanguage.isEmpty()) {
        return savedLanguage;
    }
    return QLocale::system().name().startsWith("zh", Qt::CaseInsensitive) ? "zh_CN" : "en_US";
}

bool LanguageManager::setLanguage(const QString &language)
{
    const QString normalized = language.startsWith("zh", Qt::CaseInsensitive) ? "zh_CN" : "en_US";
    if (m_currentLanguage == normalized) {
        return true;
    }

    auto *app = QCoreApplication::instance();
    if (!app) {
        return false;
    }

    if (m_translatorInstalled) {
        app->removeTranslator(&m_translator);
        m_translatorInstalled = false;
    }

    bool ok = true;
    if (normalized == "zh_CN") {
        ok = loadChineseTranslator();
        if (ok) {
            app->installTranslator(&m_translator);
            m_translatorInstalled = true;
        }
    }

    if (ok) {
        m_currentLanguage = normalized;
        QSettings().setValue("language", normalized);
        emit languageChanged();
    }
    return ok;
}

bool LanguageManager::loadChineseTranslator()
{
    const QString fileName = "ImageNodeEditor_zh_CN";
    const QStringList searchPaths {
        QCoreApplication::applicationDirPath() + "/translations",
        QDir::currentPath() + "/translations",
        QDir::currentPath() + "/build-qt/translations"
    };
    for (const QString &path : searchPaths) {
        if (m_translator.load(fileName, path)) {
            return true;
        }
    }
    return false;
}

