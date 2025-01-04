#ifndef SOCKET_RUNTIME_BYTES_H
#define SOCKET_RUNTIME_BYTES_H

#include "platform.hh"

namespace ssc::runtime::bytes {
  // forward
  class Buffer;
  class BufferQueue;
  class ArrayBuffer;

  template <size_t size>
  using ByteArray = Array<uint8_t, size>;

  /**
   * Encodes input as a string of hex characters.
   * @param input The input string to encode
   * @return An encoded string value
   */
  String encodeHexString (const String& input);
  String encodeHexString (const Vector<uint8_t>& input);

  /**
   * Decodes input as a string of hex characters to a normal string.
   * @param input The input string to encode
   * @return An encoded string value
   */
  String decodeHexString (const String& input);

  /**
   * Converts a `uint64_t` to a array of `uint8_t` values (bytes)
   * @param input The `uint64_t` to convert to an array of bytes
   * @return An array of `uint8_t` values
   */
  const ByteArray<8> toByteArray (const uint64_t input);
  const ByteArray<4> toByteArray (const uint32_t input);
  const ByteArray<2> toByteArray (const uint16_t input);

  size_t base64_encode_length (size_t inputSize);
  size_t base64_decode_length (size_t inputSize);

  size_t base64_encode (
    const unsigned char* input,
    size_t inputSize,
    char *output,
    size_t outputSize
  );

  size_t base64_decode (
    const char *input,
    size_t inputSize,
    unsigned char *output,
    size_t outputSize
  );

  namespace base64 {
    String encode (const Vector<uint8_t>&);
    String encode (const String&);
    String decode (const String&);
  }

  class ArrayBuffer {
    public:
      using Pointer = SharedPointer<unsigned char[]>;
      mutable Mutex mutex;
      Pointer bytes = nullptr;
      Atomic<size_t> byteOffset = 0;
      Atomic<size_t> byteLength = 0;

      ArrayBuffer () = default;
      ArrayBuffer (size_t);
      ArrayBuffer (size_t, const Pointer);
      ArrayBuffer (size_t, size_t, const Pointer);
      ArrayBuffer (size_t, size_t, const unsigned char*);
      ArrayBuffer (const ArrayBuffer&);
      ArrayBuffer (ArrayBuffer&&);
      virtual ~ArrayBuffer ();

      ArrayBuffer& operator = (const ArrayBuffer&);
      ArrayBuffer& operator = (ArrayBuffer&&);
      ArrayBuffer& operator = (std::nullptr_t);

      size_t size () const;
      const unsigned char* data () const;
      unsigned char* data ();
      const ArrayBuffer slice (size_t, size_t = -1) const;
      void resize (size_t);
      const Pointer pointer () const;
      Pointer pointer ();

      template <typename T>
      SharedPointer<T[]> as () {
        return SharedPointer<T[]>(
          this->bytes,
          reinterpret_cast<T*>(this->bytes.get())
        );
      }
  };

  class Buffer {
    public:
      static Buffer empty ();
      template <size_t size>
      static Buffer from (const ByteArray<size>&);
      static Buffer from (const Vector<uint8_t>&);;
      static Buffer from (const String&);
      static Buffer from (const Buffer&);
      static Buffer from (const ArrayBuffer&);
      static Buffer from (const unsigned char*, size_t);
      static Buffer from (const char*, size_t);

      enum class Encoding {
        UTF8,
        HEX,
        BASE64
      };

      ArrayBuffer buffer;
      Atomic<size_t> byteOffset = 0;
      Atomic<size_t> byteLength = 0;

      Buffer () = default;
      Buffer (const ArrayBuffer&);
      Buffer (size_t, size_t, const ArrayBuffer&);
      Buffer (const Buffer&);
      Buffer (Buffer&&);
      Buffer (const String&);
      Buffer (size_t);
      virtual ~Buffer ();

      Buffer& operator = (const ArrayBuffer&);
      Buffer& operator = (const BufferQueue&);
      Buffer& operator = (BufferQueue&&);
      Buffer& operator = (const Buffer&);
      Buffer& operator = (Buffer&&);
      Buffer& operator = (const String&);

      size_t size () const;

      template <size_t size>
      bool set (const ByteArray<size>&, size_t);
      bool set (const Vector<uint8_t>&, size_t);
      bool set (const String&, size_t);
      bool set (const Buffer&, size_t);
      bool set (const ArrayBuffer&, size_t);
      bool set (const unsigned char*, size_t, size_t);
      bool set (const char*, size_t, size_t);

      unsigned char at (size_t) const;
      const unsigned char* data () const;
      unsigned char* data ();
      const Buffer slice (size_t, size_t = -1, bool = false) const;
      String str (const Encoding = Encoding::UTF8) const;

      const ArrayBuffer::Pointer pointer () const;
      ArrayBuffer::Pointer pointer ();
  };

  class BufferQueue : public Buffer {
    public:
      using Buffer::Buffer;
      BufferQueue (const BufferQueue&);
      BufferQueue (BufferQueue&&);

      BufferQueue& operator = (const ArrayBuffer&);
      BufferQueue& operator = (const BufferQueue&);
      BufferQueue& operator = (BufferQueue&&);
      BufferQueue& operator = (const Buffer&);
      BufferQueue& operator = (Buffer&&);

      template <size_t size>
      bool push (const ByteArray<size>&);
      bool push (const Vector<uint8_t>&);
      bool push (const String&);
      bool push (const Buffer&);
      bool push (const ArrayBuffer&);
      bool push (const unsigned char*, size_t);
      bool push (const char*, size_t);
      bool push (SharedPointer<unsigned char[]>, size_t);
      bool push (SharedPointer<char[]>, size_t);
      template <size_t size>
      bool reset (const ByteArray<size>&);
      bool reset (const Vector<uint8_t>&);
      bool reset (const String&);
      bool reset (const Buffer&);
      bool reset (const ArrayBuffer&);
      bool reset (const unsigned char*, size_t);
      bool reset (const char*, size_t);
      bool reset (SharedPointer<unsigned char[]>, size_t);
      bool reset (SharedPointer<char[]>, size_t);
      bool reset ();
  };
}
#endif
