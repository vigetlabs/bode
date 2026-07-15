# bode

A small, forking HTTP/1.1 file server written in C.

`bode` listens on a TCP port, accepts connections, and serves static files
out of a document root — nothing more. There's no routing, no templating, no
CGI, no HTTPS. It reads a request line, resolves it to a file, and writes
back a response. The entire request/response path fits in a handful of
small, single-purpose source files (see [ARCHITECTURE.md](ARCHITECTURE.md)
for a tour of how they fit together).

The project's own commit history is instructive: recent commits have
hardened the server against path traversal, slow-client resource
exhaustion, integer/type bugs in option parsing, and binary-file
corruption — worth a read if you're auditing this to serve real traffic.

## Requirements

- A C compiler (tested with `cc`/`gcc`; the codebase uses POSIX APIs —
  `fork()`, `socket()`, BSD-style `sockaddr_in`, etc. — so a POSIX-ish
  environment such as Linux or macOS is expected)
- GNU Make
- `cppcheck` — only if you want to run `make lint` (see
  [Development](#development))

There are no other build-time or run-time dependencies. The one third-party
component, [antirez's `sds`](https://github.com/antirez/sds) binary-safe
string library, is vendored directly in `vendor/sds/` and built as part of
the project — no package manager or separate install step needed.

## Building

```console
$ make
```

This produces a `bode` binary in the project root. `make` compiles every
`.c` file under `src/`, plus `bode.c` (the entrypoint) and the vendored
`vendor/sds/sds.c`, and links them into `bode`.

A `make dev` target exists as well; it's the same build with `-g -Wextra
-DNDEBUG` added, useful for local debugging.

To remove build artifacts (the `bode` binary, `.o` files, and compiled test
binaries):

```console
$ make clean
```

## Usage

```console
$ ./bode --doc-root=<path> --port=<port> --conf-dir=<path>
```

All three flags are optional; each has a default:

| Flag | Default | Meaning |
|------|---------|---------|
| `--doc-root=<path>` | `root` | Directory to serve files from |
| `--port=<port>` | `8080` | TCP port to listen on |
| `--conf-dir=<path>` | `config` | Directory containing `mime.types` |

Paths are resolved relative to the current working directory unless you pass
an absolute path. Running `bode` with no arguments from the repo root will
serve whatever is in `./root` on port `8080`, using `./config/mime.types`
for MIME type lookups.

If an unrecognized flag is passed, `bode` prints a usage message to `stderr`
and exits with status `1`:

```console
$ ./bode --nope
./bode: unrecognized option '--nope'
Usage: bode --doc-root=<path> --port=<port> --conf-dir=<path>
```

### Example

```console
$ mkdir -p www
$ echo '<h1>hello from bode</h1>' > www/index.html
$ ./bode --doc-root=www --port=8888
My bode is redy on port 8888.
```

In another terminal:

```console
$ curl -i http://localhost:8888/
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 25
Connection: close

<h1>hello from bode</h1>

$ curl -i http://localhost:8888/does-not-exist
HTTP/1.1 404 NOT FOUND
Content-Type: text/plain
Content-Length: 11
Connection: close

Y U DO DIS
```

Requesting a directory path (e.g. `/`) serves `index.html` from that
directory, if present. Anything else that isn't a regular file or a
directory with an `index.html` results in a 404. The server only
understands `GET`: the request parser looks specifically for a line
matching `GET <target>`, so a `POST`, `HEAD`, or other request simply
fails to match and is served a 404 — there's no explicit `405 Method Not
Allowed` response.

The server runs until interrupted (`Ctrl-C` / `SIGINT`, or `kill`); there is
no built-in shutdown command.

## Configuring MIME types

`bode` looks for a `mime.types` file inside the directory named by
`--conf-dir` (default: `config/`). The repo ships a comprehensive
Apache-style `mime.types` file already. The format is whitespace-separated:
a MIME type, followed by one or more file extensions that map to it, with
`#`-prefixed lines treated as comments:

```
text/html    html htm
image/png    png
```

If `bode` can't open the configured `mime.types` file, it prints an error
to `stderr` naming the path and the underlying `errno` reason, and exits
with status `1` — it will not silently start with no MIME types loaded.

If a requested file's extension isn't found in the loaded MIME types table,
`bode` falls back to serving it as `application/octet-stream`.

## Security notes

This is a small, mostly single-purpose project, but a few behaviors are
worth calling out if you're evaluating it for anything beyond local/trusted
use:

- **Path traversal is blocked.** Every resolved request path is passed
  through `realpath()` and checked that it still falls inside the
  canonicalized document root before being served. A request like
  `GET /../../etc/passwd` will not escape the document root.
- **Slow clients are bounded.** Each accepted connection gets a 10-second
  receive/send timeout (`SO_RCVTIMEO`/`SO_SNDTIMEO`), and the server will
  stop reading (and cap memory use) after 64 KiB of buffered request data,
  regardless of the timeout. This defends against a client holding a
  forked worker open indefinitely (a "slowloris"-style attack) or streaming
  an unbounded request body.
- **One process per connection.** `bode` calls `fork()` for every accepted
  connection and lets the child handle the full request/response cycle
  before exiting. There is no connection pooling, keep-alive, or worker
  reuse — every response includes `Connection: close`.
- **No independent security audit.** This is a small learning/reference
  project, not a production-hardened server. Treat it accordingly if you're
  considering exposing it beyond a local or trusted network.

## Development

```console
$ make test      # build and run the unit test suite under ASan/UBSan
$ make lint       # run cppcheck (requires cppcheck to be installed)
$ make analyze    # run clang's built-in static analyzer (no extra install)
```

Unit tests live in `tests/*_tests.c` and use a vendored, single-header
`minunit`-style framework (`tests/minunit.h`). `make test` compiles each
`tests/*_tests.c` file against the library sources (everything under `src/`,
minus `bode.c`'s `main()`) with `-fsanitize=address,undefined`, then runs
every resulting binary via `tests/runtests.sh`, logging output to
`tests/tests.log`. To run the tests under Valgrind instead of relying on the
sanitizers:

```console
$ VALGRIND="valgrind --error-exitcode=1 --leak-check=full" make test
```

CI (`.github/workflows/ci.yml`) runs `make` + `make test` on every push to
`master` and every pull request; a `cppcheck` lint job also runs but is
currently advisory (`continue-on-error: true`) while some known findings
remain open.

## Project layout

See [ARCHITECTURE.md](ARCHITECTURE.md) for a description of how the pieces
in `bode.c` and `src/` fit together.

## License

No `LICENSE` file is present in this repository at the time of writing;
confirm licensing terms with the project maintainer before reuse.
