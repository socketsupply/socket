#include "../http.hh"
#include "../json.hh"
#include "../string.hh"

using ssc::runtime::string::toProperCase;
using ssc::runtime::string::toLowerCase;
using ssc::runtime::string::trim;
using ssc::runtime::string::split;

namespace ssc::runtime::http {
  static const Map<int, Status> statuses = {
    {100, {100, "Continue"}},
    {101, {101, "Switching Protocols"}},
    {102, {102, "Processing"}},
    {103, {103, "Early Hints"}},
    {200, {200, "OK"}},
    {201, {201, "Created"}},
    {202, {202, "Accepted"}},
    {203, {203, "Non-Authoritative Information"}},
    {204, {204, "No Content"}},
    {205, {205, "Reset Content"}},
    {206, {206, "Partial Content"}},
    {207, {207, "Multi-Status"}},
    {208, {208, "Already Reported"}},
    {226, {226, "IM Used"}},
    {300, {300, "Multiple Choices"}},
    {301, {301, "Moved Permanently"}},
    {302, {302, "Found"}},
    {303, {303, "See Other"}},
    {304, {304, "Not Modified"}},
    {305, {305, "Use Proxy"}},
    {307, {307, "Temporary Redirect"}},
    {308, {308, "Permanent Redirect"}},
    {400, {400, "Bad Request"}},
    {401, {401, "Unauthorized"}},
    {402, {402, "Payment Required"}},
    {403, {403, "Forbidden"}},
    {404, {404, "Not Found"}},
    {405, {405, "Method Not Allowed"}},
    {406, {406, "Not Acceptable"}},
    {407, {407, "Proxy Authentication Required"}},
    {408, {408, "Request Timeout"}},
    {409, {409, "Conflict"}},
    {410, {410, "Gone"}},
    {411, {411, "Length Required"}},
    {412, {412, "Precondition Failed"}},
    {413, {413, "Payload Too Large"}},
    {414, {414, "URI Too Long"}},
    {415, {415, "Unsupported Media Type"}},
    {416, {416, "Range Not Satisfiable"}},
    {417, {417, "Expectation Failed"}},
    {418, {418, "I'm a Teapot"}},
    {421, {421, "Misdirected Request"}},
    {422, {422, "Unprocessable Entity"}},
    {423, {423, "Locked"}},
    {424, {424, "Failed Dependency"}},
    {425, {425, "Too Early"}},
    {426, {426, "Upgrade Required"}},
    {428, {428, "Precondition Required"}},
    {429, {429, "Too Many Requests"}},
    {431, {431, "Request Header Fields Too Large"}},
    {451, {451, "Unavailable For Legal Reasons"}},
    {500, {500, "Internal Server Error"}},
    {501, {501, "Not Implemented"}},
    {502, {502, "Bad Gateway"}},
    {503, {503, "Service Unavailable"}},
    {504, {504, "Gateway Timeout"}},
    {505, {505, "HTTP Version Not Supported"}},
    {506, {506, "Variant Also Negotiates"}},
    {507, {507, "Insufficient Storage"}},
    {508, {508, "Loop Detected"}},
    {509, {509, "Bandwidth Limit Exceeded"}},
    {510, {510, "Not Extended"}},
    {511, {511, "Network Authentication Required"}}
  };

  static String normalizeStatusText (const String& statusText) {
    StringStream output;
    for (const auto& word : split(trim(statusText), ' ')) {
      output << toProperCase(word) << " ";
    }
    return trim(output.str());
  }

  String getStatusText (int code) {
    if (statuses.contains(code)) {
      return statuses.at(code).text;
    }

    return "";
  }

  int getStatusCode (const String& text) {
    for (const auto& entry : statuses) {
      if (text == entry.second.text) {
        return entry.second.code;
      }

      if (toLowerCase(text) == toLowerCase(entry.second.text)) {
        return entry.second.code;
      }
    }
    return 0;
  }

  const Map<int, Status>& getStatusMap () {
    return statuses;
  }

  Status::Status (int code, const String& text)
    : code(code),
      text(text)
  {}

  Status::Status (int code)
    : code(code),
      text(getStatusText(code))
  {}

  Status::Status (const String& text)
    : code(getStatusCode(text)),
      text(text)
  {}

  String Status::str () const {
    StringStream output;
    output << this->code << " " << normalizeStatusText(this->text);
    return trim(output.str());
  }

  JSON::Object Status::json () const {
    return JSON::Object::Entries {
      {"code", this->code},
      {"text", normalizeStatusText(this->text)}
    };
  }
}
