#include "../bytes.hh"

namespace ssc::runtime::bytes {
  ArrayBuffer::ArrayBuffer (size_t byteLength)
    : byteLength(byteLength),
      byteOffset(0),
      bytes(new unsigned char[byteLength]{0})
  {}

  ArrayBuffer::ArrayBuffer (
    size_t byteLength,
    const Pointer bytes
  ) : byteLength(byteLength),
      byteOffset(0),
      bytes(bytes)
  {}

  ArrayBuffer::ArrayBuffer (
    size_t byteLength,
    size_t byteOffset,
    const Pointer bytes
  ) : byteLength(byteLength),
      byteOffset(byteOffset),
      bytes(bytes)
  {}

  ArrayBuffer::ArrayBuffer (const ArrayBuffer& arrayBuffer) {
    Lock lock(arrayBuffer.mutex);
    this->byteLength = arrayBuffer.byteLength.load();
    this->byteOffset = arrayBuffer.byteOffset.load();
    this->bytes = arrayBuffer.bytes;
  }

  ArrayBuffer::ArrayBuffer (ArrayBuffer&& arrayBuffer) {
    Lock lock(arrayBuffer.mutex);
    this->byteLength = arrayBuffer.byteLength.load();
    this->byteOffset = arrayBuffer.byteOffset.load();
    this->bytes = std::move(arrayBuffer.bytes);
    arrayBuffer.byteLength = 0;
    arrayBuffer.byteOffset = 0;
    arrayBuffer.bytes = nullptr;
  }

  ArrayBuffer::~ArrayBuffer () {}

  ArrayBuffer& ArrayBuffer::operator = (const ArrayBuffer& arrayBuffer) {
    if (this == &arrayBuffer) {
      return *this;
    }

    ScopedLock lock(this->mutex, arrayBuffer.mutex);
    this->byteLength = arrayBuffer.byteLength.load();
    this->byteOffset = arrayBuffer.byteOffset.load();
    this->bytes = arrayBuffer.bytes;
    return *this;
  }

  ArrayBuffer& ArrayBuffer::operator = (ArrayBuffer&& arrayBuffer) {
    ScopedLock lock(this->mutex, arrayBuffer.mutex);
    this->byteLength = arrayBuffer.byteLength.load();
    this->byteOffset = arrayBuffer.byteOffset.load();
    this->bytes = std::move(arrayBuffer.bytes);
    arrayBuffer.byteLength = 0;
    arrayBuffer.byteOffset = 0;
    arrayBuffer.bytes = nullptr;
    return *this;
  }

  ArrayBuffer& ArrayBuffer::operator = (std::nullptr_t) {
    this->resize(0);
    return *this;
  }

  size_t ArrayBuffer::size () const {
    return this->byteLength;
  }

  const unsigned char* ArrayBuffer::data () const {
    return this->bytes.get() + this->byteOffset;
  }

  unsigned char* ArrayBuffer::data () {
    return this->bytes.get() + this->byteOffset;
  }

  const ArrayBuffer::Pointer ArrayBuffer::pointer () const {
    return this->bytes;
  }

  ArrayBuffer::Pointer ArrayBuffer::pointer () {
    return this->bytes;
  }

  ArrayBuffer ArrayBuffer::slice (size_t begin, size_t end) const {
    Lock lock(this->mutex);
    if (begin < 0) {
      begin = this->size() -1 + begin;
    }

    if (end < 0) {
      end = this->size() -1 + end;
    }

    if (end < begin) {
      end = begin;
    }

    if (begin > end) {
      begin = end;
    }

    return ArrayBuffer(
      end - begin,
      this->byteOffset + begin,
      this->bytes
    );
  }

  void ArrayBuffer::resize (size_t size) {
    if (size < 0) {
      size = this->size() -1 + size;
    }

    if (size == 0) {
      this->byteOffset = 0;
      this->byteLength = 0;
      this->bytes = nullptr;
    } else if (size > this->byteLength) {
      if (this->bytes) {
        const auto bytes = std::make_shared<unsigned char[]>(size);
        memset(bytes.get(), 0, size);
        memcpy(
          bytes.get(),
          this->bytes.get() + this->byteOffset,
          this->byteLength
        );

        this->byteLength = size;
        this->byteOffset = 0;
        this->bytes = bytes;
      } else {
        this->byteLength = size;
        this->byteOffset = 0;
        this->bytes = std::make_shared<unsigned char[]>(size);
      }
    } else if (size < this->byteLength) {
      this->byteLength = size;
    }
  }

  Buffer Buffer::empty () {
    return Buffer(0);
  }

  Buffer::Buffer (const ArrayBuffer& buffer)
    : byteLength(buffer.byteLength.load(std::memory_order_relaxed)),
      byteOffset(0),
      buffer(buffer)
  {}

  Buffer::Buffer (
    size_t byteLength,
    size_t byteOffset,
    const ArrayBuffer& buffer
  ) : byteLength(byteLength),
      byteOffset(byteOffset),
      buffer(buffer)
  {}

  Buffer::Buffer (const Buffer& buffer)
    : byteLength(buffer.byteLength.load()),
      byteOffset(buffer.byteOffset.load()),
      buffer(buffer.buffer)
  {}

  Buffer::Buffer (Buffer&& buffer)
    : byteLength(buffer.byteLength.load()),
      byteOffset(buffer.byteOffset.load()),
      buffer(buffer.buffer)
  {
    buffer.byteOffset = 0;
    buffer.byteLength = 0;
    buffer.buffer = nullptr;
  }

  Buffer::Buffer (const String& string)
    : byteLength(string.size()),
      byteOffset(0)
  {
    this->buffer.resize(string.size());
    memcpy(this->buffer.data(), string.c_str(), string.size());
  }

  Buffer::Buffer (size_t size)
    : byteLength(size),
      byteOffset(0),
      buffer(size)
  {}

  Buffer::~Buffer () {
  }

  Buffer& Buffer::operator = (const Buffer&) {
    return *this;
  }

  Buffer& Buffer::operator = (Buffer&&) {
    return *this;
  }

  size_t Buffer::size () const {
    return this->byteLength.load(std::memory_order_relaxed);
  }

  template <size_t size>
  bool Buffer::set (const ByteArray<size>& input, size_t byteOffset) {
    if (byteOffset + size > this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }

    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const Vector<uint8_t>& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const String& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const Buffer& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const ArrayBuffer& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const unsigned char* input, size_t byteOffset, size_t byteLength) {
    if (byteOffset + byteLength > this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input, byteLength);
    return true;
  }

  bool Buffer::set (const char* input, size_t byteOffset, size_t byteLength) {
    if (byteOffset + byteLength > this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input, byteLength);
    return true;
  }

  unsigned char Buffer::at (size_t size) const {
    if (size >= this->byteLength) {
      throw new Error("Buffer::at: RangeError: 'size' exceeds 'byteLength'");
    }

    const auto data = this->buffer.data();
    if (data == nullptr) {
      return 0;
    }

    return data[size];
  }

  const unsigned char* Buffer::data () const {
    return this->buffer.data() + this->byteOffset;
  }

  unsigned char* Buffer::data () {
    return this->buffer.data() + this->byteOffset;
  }

  Buffer Buffer::slice (size_t begin, size_t end, bool copy) const {
    if (begin < 0) {
      begin = this->size() -1 + begin;
    }

    if (end < 0) {
      end = this->size() -1 + end;
    }

    if (begin >= this->byteLength) {
      throw new Error("Buffer::slice: RangeError: 'begin' exceeds 'byteLength'");
    }

    if (end >= this->byteLength) {
      throw new Error("Buffer::slice: RangeError: 'end' exceeds 'byteLength'");
    }

    if (begin > end) {
      throw new Error("Buffer::slice: RangeError: 'end' cannot be less than 'begin'");
    }

    if (!copy) {
      return Buffer(
        end - begin,
        this->byteOffset + begin,
        this->buffer
      );
    }

    auto slice = this->buffer.slice(
      this->byteOffset + begin,
      this->byteOffset + begin + end
    );

    auto buffer = ArrayBuffer(slice.size());
    memcpy(buffer.data(), slice.data(), slice.size());
    return buffer;
  }

  String Buffer::str (const Encoding encoding) const {
    const auto string = reinterpret_cast<const char*>(this->buffer.data());
    if (encoding == Encoding::HEX) {
      return encodeHexString(string);
    } else if (encoding == Encoding::BASE64) {
      return base64::encode(string);
    }
    return string;
  }

  template <size_t size>
  bool BufferQueue::push (const ByteArray<size>& input) {
    this->buffer.resize(this->byteLength + size);
    if (this->set(input, this->byteLength)) {
      this->byteLength += size;
      return true;
    }
    return false;
  }

  bool BufferQueue::push (const Vector<uint8_t>& input) {
    this->buffer.resize(this->byteLength + input.size());
    if (this->set(input, this->byteLength)) {
      this->byteLength += input.size();
      return true;
    }
    return false;
  }

  bool BufferQueue::push (const String& input) {
    this->buffer.resize(this->byteLength + input.size());
    return this->set(input, this->byteLength);
  }

  bool BufferQueue::push (const Buffer& input) {
    this->buffer.resize(this->byteLength + input.size());
    if (this->set(input, this->byteLength)) {
      this->byteLength += input.size();
      return true;
    }
    return false;
  }

  bool BufferQueue::push (const ArrayBuffer& input) {
    this->buffer.resize(this->byteLength + input.size());
    if (this->set(input, this->byteLength)) {
      this->byteLength += input.size();
      return true;
    }
    return false;
  }

  bool BufferQueue::push (const unsigned char* input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    if (this->set(input, this->byteLength, size)) {
      this->byteLength += size;
      return true;
    }
    return false;
  }

  bool BufferQueue::push (SharedPointer<unsigned char[]> input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    if (this->set(input.get(), this->byteLength, size)) {
      this->byteLength += size;
      return true;
    }
    return false;
  }

  bool BufferQueue::push (const char* input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    if (this->set(input, this->byteLength, size)) {
      this->byteLength += size;
      return true;
    }
    return false;
  }

  bool BufferQueue::push (SharedPointer<char[]> input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    if (this->set(input.get(), this->byteLength, size)) {
      this->byteLength += size;
      return true;
    }
    return false;
  }

  template <size_t size>
  bool BufferQueue::reset (const ByteArray<size>& input) {
    this->buffer.resize(size);
    this->set(input.get(), 0, size);
    return true;
  }

  bool BufferQueue::reset (const Vector<uint8_t>& input) {
    this->buffer.resize(input.size());
    this->set(input, 0);
    return true;
  }

  bool BufferQueue::reset (const String& input) {
    this->buffer.resize(input.size());
    this->set(input, 0);
    return true;
  }

  bool BufferQueue::reset (const Buffer& input) {
    this->buffer.resize(input.size());
    this->set(input, 0);
    return true;
  }

  bool BufferQueue::reset (const ArrayBuffer& input) {
    this->buffer.resize(input.size());
    this->set(input, 0);
    return true;
  }

  bool BufferQueue::reset (const unsigned char* input, size_t size) {
    this->buffer.resize(size);
    this->set(input, 0, size);
    return true;
  }

  bool BufferQueue::reset (SharedPointer<unsigned char[]> input, size_t size) {
    this->buffer.resize(size);
    this->set(input.get(), 0, size);
    return true;
  }

  bool BufferQueue::reset (const char* input, size_t size) {
    this->buffer.resize(size);
    this->set(input, 0, size);
    return true;
  }

  bool BufferQueue::reset (SharedPointer<char[]> input, size_t size) {
    this->buffer.resize(size);
    this->set(input.get(), 0, size);
    return true;
  }

  bool BufferQueue::reset () {
    this->buffer.resize(0);
    this->byteOffset = 0;
    this->byteLength = 0;
    return true;
  }
}
