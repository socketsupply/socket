#include <socket/extension.h>

extern "C" {
  #include <sqlite3.h>
};

#include <string>
#include <map>

using ID = std::string;
using DatabaseMap = std::map<ID, sqlite3*>;

DatabaseMap databases;

void onexec (
  sapi_context_t* context,
  sapi_ipc_message_t* message,
  const sapi_ipc_router_t* router
) {
  auto id= sapi_ipc_message_get(message, "id");
  auto query = sapi_ipc_message_get(message, "query");
  auto result = sapi_ipc_result_create(context, message);

  if (query == nullptr || std::string(query).size() == 0) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_string_create(context, "Missing 'query' in parameters")
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  if (id == nullptr) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_string_create(context, "Missing 'id' in parameters")
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  if (!databases.contains(id)) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "type",
      sapi_json_string_create(context, "NotFoundError")
    );
    sapi_json_object_set(
      err,
      "message",
      sapi_json_string_create(context, "Database not found")
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  auto db = databases[id];

  // sapi_printf(context, "query: %s", query);
  sqlite3_stmt* statement;
  if (sqlite3_prepare_v3(db, query, -1, 0, &statement, nullptr)) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_string_create(context, sqlite3_errmsg(db))
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  auto rows = sapi_json_array_create(context);
  int status = 0;
  int items = 0;
  while ((status = sqlite3_step(statement)) == SQLITE_ROW) {
    const int columns = sqlite3_data_count(statement);
    auto row = sapi_json_object_create(context);
    if (columns > 0) {
      items++;
    }

    for (int i = 0; i < columns;  ++i) {
      const char* name = sqlite3_column_name(statement, i);
      switch (sqlite3_column_type(statement, i)) {
        case SQLITE_INTEGER: {
          const int value = sqlite3_column_int(statement, i);
          sapi_json_object_set(
            row,
            name,
            sapi_json_number_create(context, value)
          );
          break;
        }

        case SQLITE_FLOAT: {
          const double value = sqlite3_column_double(statement, i);
          sapi_json_object_set(
            row,
            name,
            sapi_json_any(sapi_json_number_create(context, value))
          );
          break;
        }

        case SQLITE_TEXT: {
          const unsigned char* value = sqlite3_column_text(statement, i);
          sapi_json_object_set(
            row,
            name,
            sapi_json_any(sapi_json_string_create(context, (const char*) value))
          );
          break;
        }

        case SQLITE_BLOB: {
          // not supported
          break;
        }

        case SQLITE_NULL: {
          sapi_json_object_set(row, name, nullptr);
          break;
        }
      }
    }

    sapi_json_array_push(rows, sapi_json_any(row));
  }

  sqlite3_finalize(statement);
  if (status != SQLITE_DONE) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_any(sapi_json_string_create(context, sqlite3_errmsg(db)))
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  sapi_ipc_result_set_json_data(result, sapi_json_any(rows));
  sapi_ipc_reply(result);
}

void onopen (
  sapi_context_t* context,
  sapi_ipc_message_t* message,
  const sapi_ipc_router_t* router
) {
  auto path = sapi_ipc_message_get(message, "path");
  auto result = sapi_ipc_result_create(context, message);

  if (path == nullptr) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_any(sapi_json_string_create(context, "'path' is required"))
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    return;
  }

  sqlite3* db;
  auto id = std::to_string(sapi_rand64());

  sapi_printf(context, "Opening database '%s'", path);

  if (sqlite3_open(path, &db)) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_any(sapi_json_string_create(context, sqlite3_errmsg(db)))
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    if (sqlite3_close(db)) {
      auto err = sapi_json_object_create(context);
      sapi_json_object_set(
        err,
        "message",
        sapi_json_any(sapi_json_string_create(context, sqlite3_errmsg(db)))
      );
      sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    }
  } else {
    auto data = sapi_json_object_create(context);
    sapi_json_object_set(
      data,
      "id",
      sapi_json_any(sapi_json_string_create(context, id.c_str()))
    );

    sapi_json_object_set(
      data,
      "path",
      sapi_json_any(sapi_json_string_create(context, sqlite3_db_filename(db, nullptr)))
    );

    sapi_ipc_result_set_json_data(result, sapi_json_any(data));
    databases[id] = db;
  }

  sapi_ipc_reply(result);
}

void onclose (
  sapi_context_t* context,
  sapi_ipc_message_t* message,
  const sapi_ipc_router_t* router
) {
  auto id= sapi_ipc_message_get(message, "id");
  auto result = sapi_ipc_result_create(context, message);

  if (id == nullptr) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_any(sapi_json_string_create(context, "Missing 'id' in parameters"))
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  if (!databases.contains(id)) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "type",
      sapi_json_any(sapi_json_string_create(context, "NotFoundError"))
    );
    sapi_json_object_set(
      err,
      "message",
      sapi_json_any(sapi_json_string_create(context, "Database not found"))
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  auto db = databases[id];
  if (sqlite3_close(db)) {
    auto err = sapi_json_object_create(context);
    sapi_json_object_set(
      err,
      "message",
      sapi_json_any(sapi_json_string_create(context, sqlite3_errmsg(db)))
    );
    sapi_ipc_result_set_json_error(result, sapi_json_any(err));
    sapi_ipc_reply(result);
    return;
  }

  databases.erase(id);
  sapi_ipc_reply(result);
}

bool initialize (sapi_context_t* context, const void *data) {
  sapi_ipc_router_map(context, "sqlite3.open", onopen, data);
  sapi_ipc_router_map(context, "sqlite3.close", onclose, data);
  sapi_ipc_router_map(context, "sqlite3.exec", onexec, data);
  return true;
}

bool deinitialize (sapi_context_t* context, const void *data) {
  return true;
}

SOCKET_RUNTIME_REGISTER_EXTENSION(
  "sqlite3", // name
  initialize, // initializer
  deinitialize,
  "a simple sqlite3 binding", // description
);
