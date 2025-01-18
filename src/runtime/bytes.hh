#ifndef SOCKET_RUNTIME_BYTES_H
#define SOCKET_RUNTIME_BYTES_H

#include "platform.hh"

namespace ssc::runtime::bytes {
  // forward
  class Buffer;
  class BufferQueue;
  class ArrayBuffer;

  using size_type = signed long int; // we're just explicit here

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
    size_t inputSize,
    const char *input,
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
      using SharedPointer = SharedPointer<unsigned char[]>;
      using size_type = bytes::size_type;

      mutable Mutex mutex;
      SharedPointer bytes = nullptr;
      Atomic<size_type> byteOffset = 0;
      Atomic<size_type> byteLength = 0;

      ArrayBuffer () = default;
      ArrayBuffer (size_type);
      ArrayBuffer (size_type, const SharedPointer);
      ArrayBuffer (size_type, size_type, const SharedPointer);
      ArrayBuffer (size_type, size_type, const unsigned char*);
      ArrayBuffer (const ArrayBuffer&);
      ArrayBuffer (ArrayBuffer&&);
      virtual ~ArrayBuffer ();

      ArrayBuffer& operator = (const ArrayBuffer&);
      ArrayBuffer& operator = (ArrayBuffer&&);
      ArrayBuffer& operator = (std::nullptr_t);

      size_t size () const;
      const unsigned char* data () const;
      unsigned char* data ();
      const ArrayBuffer slice (size_type, size_type = -1) const;
      void resize (size_type);
      const SharedPointer shared () const;
      SharedPointer shared ();

      template <typename T>
      types::SharedPointer<T[]> as () {
        return types::SharedPointer<T[]>(
          this->bytes,
          reinterpret_cast<T*>(this->bytes.get())
        );
      }
  };

  class Buffer {
    public:
      // just alias `String::npos` - same effect
      static constexpr auto npos = String::npos;
      using size_type = ArrayBuffer::size_type;

      // iterator protocol/interface
      using value_type = unsigned char;
      using pointer = value_type*;
      using const_pointer = const value_type*;
      using reference = value_type&;
      using const_reference = const value_type&;
      using iterator = pointer;
      using const_iterator = const_pointer;

      static Buffer empty ();
      template <size_type size>
      static Buffer from (const ByteArray<size>&);
      static Buffer from (const Vector<uint8_t>&);;
      static Buffer from (const String&);
      static Buffer from (const Buffer&);
      static Buffer from (const ArrayBuffer&);
      static Buffer from (const unsigned char*, size_type);
      static Buffer from (const char*, size_type);
      static Buffer concat (const Vector<Buffer>&);
      static int compare (const Buffer&, const Buffer&);
      static bool equals (const Buffer&, const Buffer&);

      enum class Encoding {
        UTF8,
        HEX,
        BASE64
      };

      ArrayBuffer buffer;
      Atomic<size_type> byteOffset = 0;
      Atomic<size_type> byteLength = 0;

      Buffer () = default;
      Buffer (const ArrayBuffer&);
      Buffer (size_type, size_type, const ArrayBuffer&);
      Buffer (const Buffer&);
      Buffer (Buffer&&);
      Buffer (const String&);
      Buffer (size_type);
      virtual ~Buffer ();

      Buffer& operator = (const ArrayBuffer&);
      Buffer& operator = (const BufferQueue&);
      Buffer& operator = (BufferQueue&&);
      Buffer& operator = (const Buffer&);
      Buffer& operator = (Buffer&&);
      Buffer& operator = (const String&);

      Buffer operator + (const Buffer&);
      Buffer operator + (size_type);
      Buffer operator - (size_type);

      bool operator == (const Buffer&);
      bool operator != (const Buffer&);

      unsigned char operator [] (size_type) const;
      unsigned char& operator [] (size_type);

      size_t size () const;

      template <size_type size>
      bool set (const ByteArray<size>&, size_type = 0);
      bool set (const Vector<uint8_t>&, size_type = 0);
      bool set (const String&, size_type = 0);
      bool set (const Buffer&, size_type = 0);
      bool set (const ArrayBuffer&, size_type = 0);
      bool set (const unsigned char*, size_type, size_type = 0);
      bool set (const char*, size_type, size_type = 0);
      bool set (unsigned char, size_type = 0);

      bool fill (unsigned char, size_type = 0, size_type = -1);
      bool contains (unsigned char, size_type = 0) const;
      size_type find (unsigned char, size_type = 0) const;

      unsigned char at (size_type);
      unsigned char at (size_type) const;
      const unsigned char* data () const;
      unsigned char* data ();
      const Buffer slice (size_type = 0, size_type = -1, bool = false) const;
      String str (const Encoding = Encoding::UTF8) const;

      const ArrayBuffer::SharedPointer shared () const;
      ArrayBuffer::SharedPointer shared ();

      const const_iterator begin () const noexcept;
      const const_iterator end () const noexcept;
      iterator begin () noexcept;
      iterator end () noexcept;
  };

  class BufferQueue : public Buffer {
    public:
      using Buffer::Buffer;
      Atomic<bool> resizable = true;

      BufferQueue (const BufferQueue&);
      BufferQueue (BufferQueue&&);

      BufferQueue& operator = (const ArrayBuffer&);
      BufferQueue& operator = (const BufferQueue&);
      BufferQueue& operator = (BufferQueue&&);
      BufferQueue& operator = (const Buffer&);
      BufferQueue& operator = (Buffer&&);

      template <size_type size>
      bool push (const ByteArray<size>&);
      bool push (const Vector<uint8_t>&);
      bool push (const String&);
      bool push (const Buffer&);
      bool push (const ArrayBuffer&);
      bool push (const unsigned char);
      bool push (const unsigned char*, size_type);
      bool push (const char*, size_type);
      bool push (SharedPointer<unsigned char[]>, size_type);
      bool push (SharedPointer<char[]>, size_type);
      template <size_type size>
      bool reset (const ByteArray<size>&);
      bool reset (const Vector<uint8_t>&);
      bool reset (const String&);
      bool reset (const Buffer&);
      bool reset (const ArrayBuffer&);
      bool reset (const unsigned char*, size_type);
      bool reset (const char*, size_type);
      bool reset (SharedPointer<unsigned char[]>, size_type);
      bool reset (SharedPointer<char[]>, size_type);
      bool reset ();
  };
}
#endif
