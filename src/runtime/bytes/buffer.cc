#include "../bytes.hh"
#include "../debug.hh"

namespace ssc::runtime::bytes {
  ArrayBuffer::ArrayBuffer (size_t byteLength)
    : byteLength(byteLength),
      byteOffset(0),
      bytes(byteLength > 0 ? new unsigned char[byteLength]{0} : nullptr)
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
    if (this == &arrayBuffer) {
      return *this;
    }

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
    return this->byteLength.load(std::memory_order_relaxed);
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

  const ArrayBuffer ArrayBuffer::slice (size_t begin, size_t end) const {
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
      size = this->size() - 1 + size;
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

  template <size_t size>
  Buffer Buffer::from (const ByteArray<size>& input) {
    auto buffer = Buffer(input.size());
    buffer.set(input, 0);
    return buffer;
  }

  Buffer Buffer::from (const Vector<uint8_t>& input) {
    auto buffer = Buffer(input.size());
    buffer.set(input, 0);
    return buffer;
  }

  Buffer Buffer::from (const String& input) {
    auto buffer = Buffer(input.size());
    buffer.set(input, 0);
    return buffer;
  }

  Buffer Buffer::from (const Buffer& input) {
    auto buffer = Buffer(input.size());
    buffer.set(input, 0);
    return buffer;
  }

  Buffer Buffer::from (const ArrayBuffer& input) {
    auto buffer = Buffer(input.size());
    buffer.set(input, 0);
    return buffer;
  }

  Buffer Buffer::from (const unsigned char* input, size_t size) {
    auto buffer = Buffer(size);
    buffer.set(input, 0, size);
    return buffer;
  }

  Buffer Buffer::from (const char* input, size_t size) {
    auto buffer = Buffer(size);
    buffer.set(input, 0, size);
    return buffer;
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

  Buffer::~Buffer () {}

  Buffer& Buffer::operator = (const ArrayBuffer& arrayBuffer) {
    this->buffer = arrayBuffer;
    this->byteLength = arrayBuffer.byteLength.load(std::memory_order_relaxed);
    this->byteOffset = arrayBuffer.byteOffset.load(std::memory_order_relaxed);
    return *this;
  }

  Buffer& Buffer::operator = (const BufferQueue& bufferQueue) {
    this->buffer = bufferQueue.buffer;
    this->byteLength = bufferQueue.byteLength.load(std::memory_order_relaxed);
    this->byteOffset = bufferQueue.byteOffset.load(std::memory_order_relaxed);
    return *this;
  }

  Buffer& Buffer::operator = (BufferQueue&& bufferQueue) {
    this->buffer = bufferQueue.buffer;
    this->byteLength = bufferQueue.byteLength.load(std::memory_order_relaxed);
    this->byteOffset = bufferQueue.byteOffset.load(std::memory_order_relaxed);
    bufferQueue.byteOffset = 0;
    bufferQueue.byteLength = 0;
    bufferQueue.buffer = nullptr;
    return *this;
  }

  Buffer& Buffer::operator = (const Buffer& buffer) {
    this->buffer = buffer.buffer;
    this->byteLength = buffer.byteLength.load(std::memory_order_relaxed);
    this->byteOffset = buffer.byteOffset.load(std::memory_order_relaxed);
    return *this;
  }

  Buffer& Buffer::operator = (Buffer&& buffer) {
    this->buffer = buffer.buffer;
    this->byteLength = buffer.byteLength.load(std::memory_order_relaxed);
    this->byteOffset = buffer.byteOffset.load(std::memory_order_relaxed);
    buffer.byteOffset = 0;
    buffer.byteLength = 0;
    buffer.buffer = nullptr;
    return *this;
  }

  Buffer& Buffer::operator = (const String& string) {
    const auto buffer = Buffer::from(string);
    this->buffer = buffer.buffer;
    this->byteOffset = 0; this->byteLength = buffer.byteLength.load(std::memory_order_relaxed);
    return *this;
  }

  size_t Buffer::size () const {
    return this->byteLength.load(std::memory_order_relaxed);
  }

  template <size_t size>
  bool Buffer::set (const ByteArray<size>& input, size_t byteOffset) {
    if (byteOffset + size > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }

    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const Vector<uint8_t>& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const String& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const Buffer& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const ArrayBuffer& input, size_t byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const unsigned char* input, size_t byteOffset, size_t byteLength) {
    if (byteOffset + byteLength > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input, byteLength);
    return true;
  }

  bool Buffer::set (const char* input, size_t byteOffset, size_t byteLength) {
    if (byteOffset + byteLength > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input, byteLength);
    return true;
  }

  unsigned char Buffer::at (size_t size) const {
    if (size >= this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' exceeds 'byteLength'");
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

  const Buffer Buffer::slice (size_t begin, size_t end, bool copy) const {
    if (begin < 0) {
      begin = this->size() -1 + begin;
    }

    if (end < 0) {
      end = this->size() -1 + end;
    }

    if (begin >= this->byteLength) {
      throw Error("Buffer::slice: RangeError: 'begin' exceeds 'byteLength'");
    }

    if (end >= this->byteLength) {
      throw Error("Buffer::slice: RangeError: 'end' exceeds 'byteLength'");
    }

    if (begin > end) {
      throw Error("Buffer::slice: RangeError: 'end' cannot be less than 'begin'");
    }

    if (!copy) {
      return Buffer(
        end - begin,
        this->byteOffset + begin,
        this->buffer
      );
    }

    const auto slice = this->buffer.slice(
      this->byteOffset + begin,
      this->byteOffset + begin + end
    );

    auto buffer = ArrayBuffer(slice.size());
    memcpy(buffer.data(), slice.data(), slice.size());
    return buffer;
  }

  String Buffer::str (const Encoding encoding) const {
    const auto string = reinterpret_cast<const char*>(this->buffer.data());

    if (string == nullptr) {
      return "";
    }

    if (encoding == Encoding::HEX) {
      return encodeHexString(string);
    } else if (encoding == Encoding::BASE64) {
      return base64::encode(string);
    }

    return string;
  }

  const ArrayBuffer::Pointer Buffer::pointer () const {
    return ArrayBuffer::Pointer(
      this->buffer.pointer(),
      this->buffer.pointer().get() + this->byteOffset
    );
  }

  ArrayBuffer::Pointer Buffer::pointer () {
    return ArrayBuffer::Pointer(
      this->buffer.pointer(),
      this->buffer.pointer().get() + this->byteOffset
    );
  }

  BufferQueue& BufferQueue::operator = (const ArrayBuffer& arrayBuffer) {
    this->buffer = arrayBuffer;
    this->byteLength = arrayBuffer.byteLength.load(std::memory_order_relaxed   );
    this->byteOffset = arrayBuffer.byteOffset.load(std::memory_order_relaxed   );
    return *this;
  }

  BufferQueue& BufferQueue::operator = (const BufferQueue& bufferQueue) {
    this->buffer = bufferQueue.buffer;
    this->byteLength = bufferQueue.byteLength.load(std::memory_order_relaxed   );
    this->byteOffset = bufferQueue.byteOffset.load(std::memory_order_relaxed   );
    return *this;
  }

  BufferQueue& BufferQueue::operator = (BufferQueue&& bufferQueue) {
    this->buffer = bufferQueue.buffer;
    this->byteLength = bufferQueue.byteLength.load(std::memory_order_relaxed   );
    this->byteOffset = bufferQueue.byteOffset.load(std::memory_order_relaxed   );
    bufferQueue.byteOffset = 0;
    bufferQueue.byteLength = 0;
    bufferQueue.buffer = nullptr;
    return *this;
  }

  BufferQueue& BufferQueue::operator = (const Buffer& buffer) {
    this->buffer = buffer.buffer;
    this->byteLength = buffer.byteLength.load(std::memory_order_relaxed   );
    this->byteOffset = buffer.byteOffset.load(std::memory_order_relaxed   );
    return *this;
  }

  BufferQueue& BufferQueue::operator = (Buffer&& buffer) {
    this->buffer = buffer.buffer;
    this->byteLength = buffer.byteLength.load(std::memory_order_relaxed   );
    this->byteOffset = buffer.byteOffset.load(std::memory_order_relaxed   );
    buffer.byteOffset = 0;
    buffer.byteLength = 0;
    buffer.buffer = nullptr;
    return *this;
  }

  BufferQueue::BufferQueue (const BufferQueue& bufferQueue) {
    this->buffer = bufferQueue.buffer;
    this->byteLength = bufferQueue.byteLength.load(std::memory_order_relaxed   );
    this->byteOffset = bufferQueue.byteOffset.load(std::memory_order_relaxed   );
  }

  BufferQueue::BufferQueue (BufferQueue&& bufferQueue) {
    this->buffer = bufferQueue.buffer;
    this->byteLength = bufferQueue.byteLength.load(std::memory_order_relaxed   );
    this->byteOffset = bufferQueue.byteOffset.load(std::memory_order_relaxed   );
    bufferQueue.byteOffset = 0;
    bufferQueue.byteLength = 0;
    bufferQueue.buffer = nullptr;
  }

  template <size_t size>
  bool BufferQueue::push (const ByteArray<size>& input) {
    this->buffer.resize(this->buffer.byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - input.size());
  }

  bool BufferQueue::push (const Vector<uint8_t>& input) {
    this->buffer.resize(this->byteLength + input.size());
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - input.size());
  }

  bool BufferQueue::push (const String& input) {
    this->buffer.resize(this->byteLength + input.size());
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - input.size());
  }

  bool BufferQueue::push (const Buffer& input) {
    this->buffer.resize(this->byteLength + input.size());
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - input.size());
  }

  bool BufferQueue::push (const ArrayBuffer& input) {
    this->buffer.resize(this->byteLength + input.size());
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - input.size());
  }

  bool BufferQueue::push (const unsigned char* input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - size, size);
  }

  bool BufferQueue::push (SharedPointer<unsigned char[]> input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input.get(), this->byteLength - size, size);
  }

  bool BufferQueue::push (const char* input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - size, size);
  }

  bool BufferQueue::push (SharedPointer<char[]> input, size_t size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input.get(), this->byteLength - size, size);
  }

  template <size_t size>
  bool BufferQueue::reset (const ByteArray<size>& input) {
    this->buffer.resize(size);
    return this->set(input.get(), 0, size);
  }

  bool BufferQueue::reset (const Vector<uint8_t>& input) {
    this->buffer.resize(input.size());
    return this->set(input, 0);
  }

  bool BufferQueue::reset (const String& input) {
    this->buffer.resize(input.size());
    return this->set(input, 0);
  }

  bool BufferQueue::reset (const Buffer& input) {
    this->buffer.resize(input.size());
    return this->set(input, 0);
  }

  bool BufferQueue::reset (const ArrayBuffer& input) {
    this->buffer.resize(input.size());
    return this->set(input, 0);
  }

  bool BufferQueue::reset (const unsigned char* input, size_t size) {
    this->buffer.resize(size);
    return this->set(input, 0, size);
  }

  bool BufferQueue::reset (SharedPointer<unsigned char[]> input, size_t size) {
    this->buffer.resize(size);
    return this->set(input.get(), 0, size);
  }

  bool BufferQueue::reset (const char* input, size_t size) {
    this->buffer.resize(size);
    return this->set(input, 0, size);
  }

  bool BufferQueue::reset (SharedPointer<char[]> input, size_t size) {
    this->buffer.resize(size);
    return this->set(input.get(), 0, size);
  }

  bool BufferQueue::reset () {
    this->buffer.resize(0);
    this->byteOffset = 0;
    this->byteLength = 0;
    return true;
  }
}
