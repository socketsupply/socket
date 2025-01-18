#include "../bytes.hh"
#include "../debug.hh"

namespace ssc::runtime::bytes {
  ArrayBuffer::ArrayBuffer (size_type byteLength)
    : byteLength(byteLength),
      byteOffset(0),
      bytes(byteLength > 0 ? new unsigned char[byteLength]{0} : nullptr)
  {}

  ArrayBuffer::ArrayBuffer (
    size_type byteLength,
    const SharedPointer bytes
  ) : byteLength(byteLength),
      byteOffset(0),
      bytes(bytes)
  {}

  ArrayBuffer::ArrayBuffer (
    size_type byteLength,
    size_type byteOffset,
    const SharedPointer bytes
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

  const ArrayBuffer::SharedPointer ArrayBuffer::shared () const {
    return this->bytes;
  }

  ArrayBuffer::SharedPointer ArrayBuffer::shared () {
    return this->bytes;
  }

  const ArrayBuffer ArrayBuffer::slice (size_type begin, size_type end) const {
    Lock lock(this->mutex);
    if (begin < 0) {
      begin = this->size() + begin;
    }

    if (end < 0) {
      end = this->size() + end;
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

  void ArrayBuffer::resize (size_type size) {
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

  template <Buffer::size_type size>
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

  Buffer Buffer::from (const unsigned char* input, size_type size) {
    auto buffer = Buffer(size);
    buffer.set(input, 0, size);
    return buffer;
  }

  Buffer Buffer::from (const char* input, size_type size) {
    auto buffer = Buffer(size);
    buffer.set(input, 0, size);
    return buffer;
  }

  Buffer Buffer::concat (const Vector<Buffer>& buffers) {
    auto buffer = Buffer(0);
    for (const auto& entry : buffers) {
      auto tmp = Buffer(buffer.byteLength + entry.byteLength);
      tmp.set(buffer, 0);
      tmp.set(entry, buffer.byteLength);
      buffer = tmp;
    }
    return buffer;
  }

  int Buffer::compare (const Buffer& left, const Buffer& right) {
    if (&left == &right) {
      return 0;
    }

    auto x = left.size();
    auto y = right.size();

    for (size_t i = 0, l = std::min(x, y); i < l; ++i) {
      if (left[i] != right[i]) {
        x = left[i];
        y = right[i];
        break;
      }
    }

    if (x < y) {
      return -1;
    } else if (y < x) {
      return 1;
    }

    return 0;
  }

  bool Buffer::equals (const Buffer& left, const Buffer& right) {
    return Buffer::compare(left, right) == 0;
  }

  Buffer::Buffer (const ArrayBuffer& buffer)
    : byteLength(buffer.byteLength.load(std::memory_order_relaxed)),
      byteOffset(0),
      buffer(buffer)
  {}

  Buffer::Buffer (
    size_type byteLength,
    size_type byteOffset,
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

  Buffer::Buffer (size_type size)
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

  unsigned char Buffer::operator [] (size_type byteOffset) const {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }

    if (byteOffset >= this->byteLength) {
      throw Error("Buffer::operator[]: RangeError: 'byteOffset' exceeds 'byteLength'");
    }

    return this->at(byteOffset);
  }

  unsigned char& Buffer::operator [] (size_type byteOffset) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }

    if (byteOffset >= this->byteLength) {
      throw Error("Buffer::operator[]: RangeError: 'byteOffset' exceeds 'byteLength'");
    }

    return (this->buffer.bytes.get() + this->byteOffset)[byteOffset];
  }

  Buffer Buffer::operator + (const Buffer& buffer) {
    return Buffer::concat({ *this ,buffer });
  }

  Buffer Buffer::operator + (size_type byteOffset) {
    return this->slice(byteOffset);
  }

  Buffer Buffer::operator - (size_type byteOffset) {
    return this->slice(this->byteLength - byteOffset);
  }

  bool Buffer::operator == (const Buffer& buffer) {
    return this == &buffer || Buffer::compare(*this, buffer) == 0;
  }

  bool Buffer::operator != (const Buffer& buffer) {
    return !this->operator==(buffer);
  }

  size_t Buffer::size () const {
    return this->byteLength.load(std::memory_order_relaxed);
  }

  template <Buffer::size_type size>
  bool Buffer::set (const ByteArray<size>& input, size_type byteOffset) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }
    if (byteOffset + size > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const Vector<uint8_t>& input, size_type byteOffset) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const String& input, size_type byteOffset) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const Buffer& input, size_type byteOffset) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const ArrayBuffer& input, size_type byteOffset) {
    if (byteOffset + input.size() > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input.data(), input.size());
    return true;
  }

  bool Buffer::set (const unsigned char* input, size_type byteOffset, size_type byteLength) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }
    if (byteOffset + byteLength > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input, byteLength);
    return true;
  }

  bool Buffer::set (const char* input, size_type byteOffset, size_type byteLength) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }
    if (byteOffset + byteLength > this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' and 'byteOffset' exceeds 'byteLength'");
    }
    memcpy(this->buffer.data() + byteOffset, input, byteLength);
    return true;
  }

  bool Buffer::set (unsigned char byte, size_type byteOffset) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }

    if (byteOffset >= this->byteLength) {
      throw Error("Buffer::at: RangeError: 'byteOffset' exceeds 'byteLength'");
    }

    this->data()[byteOffset] = byte;
    return true;
  }

  bool Buffer::fill (unsigned char byte, size_type byteOffset, size_type endOffset) {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }

    if (endOffset < 0) {
      endOffset = this->byteLength + endOffset;
    }

    if (byteOffset >= this->byteLength) {
      throw Error("Buffer::at: RangeError: 'byteOffset' exceeds 'byteLength'");
    } else if (endOffset >= this->byteLength) {
      throw Error("Buffer::at: RangeError: 'endOffset' exceeds 'byteLength'");
    } else if (endOffset < byteOffset) {
      throw Error("Buffer::at: RangeError: 'endOffset' must exceed 'byteOffset'");
    }

    for (int i = byteOffset; i < this->byteLength; ++i) {
      this->data()[i] = 0;
    }

    return true;
  }

  bool Buffer::contains (unsigned char value, size_type byteOffset) const {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }

    if (byteOffset >= this->byteLength) {
      return false;
    }

    for (int i = byteOffset; i < this->byteLength; ++i) {
      if (this->data()[i] == value) {
        return true;
      }
    }

    return false;
  }

  Buffer::size_type Buffer::find (unsigned char value, size_type byteOffset) const {
    if (byteOffset < 0) {
      byteOffset = this->byteLength + byteOffset;
    }

    if (byteOffset >= this->byteLength) {
      return Buffer::npos;
    }

    for (Buffer::size_type i = byteOffset; i < this->byteLength; ++i) {
      if (this->data()[i] == value) {
        return i;
      }
    }

    return Buffer::npos;
  }

  unsigned char Buffer::at (size_type size) {
    if (size >= this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' exceeds 'byteLength'");
    }

    const auto data = this->data();
    if (data == nullptr) {
      return 0;
    }

    return data[size];
  }

  unsigned char Buffer::at (size_type size) const {
    if (size >= this->byteLength) {
      throw Error("Buffer::at: RangeError: 'size' exceeds 'byteLength'");
    }

    const auto data = this->data();
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

  const Buffer Buffer::slice (size_type begin, size_type end, bool copy) const {
    if (begin < 0) {
      begin = this->size() + begin;
    }

    if (end < 0) {
      end = this->size() + end;
    }

    if (begin >= this->byteLength) {
      throw Error("Buffer::slice: RangeError: 'begin' exceeds 'byteLength'");
    }

    if (end > this->byteLength) {
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
    const auto string = reinterpret_cast<const char*>(this->data());

    if (string == nullptr) {
      return "";
    }

    if (encoding == Encoding::HEX) {
      return encodeHexString(String(string, this->byteLength));
    } else if (encoding == Encoding::BASE64) {
      return base64::encode(String(string, this->byteLength));
    }

    return String(string, this->byteLength);
  }

  const ArrayBuffer::SharedPointer Buffer::shared () const {
    return ArrayBuffer::SharedPointer(
      this->buffer.shared(),
      this->buffer.shared().get() + this->byteOffset
    );
  }

  ArrayBuffer::SharedPointer Buffer::shared () {
    return ArrayBuffer::SharedPointer(
      this->buffer.shared(),
      this->buffer.shared().get() + this->byteOffset
    );
  }

  const Buffer::const_iterator Buffer::begin () const noexcept {
    return this->data();
  }

  const Buffer::const_iterator Buffer::end () const noexcept {
    return this->data() + this->size();
  }

  Buffer::iterator Buffer::begin () noexcept {
    return this->data();
  }

  Buffer::iterator Buffer::end () noexcept {
    return this->data() + this->size();
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

  template <Buffer::size_type size>
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

  bool BufferQueue::push (const unsigned char* input, size_type size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - size, size);
  }

  bool BufferQueue::push (SharedPointer<unsigned char[]> input, size_type size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input.get(), this->byteLength - size, size);
  }

  bool BufferQueue::push (const char* input, size_type size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input, this->byteLength - size, size);
  }

  bool BufferQueue::push (SharedPointer<char[]> input, size_type size) {
    this->buffer.resize(this->byteLength + size);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(input.get(), this->byteLength - size, size);
  }

  bool BufferQueue::push (const unsigned char byte) {
    this->buffer.resize(this->byteLength + 1);
    this->byteLength = this->buffer.byteLength.load(std::memory_order_relaxed);
    return this->set(byte, this->byteLength - 1);
  }

  template <BufferQueue::size_type size>
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

  bool BufferQueue::reset (const unsigned char* input, size_type size) {
    this->buffer.resize(size);
    return this->set(input, 0, size);
  }

  bool BufferQueue::reset (SharedPointer<unsigned char[]> input, size_type size) {
    this->buffer.resize(size);
    return this->set(input.get(), 0, size);
  }

  bool BufferQueue::reset (const char* input, size_type size) {
    this->buffer.resize(size);
    return this->set(input, 0, size);
  }

  bool BufferQueue::reset (SharedPointer<char[]> input, size_type size) {
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
