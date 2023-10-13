#include "tests.hh"
#include "src/core/codec.hh"

namespace SSC::Tests {
  void codec (const Harness& t) {
    t.test("SSC::encodeURIComponent", [](auto t) {
      const auto encoded = SSC::encodeURIComponent(
        "a % encoded string with foo@bar.com, $100, & #tag"
      );

      t.assert(encodeURIComponent("").size() == 0, "Empty input returns empty string");
      t.assert(encoded.size() != 0, "encoded has size");
      t.equals(
        encoded,
        "a%20%25%20encoded%20string%20with%20foo%40bar%2Ecom%2C%20%24100%2C%20%26%20%23tag",
        "encoded value is correct"
      );
    });

    t.test("SSC::decodeURIComponent", [](auto t) {
      const auto decoded = SSC::decodeURIComponent(
        "a%20%25%20encoded%20string%20with%20foo%40bar%2Ecom%2C%20%24100%2C%20%26%20%23tag"
      );

      t.assert(decodeURIComponent("").size() == 0, "Empty input returns empty string");
      t.assert(decoded.size() != 0, "decoded has size");
      t.equals(
        decoded,
        "a % encoded string with foo@bar.com, $100, & #tag",
        "decoded value is correct"
      );
    });

    t.test("SSC::encodeHexString", [](auto t) {
      t.equals(
        SSC::encodeHexString("hello world"),
        "68656C6C6F20776F726C64",
        "encodes 'hello world'"
      );

      t.equals(
        SSC::encodeHexString("#F"),
        "2346",
        "encodes '\u0023\u0046'"
      );

      t.equals(
        SSC::encodeHexString("{\"foo\":\"bar\",\"biz\":{\"baz\":\"boop\"}}"),
        "7B22666F6F223A22626172222C2262697A223A7B2262617A223A22626F6F70227D7D",
        "encodes '{\"foo\":\"bar\",\"biz\":{\"baz\":\"boop\"}}'"
      );
    });

    t.test("SSC::decodeHexString", [](auto t) {
      t.equals(
        SSC::decodeHexString("68656C6C6F20776F726C64"),
        "hello world",
        "decodes '68656C6C6F20776F726C64'"
      );

      t.equals(
        SSC::decodeHexString("2346"),
        "#F",
        "decodes '2346'"
      );

      t.equals(
        SSC::decodeHexString("7B22666F6F223A22626172222C2262697A223A7B2262617A223A22626F6F70227D7D"),
        "{\"foo\":\"bar\",\"biz\":{\"baz\":\"boop\"}}",
        "decodes '7B22666F6F223A22626172222C2262697A223A7B2262617A223A22626F6F70227D7D'"
      );
    });

    t.test("SSC::decodeUTF8", [](auto t) {
      t.comment("skip: TODO(@jwerle)");
    });

    t.test("SSC::toBytes", [](auto t) {
      t.comment("skip: TODO(@jwerle)");
    });
  }
}
