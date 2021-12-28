#include "lib/uv/include/uv.h"
#include "lib/utp/include/utp.h"

typedef struct opkit_utp_connection_t {
  uint64_t connectionId;
  uint32_t min_recv_packet_size;
  uint32_t recv_packet_size;

  struct utp_iovec send_buffer[256];
  struct utp_iovec *send_buffer_next;
  uint32_t send_buffer_missing;

  utp_socket *socket;
  void* env;
  void* ctx;
  uv_buf_t buf;
  // void* on_read;
  // void* on_drain;
  // void* on_end;
  void (*on_error)(opkit_utp_connection_t*) {};
  // void* on_close;
  // void* on_connect;
  void* realloc;
} opkit_utp_connection_t;

typedef struct {
  uv_udp_t handle;
  utp_context *utp;
  uint32_t accept_connections;
  opkit_utp_connection_t *next_connection;
  uv_timer_t timer;
  void* env;
  void* ctx;
  uv_buf_t buf;
  // void* on_message;
  // void* on_send;
  // void* on_connection;
  // void* on_close;
  void* realloc;
  int pending_close;
  int closing;
} opkit_utp_t;

typedef struct {
  uv_udp_send_t req;
  void* ctx;
} utp_send_request_t;

static uint64 on_utp_state_change (utp_callback_arguments *a) {
  opkit_utp_connection_t *self = (opkit_utp_connection_t*) utp_get_userdata(a->socket);
  return 0;
}

static uint64 on_utp_read (utp_callback_arguments *a) {
  opkit_utp_connection_t *self = (opkit_utp_connection_t *) utp_get_userdata(a->socket);
  return 0;
}

static uint64 on_utp_sendto (utp_callback_arguments *a) {
  opkit_utp_t *self = (opkit_utp_t *) utp_context_get_userdata(a->context);
  return 0;
}

static uint64 on_utp_firewall (utp_callback_arguments *a) {
  opkit_utp_t *self = (opkit_utp_t *) utp_context_get_userdata(a->context);
  return self->accept_connections ? 0 : 1;
}

static uint64 on_utp_accept (utp_callback_arguments *a) {
  opkit_utp_t *self = (opkit_utp_t *) utp_context_get_userdata(a->context);
  return 0;
}

static uint64 on_utp_error (utp_callback_arguments *a) {
  opkit_utp_connection_t *self = (opkit_utp_connection_t *) utp_get_userdata(a->socket);
  return 0;
}

void opkit_utp_init (opkit_utp_t* self, opkit_utp_connection_t* next, int bufSize) {
  self->closing = 0;
  self->pending_close = 2;

  self->next_connection = next;

  uv_timer_t *timer = &(self->timer);
  timer->data = self;

  struct uv_loop_s *loop = uv_default_loop();
  int err = uv_timer_init(loop, timer);

  if (err < 0) {
    // TODO emit "error"
  }

  char buf[bufSize];
  self->buf = uv_buf_init(buf, bufSize);

  uv_udp_t *handle = &(self->handle);
  handle->data = self;

  err = uv_udp_init(loop, handle);
  if (err < 0) {
    // TODO emit "error"
  }

  self->utp = utp_init(2);
  utp_context_set_userdata(self->utp, self);

  utp_set_callback(self->utp, UTP_ON_STATE_CHANGE, &on_utp_state_change);
  utp_set_callback(self->utp, UTP_ON_READ, &on_utp_read);
  utp_set_callback(self->utp, UTP_ON_FIREWALL, &on_utp_firewall);
  utp_set_callback(self->utp, UTP_ON_ACCEPT, &on_utp_accept);
  utp_set_callback(self->utp, UTP_SENDTO, &on_utp_sendto);
  utp_set_callback(self->utp, UTP_ON_ERROR, &on_utp_error);

  self->accept_connections = 0;
}

void opkit_utp_bind () {

}

void opkit_utp_local_port () {

}

void opkit_utp_send_request_init () {

}

void opkit_utp_send () {

}

void opkit_utp_close (opkit_utp_t* self) {
  self->closing = 1;
  int err;

  err = uv_timer_stop(&(self->timer));
  if (err < 0) {
    // TODO emit "error"
  }

  err = uv_udp_recv_stop(&(self->handle));
  if (err < 0) {
    // TODO emit "error"
  }

  auto on_uv_close = [] (uv_handle_t* handle) {
    opkit_utp_t *self = (opkit_utp_t *)handle->data;
    self->pending_close--;

    if (self->pending_close > 0) return;

    // TODO emit "close"
  };

  uv_close((uv_handle_t *) &(self->handle), on_uv_close);
  uv_close((uv_handle_t *) &(self->timer), on_uv_close);
}

void opkit_utp_destroy () {

}

void opkit_utp_ref () {}
void opkit_utp_unref () {}

void opkit_utp_set_ttl () {

}

void opkit_utp_send_buffer () {

}

void opkit_utp_recv_buffer () {

}

void opkit_utp_connection_init (opkit_utp_connection_t *conn, int bufSize) {
  // We need a buffer, but we don't need to register and manage a bunch
  // of callbacks because we emit "event-name" to the UI using connectionId.

  char buf[bufSize];
  conn->buf = uv_buf_init(buf, bufSize);
}

int opkit_utp_connection_drain (opkit_utp_connection_t *conn) {
  struct utp_iovec *next = conn->send_buffer_next;
  uint32_t missing = conn->send_buffer_missing;
  if (!missing) return 1;

  size_t sent_bytes = utp_writev(conn->socket, next, missing);

  if (sent_bytes < 0) {
    // TODO emit "error"
    return 0;
  }

  size_t bytes = sent_bytes;

  while (bytes > 0) {
    if (next->iov_len <= bytes) {
      bytes -= next->iov_len;
      next++;
      missing--;
    } else {
      next->iov_len -= bytes;
      next->iov_base = ((char *) next->iov_base) + bytes;
      break;
    }
  }

  conn->send_buffer_missing = missing;
  conn->send_buffer_next = next;
  return missing ? 0 : 1;
}

uint32_t opkit_utp_connection_write (opkit_utp_connection_t* conn, uv_buf_t* buf) {
  uint32_t buf_len = sizeof(&buf) / sizeof(uv_buf_t);

  conn->send_buffer_next = conn->send_buffer;
  conn->send_buffer_next->iov_base = buf;
  conn->send_buffer_next->iov_len = buf_len;
  conn->send_buffer_missing = 1;

  return opkit_utp_connection_drain(conn);
}

uint32_t opkit_utp_connection_writev (opkit_utp_connection_t* conn, uv_buf_t* bufs[]) {
  struct utp_iovec *next = conn->send_buffer_next = conn->send_buffer;
  // TODO test if bufs_len is right
  uint32_t bufs_len = sizeof(&bufs) / sizeof(uv_buf_t);

  for (int i = 0; i < bufs_len; i++) {
    uv_buf_t* buf = bufs[i];
    next->iov_base = buf;
    next->iov_len = sizeof(buf);
    next++;
  }

  conn->send_buffer_missing = bufs_len;
  return opkit_utp_connection_drain(conn);
}

void opkit_utp_connection_shutdown (opkit_utp_connection_t* conn) {
  utp_shutdown(conn->socket, SHUT_WR);
}

void opkit_utp_connection_close (opkit_utp_connection_t* conn) {
  utp_close(conn->socket);
}

void opkit_utp_connection_destroy (opkit_utp_connection_t *conn) {
  conn->env = NULL;
  conn->buf.base = NULL;
  conn->buf.len = 0;
}

int opkit_utp_connect (opkit_utp_t* self, opkit_utp_connection_t* conn, uint32_t port, char* ip) {
  int err;
  struct sockaddr_in addr;

  conn->socket = utp_create_socket(self->utp);
  utp_set_userdata(conn->socket, conn);


  err = uv_ip4_addr((char *) &ip, port, &addr);

  if (!err) {
    utp_connect(conn->socket, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
  }

  return err;
}
