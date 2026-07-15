# Architecture

This document describes how `bode`'s request/response path is put together.
It reflects the code as of this writing; if the source and this document
disagree, trust the source and please file an issue/PR to fix the docs.

## Overview

`bode` is a single-threaded-accept, forking HTTP server. The main loop does
exactly four things per connection:

1. Accept a TCP connection.
2. Fork a child process to handle it.
3. In the child: read the request, resolve it to a file, build and send a
   response, then exit.
4. In the parent: close its copy of the connection socket and reap any
   finished children (non-blocking), then loop back to `accept()`.

There is no thread pool, no event loop, and no connection reuse. Every
request gets a fresh process, and every response is sent with
`Connection: close`.

```text
                     ┌────────────────────┐
                     │        main()       │  bode.c
                     └──────────┬─────────┘
                                │
                 initialize_configuration()
                     (parses --doc-root, --port, --conf-dir)
                                │
                     mime_types_load(config)   ── src/mime.c
                                │
                 socket() / bind() / listen()
                                │
                 ┌──────────────┴──────────────┐
                 │      while(1) accept()       │
                 └──────────────┬──────────────┘
                                │
                    set_connection_timeouts()
                                │
                            fork()
                 ┌──────────────┴──────────────┐
             child (pid==0)              parent
                 │                            │
     fetch_request_path()          close(conn); waitpid(WNOHANG)
                 │                            │
     response_create()                  loop back to accept()
                 │
     response_output() → send()
                 │
        free/close/exit(0)
```

## Modules

Each concern lives in its own small `.c`/`.h` pair under `src/`. None of
them know about sockets — networking is confined to `bode.c`.

### `bode.c` — entrypoint and connection loop

Owns `main()`, the accept loop, and connection-level concerns:

- **`initialize_configuration()`** — parses `--doc-root`, `--port`, and
  `--conf-dir` via `getopt_long`, applying defaults from `src/config.h`
  where a flag is omitted. Returns `NULL` (causing `bode` to print a usage
  message and exit 1) if an unrecognized option is passed.
- **`set_connection_timeouts()`** — applies a 10-second `SO_RCVTIMEO` /
  `SO_SNDTIMEO` to each accepted connection so a stalled client can't hold
  a forked worker open indefinitely.
- **`fetch_request_path()`** — reads off the socket into a growable `sds`
  buffer (bounded to 64 KiB total via `MAX_REQUEST_SIZE`), extracts the
  request target from a line matching `GET %s`, joins it onto the
  configured document root, and hands the result to
  `resolve_within_root()` for canonicalization/containment checking. If the
  request line doesn't match (or the read fails), returns `NULL`, which
  downstream is treated as "serve a 404."
- **`resolve_within_root()`** — calls `realpath()` on both the document
  root and the candidate path, then checks that the candidate's canonical
  path has the document root's canonical path as a proper prefix (i.e. it
  either matches exactly or continues with `/`). This is what stops a
  request like `GET /../../etc/passwd` from escaping the document root.
  Returns `NULL` if either path fails to resolve (including a simply
  nonexistent target — which is why a bad path also results in a 404
  further down the pipeline) or if the candidate isn't contained in the
  root.

### `src/config.c` / `src/config.h` — configuration state

A small struct (`Config`) holding `document_root`, `config_dir`, and
`port`. `config_create()` allocates it and applies the compiled-in
defaults (`root`, `config`, `8080` — see `CONFIG_DEFAULT_*` in
`config.h`); `config_set()` overwrites a field by symbolic attribute
(`CONFIG_DOCUMENT_ROOT`, `CONFIG_CONFIG_DIR`, `CONFIG_PORT`). String fields
are copied into fixed `CONFIG_VALUE_MAX_LENGTH` (1024-byte) buffers via an
internal `write_value()` helper.

### `src/mime.c` / `src/mime.h` — MIME type table

Loads and queries an in-memory table mapping file extensions to MIME
content types.

- **`mime_types_load()`** opens `<conf-dir>/mime.types`, and for each
  non-comment line, treats the first whitespace-separated token as the
  MIME type and every subsequent token on that line as an extension
  mapping to it (this matches the Apache-style `mime.types` format shipped
  in `config/mime.types`). If the file can't be opened, it reports the
  path and `errno` reason to `stderr` and returns `NULL` — `bode.c`
  treats a `NULL` return here as fatal and exits.
- **`mime_types_find_content_type()`** does a linear scan over the loaded
  table for a matching extension, and falls back to
  `MIME_TYPES_DEFAULT_TYPE` (`application/octet-stream`) if nothing
  matches.
- The backing array (`mime_types->types`) grows by doubling
  (`MIME_TYPES_ALLOC_SIZE` = 32 initial slots) as needed in
  `mime_types_push()`.

### `src/file.c` / `src/file.h` — filesystem resolution

Bridges a resolved request path to an actual file to serve:

- **`file_path_for()`** — if the given path is a regular file, returns a
  copy of it as-is. If it's a directory, appends `index.html` (via
  `path_append()`, which is careful not to double up a `/` if the
  directory path already ends in one). Returns `NULL` if the path is
  neither.
- **`file_content_type_for()`** — extracts the file extension via
  `file_get_extension()` (a non-mutating `strrchr()`-based lookup that
  returns `""` for extension-less names) and looks up the MIME type for it.
- `is_file()` / `is_dir()` are thin wrappers over `stat()` checking
  `S_IFREG` / `S_IFDIR`.

### `src/header.c` / `src/header.h` — response headers

A minimal `Header { key, value }` pair type with `header_create()` /
`header_free()`. Used to build up the header list attached to a
`Response`.

### `src/response.c` / `src/response.h` — response assembly

Builds an in-memory HTTP response from a resolved file path (or lack
thereof):

- **`response_create()`** — resolves the filename via `file_path_for()`.
  If no file was found, or it can't be opened, builds a 404 response with
  a `text/plain` body reading `Y U DO DIS\n`. Otherwise builds a 200 response
  with `Content-Type` set from `file_content_type_for()` and the body read
  from the file. In both cases it also adds `Content-Length` (computed from
  the actual body length written) and `Connection: close` headers.
- **`response_bode_from_file()`** — reads the source file with `fread()`
  in `BUF_SIZE`-byte chunks into a growable `sds` buffer, tracking length
  via `sdslen()` rather than `strlen()`. This is deliberate: it lets binary
  files (containing embedded NUL bytes, e.g. images) survive intact,
  whereas a `strlen()`/`fgets()`-based approach would truncate at the
  first NUL or newline.
- **`response_output()`** — serializes the status line, headers, a blank
  line, and the body into a single `sds` buffer suitable for `send()`,
  using `sdscatlen()` (not `sdscat()`/string concatenation) for the body so
  a NUL-containing body is copied in full rather than truncated.
- Response, Status, and Header lifetimes are all explicitly freed via
  `response_free()` (which cascades into `status_free()` and
  `header_free()` for each header) — called by the child process after
  `send()` completes.

Note: the `Response` struct's body field and the functions that populate
it are named `bode` (matching the project name) rather than something like
`body` — this is intentional naming in the source, not a typo introduced by
this documentation.

### `vendor/sds/` — vendored string library

[`sds`](https://github.com/antirez/sds) (Simple Dynamic Strings) by
Salvatore Sanfilippo, vendored directly rather than pulled in as a
dependency. `bode` uses it wherever a buffer needs to grow dynamically and
must remain binary-safe (i.e., safe to hold embedded NUL bytes) —
request buffering, response body assembly, and final response
serialization. `sds` strings are plain `char *` under the hood, so they
interoperate directly with C string/socket APIs (`send()`, etc.) without
a wrapper or conversion step.

## Request lifecycle, end to end

1. `main()` accepts a connection and forks.
2. The child calls `fetch_request_path()`, which reads the raw request off
   the socket, extracts the target from a `GET <target>` line, joins it to
   the document root, and validates the result stays within the document
   root via `resolve_within_root()`. Any failure along this path
   (malformed request line, receive error/timeout, path escaping the
   root, or a target that doesn't resolve on disk) results in a `NULL`
   path.
3. `response_create()` takes that path (or `NULL`) and, via `file_path_for()`,
   either finds a servable file (regular file, or a directory's
   `index.html`) or fails to — driving whether the response is a 200 or a
   404.
4. For a 200, the file's bytes are read into the response body and its
   MIME type is looked up by extension; for a 404, a fixed `text/plain`
   body is used.
5. `response_output()` serializes the whole response (status line +
   headers + body) into one buffer, which is `send()`'d to the client.
6. The child frees everything it allocated and calls `exit(0)`.
7. The parent, having already closed its own handle to that connection
   right after forking, reaps the exited child via a non-blocking
   `waitpid(-1, NULL, WNOHANG)` call on its next loop iteration (not
   necessarily immediately after that specific child exits) and returns to
   `accept()`.

## Design tradeoffs worth knowing

- **Fork-per-connection** is simple and gives natural process-level
  isolation (a crash handling one request can't corrupt another
  in-flight request's memory), at the cost of the overhead of a `fork()`
  per request and no support for HTTP keep-alive.
- **No threading** means no shared-state synchronization is needed
  anywhere in the request path — the tradeoff made for that simplicity is
  the per-connection process cost above.
- **Fixed-format MIME parsing and config values** (whitespace-delimited
  `mime.types`, `1024`-byte config value buffers) keep the parsing code
  small, at the cost of being inflexible to alternate formats.
- **GET-only in practice.** There's no explicit HTTP method dispatch;
  `fetch_request_path()` scans specifically for a `GET %s` pattern in the
  raw request. Non-GET requests will typically fail that scan and result
  in `fetch_request_path()` returning `NULL` (served as a 404), rather
  than being explicitly rejected with a `405` or similar.
