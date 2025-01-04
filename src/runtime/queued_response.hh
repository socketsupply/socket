#ifndef SOCKET_RUNTIME_QUEUED_RESPONSE_H
#define SOCKET_RUNTIME_QUEUED_RESPONSE_H

#include "http.hh"
#include "crypto.hh"
#include "platform.hh"

namespace ssc::runtime {
  /**
   * QueuedResponse represents a response object that is typically
   * queued for sending back to a client. It may hold simple data or
   * pointers to SSE/streaming callback mechanisms.
   */
  struct QueuedResponse {
    /**
     * Numeric ID for this response entry in the queue.
     */
    using ID = uint64_t;

    /**
     * A callback used for an Event Stream (e.g., SSE, "Server-Sent Events").
     * Return `true` if sending succeeded, `false` if an error or stop occurred.
     *
     * Parameters:
     *  - name: event name
     *  - data: event data
     *  - finished: whether this is the final event
     */
    using EventStreamCallback = Function<bool(const char*, const unsigned char*, bool)>;

    /**
     * A callback used for chunked transfer encoding.
     * Return `true` if sending succeeded, `false` if an error or stop occurred.
     *
     * Parameters:
     *  - chunk: pointer to chunk data
     *  - size: size of this chunk
     *  - finished: whether this is the final chunk
     */
    using ChunkStreamCallback = Function<bool(const unsigned char*, size_t, bool)>;

    /**
     * Identifies this queued response uniquely.
     */
    ID id = crypto::rand64();

    /**
     * Optional time-to-live or expiration for this response, in milliseconds
     * or similar. Zero means "no expiration."
     */
    uint64_t ttl = 0;

    /**
     * Pointer to the raw response body (if not streaming).
     */
    SharedPointer<unsigned char[]> body = nullptr;

    /**
     * Length of the response body in bytes.
     */
    size_t length = 0;

    /**
     * HTTP headers sent in queued response
     */
    http::Headers headers;

    /**
     * (Optional) If this response is associated with a Worker,
     * store the Worker ID here.
     */
    String workerId = "";

    /**
     * Callback for SSE-like event stream. If set, we interpret the response
     * as an event stream. The callback is responsible for sending each event.
     */
    SharedPointer<EventStreamCallback> eventStreamCallback = nullptr;

    /**
     * Callback for chunked transfer encoding. If set, we interpret the response
     * as a chunked stream. The callback is responsible for sending each chunk.
     */
    SharedPointer<ChunkStreamCallback> chunkStreamCallback = nullptr;
  };

  /**
   * Container alias for storing multiple QueuedResponse entries by ID.
   */
  using QueuedResponses = Map<QueuedResponse::ID, QueuedResponse>;
}
#endif
