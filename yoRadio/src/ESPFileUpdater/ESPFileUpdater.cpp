// -- Created by Trip5 : https://github.com/trip5/ESPFileUpdater

#include "ESPFileUpdater.h"
#include <time.h>
#include <WiFiClient.h>
#include <Arduino.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <WiFi.h>

// Compatibility for mbedtls_sha256_*_ret functions (for older ESP32 Arduino cores)
#if !defined(mbedtls_sha256_starts_ret)
#define mbedtls_sha256_starts_ret mbedtls_sha256_starts
#define mbedtls_sha256_update_ret mbedtls_sha256_update
#define mbedtls_sha256_finish_ret mbedtls_sha256_finish
#endif

ESPFileUpdater::ESPFileUpdater(fs::FS& fs) : _fs(fs) {}

/// @copydoc ESPFileUpdater::checkAndUpdate(const String&, const String&, const String&, bool)
ESPFileUpdater::UpdateStatus ESPFileUpdater::checkAndUpdate(const String& localPath, const String& remoteURL, const String& maxAge, bool verbose) {
  String meta = metaPath(localPath);
  bool ForceUpdate = false;
  String newLastModified = "";
  String newHash = "";

  if(_insecure) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Info] Insecure mode enabled: disable checking of secure certificates.\n", localPath.c_str());
  }

  if (waitForSystemReadyFS()==false) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] File system not ready. Aborting update.\n", localPath.c_str());
    return FS_ERROR;
  }

  if (!_fs.exists(localPath)) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Info] File doesn't exist. Forcing download.\n", localPath.c_str());
    ForceUpdate = true;
  } else if (maxAge == "" || maxAge == "0") {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Info] Downloading.\n", localPath.c_str());
    ForceUpdate = true;
  } else {
    if (waitForSystemReadyTime()==false) {
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] System time not set. Aborting update.", localPath.c_str());
      return TIME_ERROR;
    }
    time_t now = time(nullptr);
    // check .meta file if URL has changed
    if (_fs.exists(meta)) {
      String storedURL = readMetaURL(meta);
      if (storedURL.length() > 0 && storedURL != remoteURL) {
        if (verbose) Serial.printf("[ESPFileUpdater: %s] [Info] Source URL changed. Forcing re-download.\n", localPath.c_str());
        ForceUpdate = true;
      }
    }
  }

  if (ForceUpdate == false) {
    // check .meta file and maxAge
    time_t now = time(nullptr);
    if (maxAge.length() > 0 && _fs.exists(meta)) {
      time_t metaTime = parseMetaTime(meta);
      time_t interval = parseMaxAge(maxAge);

      char nowStr[32];
      strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
      if (verbose) Serial.printf("[ESPFileUpdater: %s] Current time: %s\n", localPath.c_str(), nowStr);
      char metaTimeStr[32];
      strftime(metaTimeStr, sizeof(metaTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&metaTime));
      if (verbose) Serial.printf("[ESPFileUpdater: %s] .meta file time: %s\n", localPath.c_str(), metaTimeStr);

      if (metaTime > 0 && interval > 0 && (now < metaTime + interval)) {
        if (verbose) Serial.printf("[ESPFileUpdater: %s] [Info] Skipping update: max age (%s) not reached.\n", localPath.c_str(), maxAge.c_str());
        return MAX_AGE_NOT_REACHED;
      }
    }

    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Check] Connecting to %s\n", localPath.c_str(), remoteURL.c_str());
    if (waitForSystemReadyNetwork()==false) {
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Network connection not ready. Aborting.\n", localPath.c_str());
      return NETWORK_ERROR;
    }

    WiFiClientSecure secureClient;  
    HTTPClient http;            

    if (_insecure) {
      secureClient.setInsecure();
      http.begin(secureClient, remoteURL);
    } else {
      http.begin(remoteURL);
    }

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent(_userAgent);
    int httpCode = http.sendRequest("HEAD");

    if (httpCode <= 0) {
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Server unreachable.\n", localPath.c_str());
      return SERVER_ERROR;
    }

    String lastModified = "";
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Response] HTTP code: %d\n", localPath.c_str(), httpCode);
    if (httpCode == HTTP_CODE_NOT_FOUND) {
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] File not found on server. Trying GET anyways.\n", localPath.c_str());
    } else {
      lastModified = http.header("Last-Modified");
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Header] Last-Modified: %s\n", localPath.c_str(), lastModified.c_str());
    }

    UpdateStatus status = isRemoteFileNewer(localPath, remoteURL, lastModified, newLastModified, newHash, verbose);

    if (status != UPDATED) {
      if (status == NOT_MODIFIED) 
        if (verbose) Serial.printf("[ESPFileUpdater: %s] [Complete] Remote file is same.\n", localPath.c_str());
      if (status == FILE_NOT_FOUND) 
        if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] File not found. Aborting.\n", localPath.c_str());
      if (status == SERVER_ERROR) 
        if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Server error. Aborting.\n", localPath.c_str());
      return status;
    }

    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Update] Downloading remote file...\n", localPath.c_str());
    http.end();
  }

  if (waitForSystemReadyNetwork()==false) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Network connection not ready. Aborting.\n", localPath.c_str());
    return NETWORK_ERROR;
  }

  WiFiClientSecure secureClient;  
  HTTPClient http;            

  if (_insecure) {
    secureClient.setInsecure();
    http.begin(secureClient, remoteURL);
  } else {
    http.begin(remoteURL);
  }

  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent(_userAgent);
  int getCode = http.GET();
  if (getCode != HTTP_CODE_OK) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] GET failed. HTTP code: %d. Aborting.\n", localPath.c_str(), getCode);
    http.end();
    return SERVER_ERROR;
  }

  ensureDirExists(localPath);

  String tmpPath = localPath + ".tmp";
  File file = _fs.open(tmpPath, FILE_WRITE);
  if (!file) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Cannot open temp file for writing. Aborting.\n", localPath.c_str());
    http.end();
    return FS_ERROR;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buf[512];
  int len;
  while ((len = stream->readBytes(buf, sizeof(buf))) > 0) {
    file.write(buf, len);
    yield();
  }

  file.close();
  http.end();

  if (_fs.exists(localPath)) _fs.remove(localPath);
  _fs.rename(tmpPath, localPath);

  if (maxAge.length() == 0 || maxAge == "") return UPDATED;
  if (newHash == "") {
    File localFile = _fs.open(localPath, FILE_READ);
    newHash = calculateFileHash(localFile);
    localFile.close();
  }
  newLastModified = String((uint32_t)time(nullptr));
  if (verbose) Serial.printf("[ESPFileUpdater: %s] [Complete] File downloaded.\n", localPath.c_str());
  writeMeta(metaPath(localPath), remoteURL, newLastModified, newHash);

  return UPDATED;
}

/// @copydoc ESPFileUpdater::checkAndUpdate(const String&, const String&, const String&, bool)
ESPFileUpdater::UpdateStatus ESPFileUpdater::checkAndUpdate(const String& localPath, const String& remoteURL, bool verbose) {
  return checkAndUpdate(localPath, remoteURL, "", verbose);
}

/// @copydoc ESPFileUpdater::isRemoteFileNewer
ESPFileUpdater::UpdateStatus ESPFileUpdater::isRemoteFileNewer(const String& localPath, const String& remoteURL,
                                                               const String& lastModified,
                                                               String& newLastModified,
                                                               String& remoteHash,
                                                               bool verbose) {
  String meta = metaPath(localPath);
  String storedLastMod = readMetaLastModified(meta);
  String storedHash = readMetaHash(meta);

  if (lastModified.length() == 0) {
    // Fallback: hash both files or check GET status
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Fallback] No Last-Modified header or server ignored request. Trying GET for file hash...\n", localPath.c_str());

    WiFiClientSecure secureClient;  
    HTTPClient http;            

    if (_insecure) {
      secureClient.setInsecure();
      http.begin(secureClient, remoteURL);
    } else {
      http.begin(remoteURL);
    }

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent(_userAgent);
    int code = http.GET();
    if (code == HTTP_CODE_NOT_FOUND) {
      http.end();
      return FILE_NOT_FOUND;
    }
    if (code == -1) {
      http.end();
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Connection failed. Library error: %s\n", localPath.c_str(), http.errorToString(code).c_str());
      return CONNECTION_FAILED;
    }
    if (code != HTTP_CODE_OK) {
      http.end();
      return SERVER_ERROR;
    }

    remoteHash = calculateStreamHash(*http.getStreamPtr(), _maxSize);
    http.end();

    File localFile = _fs.open(localPath, FILE_READ);
    String localHash = calculateFileHash(localFile);
    localFile.close();

    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Local SHA256] %s\n", localPath.c_str(), localHash.c_str());
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Remote SHA256] %s\n", localPath.c_str(), remoteHash.c_str());

    if (localHash != remoteHash) {
      return UPDATED;
    } else {
      // Update .meta file with current time and hash
      time_t now = time(nullptr);
      String nowStr = String((uint32_t)now);
      String urlStored = readMetaURL(metaPath(localPath));
      writeMeta(metaPath(localPath), urlStored, nowStr, localHash);
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Check] Hashes match, updated .meta file date to now.\n", localPath.c_str());
      return NOT_MODIFIED;
    }
  }

  if (lastModified != storedLastMod) {
    newLastModified = lastModified;
    return UPDATED;
  }

  return NOT_MODIFIED;
}

String ESPFileUpdater::metaPath(const String& filePath) {
  return filePath + ".meta";
}

String ESPFileUpdater::readMetaLastModified(const String& metaPath) {
  File meta = _fs.open(metaPath, FILE_READ);
  if (!meta) return "";
  meta.readStringUntil('\n'); // skip URL
  String line = meta.readStringUntil('\n');
  meta.close();
  line.trim();
  return line;
}

String ESPFileUpdater::readMetaHash(const String& metaPath) {
  File meta = _fs.open(metaPath, FILE_READ);
  if (!meta) return "";
  meta.readStringUntil('\n'); // skip URL
  meta.readStringUntil('\n'); // skip Last-Modified
  String line = meta.readStringUntil('\n');
  meta.close();
  line.trim();
  return line;
}

String ESPFileUpdater::readMetaURL(const String& metaPath) {
  File meta = _fs.open(metaPath, FILE_READ);
  if (!meta) return "";
  String line = meta.readStringUntil('\n');
  meta.close();
  line.trim();
  return line;
}

bool ESPFileUpdater::writeMeta(const String& metaPath, const String& url, const String& lastModified, const String& sha256) {
  File meta = _fs.open(metaPath, FILE_WRITE);
  if (!meta) return false;
  meta.println(url);
  meta.println(lastModified);
  meta.println(sha256);
  meta.close();
  return true;
}

String ESPFileUpdater::calculateFileHash(File& file) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);

  uint8_t buffer[512];
  int len;
  size_t total = 0;
  while (total < _maxSize && (len = file.read(buffer, sizeof(buffer))) > 0) {
    size_t toHash = len;
    if (total + len > _maxSize) {
      toHash = _maxSize - total;
    }
    mbedtls_sha256_update_ret(&ctx, buffer, toHash);
    total += toHash;
    if (total >= _maxSize) break;
    yield();
  }

  uint8_t hash[32];
  mbedtls_sha256_finish_ret(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  String result;
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 16) result += "0";
    result += String(hash[i], HEX);
  }
  return result;
}

String ESPFileUpdater::calculateStreamHash(WiFiClient& stream, size_t maxBytes) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);

  uint8_t buffer[512];
  size_t total = 0;
  int len;

  while (total < maxBytes && stream.connected() && (len = stream.readBytes(buffer, sizeof(buffer))) > 0) {
    mbedtls_sha256_update_ret(&ctx, buffer, len);
    total += len;
    yield();
  }

  uint8_t hash[32];
  mbedtls_sha256_finish_ret(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  String result;
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 16) result += "0";
    result += String(hash[i], HEX);
  }
  return result;
}

bool ESPFileUpdater::ensureDirExists(const String& path) {
  int lastSlash = path.lastIndexOf('/');
  if (lastSlash <= 0) return true;

  String dir = path.substring(0, lastSlash);
  if (!_fs.exists(dir)) {
    return _fs.mkdir(dir);
  }
  return true;
}

time_t ESPFileUpdater::parseMaxAge(const String& maxAgeStr) {
  String s = maxAgeStr;
  s.toLowerCase();
  s.trim();
  s.replace(" ", "");
  // Find where the digits end and the unit begins
  int idx = 0;
  while (idx < s.length() && isDigit(s[idx])) idx++;
  if (idx == 0) return 0; // No number found

  int num = s.substring(0, idx).toInt();
  String unit = s.substring(idx);
  unit.replace("s", ""); // ignore plural

  if (unit == "hour" || unit == "hr" || unit == "h")
    return num * 3600;
  if (unit == "day" || unit == "d")
    return num * 86400;
  if (unit == "week" || unit == "wk" || unit == "w")
    return num * 604800; // 7 days
  if (unit == "month" || unit == "mo" || unit == "m")
    return num * 2592000; // 30 days

  return 0;
}

time_t ESPFileUpdater::parseMetaTime(const String& metaPath) {
  File meta = _fs.open(metaPath, FILE_READ);
  if (!meta) return 0;
  meta.readStringUntil('\n'); // skip URL
  String line = meta.readStringUntil('\n');
  meta.close();
  // Try to parse as epoch seconds
  line.trim();
  if (line.length() == 0) return 0;
  // If it's a number, treat as epoch
  if (line.toInt() > 100000) return line.toInt();
  // Otherwise, try to parse HTTP date
  struct tm t;
  if (strptime(line.c_str(), "%a, %d %b %Y %H:%M:%S", &t)) {
    return mktime(&t);
  }
  return 0;
}

bool ESPFileUpdater::waitForSystemReadyFS() {
  uint32_t start = millis();
  if (&_fs == nullptr) return false;
  const char* tmpname = "/.fsreadycheck135792468.tmp";
  while (millis() - start < _timeout) {
    File f = _fs.open(tmpname, FILE_WRITE);
    if (f) {
      f.print("test");
      f.close();
      _fs.remove(tmpname);
      return true;
    }
    delay(50);
    yield();
  }
  return false;
}

bool ESPFileUpdater::waitForSystemReadyTime() {
  uint32_t start = millis();
  time_t now = time(nullptr);
  while (now < 100000 && millis() - start < _timeout) {
    delay(50);
    now = time(nullptr);
    yield();
  }
  if (now < 100000) return false;
  return true;
}

bool ESPFileUpdater::waitForSystemReadyNetwork() {
  uint32_t start = millis();
  while (millis() - start < _timeout) {
    if (networkIsConnected()) return true;
    delay(500);
    yield();
  }
  return false;
}
