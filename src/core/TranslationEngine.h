#pragma once
#include "TranscriptionEngine.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>


class TranslationEngine : public QObject {
  Q_OBJECT
public:
  explicit TranslationEngine(QObject *parent = nullptr);

  void setEndpoint(const QString &url);
  void setApiKey(const QString &key);

  // Stops at the first failed request and returns false; segments already
  // translated keep their new text, the rest keep the original.
  bool translate(QList<TranscriptSegment> &segments, const QString &sourceLang,
                 const QString &targetLang);
  QString lastError() const { return m_lastError; }

  static QStringList supportedLanguages();
  static QString languageCode(const QString &name);

signals:
  void progress(int percent);
  void logMessage(const QString &msg);

private:
  bool translateText(const QString &text, const QString &src,
                     const QString &tgt, QString &out);

  QNetworkAccessManager m_net;
  QString m_endpoint = "http://127.0.0.1:5001";
  QString m_apiKey;
  QString m_lastError;
};
