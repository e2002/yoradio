// -- Created by Trip5 : https://github.com/trip5/ESPFileUpdater

#include "espfileupdater.h"
#include <time.h>

/// @brief Construct a new ESPFileUpdater object.
/// @param fs Reference to the filesystem.
ESPFileUpdater::ESPFileUpdater(fs::FS& fs) : _fs(fs) {}

/// @copydoc ESPFileUpdater::checkAndUpdate(const String&, const String&, bool)
ESPFileUpdater::UpdateStatus ESPFileUpdater::checkAndUpdate(const String& localPath, const String& remoteURL, bool verbose) {
  return checkAndUpdate(localPath, remoteURL, "", verbose);
}

/// @copydoc ESPFileUpdater::checkAndUpdate(const String&, const String&)
ESPFileUpdater::UpdateStatus ESPFileUpdater::checkAndUpdate(const String& localPath, const String& remoteURL) { 
  return checkAndUpdate(localPath, remoteURL, "", false);
}

/// @copydoc ESPFileUpdater::checkAndUpdate(const String&, const String&, const String&, bool)
ESPFileUpdater::UpdateStatus ESPFileUpdater::checkAndUpdate(const String& localPath, const String& remoteURL, const String& maxAge, bool verbose) {
  String meta = metaPath(localPath);
  bool ForceUpdate = false;
  time_t now = time(nullptr);
  String newLastModified = "";
  String newHash = "";

  if (!_fs.exists(localPath)) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Info] File doesn't exist. Forcing download.\n", localPath.c_str());
    ForceUpdate = true;
  } else {
    if (now < 100000) {
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] System time not set. Aborting update.", localPath.c_str());
      return TIME_ERROR;
    }
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
    if (maxAge.length() > 0 && _fs.exists(meta)) {
      time_t metaTime = parseMetaTime(meta);
      time_t interval = parseMaxAge(maxAge);
      if (metaTime > 0 && interval > 0 && (now < metaTime + interval)) {
        if (verbose) Serial.printf("[ESPFileUpdater: %s] [Info] Skipping update: max age (%s) not reached.\n", localPath.c_str(), maxAge.c_str());
        return MAX_AGE_NOT_REACHED;
      }

      char nowStr[32];
      strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
      if (verbose) Serial.printf("[ESPFileUpdater: %s] Current time: %s (epoch: %ld)\n", localPath.c_str(), nowStr, now);
      char metaTimeStr[32];
      strftime(metaTimeStr, sizeof(metaTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&metaTime));
      if (verbose) Serial.printf("[ESPFileUpdater: %s] .meta file time: %s (epoch: %ld)\n", localPath.c_str(), metaTimeStr, metaTime);
    }

    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Check] Connecting to %s\n", localPath.c_str(), remoteURL.c_str());
    HTTPClient http;
    http.begin(remoteURL);
    int httpCode = http.sendRequest("HEAD");

    if (httpCode <= 0) {
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Server unreachable.", localPath.c_str());
      return SERVER_ERROR;
    }

    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Response] HTTP code: %d\n", localPath.c_str(), httpCode);
    if (httpCode == HTTP_CODE_NOT_FOUND) {
      if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] File not found on server.", localPath.c_str());
      return FILE_NOT_FOUND;
    }

    String lastModified = http.header("Last-Modified");
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Header] Last-Modified: %s\n", localPath.c_str(), lastModified.c_str());

    UpdateStatus status = isRemoteFileNewer(localPath, remoteURL, lastModified, newLastModified, newHash, verbose);

    if (status != UPDATED) {
      if (status == NOT_MODIFIED) 
        if (verbose) Serial.printf("[ESPFileUpdater: %s] [Complete] Remote file is same.\n", localPath.c_str());
      return status;
    }

    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Update] Remote file is newer. Downloading...\n", localPath.c_str());
    http.end(); // Close previous HEAD connection
  }

  HTTPClient http;
  http.begin(remoteURL);
  int getCode = http.GET();
  if (getCode != HTTP_CODE_OK) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] GET failed. HTTP code: %d\n", localPath.c_str(), getCode);
    http.end();
    return SERVER_ERROR;
  }

  ensureDirExists(localPath);

  String tmpPath = localPath + ".tmp";
  File file = _fs.open(tmpPath, FILE_WRITE);
  if (!file) {
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Error] Cannot open temp file for writing.\n", localPath.c_str());
    http.end();
    return SPIFFS_ERROR;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buf[512];
  int len;
  while ((len = stream->readBytes(buf, sizeof(buf))) > 0) {
    file.write(buf, len);
  }

  file.close();
  http.end();

  if (_fs.exists(localPath)) _fs.remove(localPath);
  _fs.rename(tmpPath, localPath);

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

/// @copydoc ESPFileUpdater::isRemoteFileNewer
ESPFileUpdater::UpdateStatus ESPFileUpdater::isRemoteFileNewer(const String& localPath, const String& url,
                                                               const String& lastModified,
                                                               String& newLastModified,
                                                               String& remoteHash,
                                                               bool verbose) {
  String meta = metaPath(localPath);
  String storedLastMod = readMetaLastModified(meta);
  String storedHash = readMetaHash(meta);

  if (lastModified.length() == 0) {
    // Fallback: hash both files
    if (verbose) Serial.printf("[ESPFileUpdater: %s] [Fallback] No Last-Modified header. Comparing file hashes...\n", localPath.c_str());

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
      http.end();
      return SERVER_ERROR;
    }

    remoteHash = calculateStreamHash(*http.getStreamPtr(), ESPFILEUPDATER_MAXSIZE);
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

/// @brief Get the path to the .meta file for a given file.
/// @param filePath Path to the local file.
/// @return Path to the .meta file.
String ESPFileUpdater::metaPath(const String& filePath) {
  return filePath + ".meta";
}

/// @brief Read the last-modified value from the .meta file.
/// @param metaPath Path to the .meta file.
/// @return Last-modified string.
String ESPFileUpdater::readMetaLastModified(const String& metaPath) {
  File meta = _fs.open(metaPath, FILE_READ);
  if (!meta) return "";
  meta.readStringUntil('\n'); // skip URL
  String line = meta.readStringUntil('\n');
  meta.close();
  line.trim();
  return line;
}

/// @brief Read the hash value from the .meta file.
/// @param metaPath Path to the .meta file.
/// @return Hash string.
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

/// @brief Read the original URL from the .meta file.
/// @param metaPath Path to the .meta file.
/// @return URL string.
String ESPFileUpdater::readMetaURL(const String& metaPath) {
  File meta = _fs.open(metaPath, FILE_READ);
  if (!meta) return "";
  String line = meta.readStringUntil('\n');
  meta.close();
  line.trim();
  return line;
}

/// @brief Write the .meta file with URL, last-modified, and hash.
/// @param metaPath Path to the .meta file.
/// @param url Source URL.
/// @param lastModified Last-modified string (epoch seconds).
/// @param sha256 SHA256 hash string.
/// @return true if successful, false otherwise.
bool ESPFileUpdater::writeMeta(const String& metaPath, const String& url, const String& lastModified, const String& sha256) {
  File meta = _fs.open(metaPath, FILE_WRITE);
  if (!meta) return false;
  meta.println(url);
  meta.println(lastModified);
  meta.println(sha256);
  meta.close();
  return true;
}

/// @brief Calculate the SHA256 hash of a file.
/// @param file Open file handle.
/// @return SHA256 hash string.
String ESPFileUpdater::calculateFileHash(File& file) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);

  uint8_t buffer[512];
  int len;
  size_t total = 0;
  while (total < ESPFILEUPDATER_MAXSIZE && (len = file.read(buffer, sizeof(buffer))) > 0) {
    size_t toHash = len;
    if (total + len > ESPFILEUPDATER_MAXSIZE) {
      toHash = ESPFILEUPDATER_MAXSIZE - total;
    }
    mbedtls_sha256_update_ret(&ctx, buffer, toHash);
    total += toHash;
    if (total >= ESPFILEUPDATER_MAXSIZE) break;
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

/// @brief Calculate the SHA256 hash of a stream.
/// @param stream WiFiClient stream.
/// @param maxBytes Maximum bytes to read.
/// @return SHA256 hash string.
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

/// @brief Ensure the directory for a given path exists.
/// @param path File path.
/// @return true if directory exists or was created, false otherwise.
bool ESPFileUpdater::ensureDirExists(const String& path) {
  int lastSlash = path.lastIndexOf('/');
  if (lastSlash <= 0) return true;

  String dir = path.substring(0, lastSlash);
  if (!_fs.exists(dir)) {
    return _fs.mkdir(dir);
  }
  return true;
}

/// @brief Parse a max-age string (e.g., "7d") to seconds.
/// @param maxAgeStr Max-age string.
/// @return Number of seconds.
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
  if (unit == "month" || unit == "mo" || unit == "m")
    return num * 2592000; // 30 days

  return 0;
}

/// @brief Parse the last-modified time from the .meta file.
/// @param metaPath Path to the .meta file.
/// @return Time as epoch seconds.
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