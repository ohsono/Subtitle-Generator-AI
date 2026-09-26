#include "TranslationEngine.h"
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>


TranslationEngine::TranslationEngine(QObject *parent) : QObject(parent) {}

void TranslationEngine::setEndpoint(const QString &url) {
  m_endpoint = url;
  while (m_endpoint.endsWith('/'))
    m_endpoint.chop(1);
}
void TranslationEngine::setApiKey(const QString &key) { m_apiKey = key; }

bool TranslationEngine::translate(QList<TranscriptSegment> &segments,
                                  const QString &sourceLang,
                                  const QString &targetLang) {
  if (segments.isEmpty())
    return true;
  emit logMessage(QString("Translating %1 segments [%2 -> %3]")
                      .arg(segments.size())
                      .arg(sourceLang)
                      .arg(targetLang));

  m_lastError.clear();
  for (int i = 0; i < segments.size(); ++i) {
    TranscriptSegment &seg = segments[i];
    QString translated;
    if (!translateText(seg.text, sourceLang, targetLang, translated)) {
      // Every request goes to the same server, so one failure means the rest
      // would fail too; stop instead of repeating the error per segment.
      emit logMessage(QString("Translation error: %1").arg(m_lastError));
      return false;
    }
    if (!translated.isEmpty())
      seg.text = translated;
    emit progress(static_cast<int>(100.0 * i / segments.size()));
  }

  emit progress(100);
  emit logMessage("Translation complete");
  return true;
}

bool TranslationEngine::translateText(const QString &text, const QString &src,
                                      const QString &tgt, QString &out) {
  QUrl url(m_endpoint + "/translate");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!m_apiKey.isEmpty())
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

  QJsonObject body;
  body["q"] = text;
  body["source"] = src.isEmpty() ? "auto" : src;
  body["target"] = tgt;
  body["format"] = "text";

  QByteArray payload = QJsonDocument(body).toJson();
  QNetworkReply *reply = m_net.post(request, payload);

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
  bool ok = false;
  if (reply->error() == QNetworkReply::NoError &&
      obj.contains("translatedText")) {
    out = obj.value("translatedText").toString();
    ok = true;
  } else if (reply->rawHeader("Server").startsWith("AirTunes")) {
    // macOS AirPlay Receiver listens on port 5000, LibreTranslate's default,
    // and answers every request with 403.
    m_lastError =
        QString("%1 is macOS AirPlay Receiver, not a translation server. Run "
                "LibreTranslate on another port (e.g. --port 5001) and set it "
                "as the API endpoint in Settings, or turn off AirPlay Receiver "
                "in System Settings > General > AirDrop & Handoff")
            .arg(m_endpoint);
  } else if (obj.contains("error")) {
    m_lastError = QString("%1 (%2)").arg(obj.value("error").toString(),
                                         reply->errorString());
  } else if (reply->error() != QNetworkReply::NoError) {
    m_lastError = reply->errorString();
  } else {
    m_lastError = QString("Unexpected response from %1").arg(url.toString());
  }

  reply->deleteLater();
  return ok;
}

QStringList TranslationEngine::supportedLanguages() {
  return {"Auto Detect",
          "English",
          "Spanish",
          "French",
          "German",
          "Italian",
          "Portuguese",
          "Russian",
          "Japanese",
          "Chinese (Simplified)",
          "Chinese (Traditional)",
          "Korean",
          "Arabic",
          "Hindi",
          "Dutch",
          "Polish",
          "Turkish",
          "Ukrainian",
          "Vietnamese",
          "Thai",
          "Indonesian",
          "Malay",
          "Swedish",
          "Norwegian",
          "Danish"};
}

QString TranslationEngine::languageCode(const QString &name) {
  static QHash<QString, QString> codes = {{"Auto Detect", "auto"},
                                          {"English", "en"},
                                          {"Spanish", "es"},
                                          {"French", "fr"},
                                          {"German", "de"},
                                          {"Italian", "it"},
                                          {"Portuguese", "pt"},
                                          {"Russian", "ru"},
                                          {"Japanese", "ja"},
                                          {"Chinese (Simplified)", "zh"},
                                          {"Chinese (Traditional)", "zh"},
                                          {"Korean", "ko"},
                                          {"Arabic", "ar"},
                                          {"Hindi", "hi"},
                                          {"Dutch", "nl"},
                                          {"Polish", "pl"},
                                          {"Turkish", "tr"},
                                          {"Ukrainian", "uk"},
                                          {"Vietnamese", "vi"},
                                          {"Thai", "th"},
                                          {"Indonesian", "id"},
                                          {"Malay", "ms"},
                                          {"Swedish", "sv"},
                                          {"Norwegian", "no"},
                                          {"Danish", "da"}};
  return codes.value(name, "en");
}
