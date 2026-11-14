# Vedam

C libraries and the `vedamd` daemon. Design lives in the [docs](https://github.com/ltbhappyness/docs) repository (start at [vedam.md](https://github.com/ltbhappyness/docs/blob/master/vedam.md)). The public messages are the `proto/` submodule, published as [vedam-interface](https://github.com/ltbhappyness/vedam-interface).

## Call

An application includes `vi/` only. A call is one `vi_msg_t`: object key, attribute array, and one status per attribute. `vi_system_get` and the other public verbs pass that pointer straight into the node. The same arrays come back filled. A singleton passes a null key and a zero length.

On this process that pointer is the object the checks and the library see. There is no pack step. A queue or HTTP call encodes that same `vi_msg_t` once into a buffer the connection already owns, and writes the reply back into it. The peer builds its own message from those bytes. The caller’s address is not sent.

Library bodies (`vlib_*`) return success and do not store values yet.

## Layout

```
model/                   Class and shared-type YAML, and the JSON Schema that checks them
tools/codegen/           Generator and one Jinja template per output kind
include/vi/              Public C API (installed)
include/vii/             Internal contract, including older UUID lines
include/lib/             vlib_* declarations
include/core/            Process node
include/lock/            Wait-Die wrapper
include/vti/             Transport and protobuf headers (not installed)
src/va/                  Public verbs: pointer in, vii_dispatch
src/via/                 vii_dispatch, the class table, contract checks, then vlib_*
src/lib/                 Empty vertical bodies
src/core/core.c          vedam_init / vedam_deinit
src/lock/                Object and attribute locks
src/ds/                  In-memory indexes used by the lock map
src/vta/inproc/          Local door: caller thread, no pack
src/vta/mq/              POSIX message queue
src/vta/grpc/            Temporary HTTP on TCP
src/vta/grpc_uds/        Temporary HTTP on a Unix socket
src/vta/connect/         Connect/QUIC stub, no socket
src/vedamd/              Daemon: public headers only, then wait
proto/                   vedam-interface submodule (proto/vedam/<class>/v1/<class>.proto)
examples/                Programs that include only vi/
```

Adding a vertical is a YAML file in `model/classes/` plus a row in `model/types.yaml`. The generator checks both against `model/schema/`, then writes the headers, the thin bodies, the class table row, and `proto/vedam/<name>/v1/<name>.proto`.

## Transports at init

Unset variables use the defaults in parentheses. IPC and remote addresses include the process id or an ephemeral port so two nodes on one machine do not share a name.

| Variable | Default | Values |
|----------|---------|--------|
| `VEDAM_TRANSPORT_LOCAL` | `inproc` | `inproc` |
| `VEDAM_TRANSPORT_IPC` | `mq` | `mq`, `grpc_uds` |
| `VEDAM_TRANSPORT_IPC_ADDR` | `/vedam.<pid>`, or `/tmp/vedamd.<pid>.sock` for `grpc_uds` | queue name or socket path |
| `VEDAM_TRANSPORT_REMOTE` | `grpc` | `grpc`, `connect` |
| `VEDAM_TRANSPORT_REMOTE_ADDR` | `127.0.0.1:0` | `host:port` |

`grpc` is HTTP/1.1 with a five-byte length prefix and a protobuf `Message`. libprotobuf is the codec. The gRPC C++ library is not linked. `connect` does not open a socket.

A remote client connects once. Further calls on that descriptor reuse the socket. The listen side reads the next request on the same connection.

## Codegen and build

Needs: CMake, a C11 compiler, C++17, `protoc` and `libprotobuf`, Python 3 with `PyYAML`, `Jinja2`, and `jsonschema`.

```bash
cd vedam
python3 tools/codegen/generate.py --root .
cmake -B build
cmake --build build
cmake --install build
ctest --test-dir build --output-on-failure
```

Install prefix defaults to `build/install` (`include/vi`, `libvedam`, `vedamd`). Shared library: `cmake -B build -DBUILD_SHARED_LIBS=ON`.

CMake reruns the generator when YAML or templates change. `python3 tools/codegen/generate.py --root . --check` compares the tree with a fresh render and writes nothing.

## Tests

| Test | What it checks |
|------|----------------|
| `test_ds` | In-memory hash and list indexes |
| `test_util` | Attribute helpers |
| `test_system` | Init, public get, queue call, contract rejects |
| `test_lock_transport` | Distinct objects and keys under the lock map |
| `test_grpc_read` | HTTP get against the temporary remote listener |
| `test_verticals` | UUID bytes and a get on each vertical |
| `test_codec_r8` | Local call skips the codec; queue and HTTP return mixed per-attribute status; two HTTP calls share one TCP socket |

Queue tests open a POSIX message queue. A restricted sandbox can fail that open with `EACCES`. Run them with normal filesystem permissions.

## Run

```bash
build/install/bin/vedamd -f
```

Without `-f` the process calls `daemon(3)` and then waits for SIGINT or SIGTERM. `vedam_init` has already started the local, IPC, and remote doors.

```c
#include "vi/vedam.h"
#include "vi/class/system.h"

int main(void)
{
    v_attr_t attr;
    v_status_t status;
    vi_msg_t msg;

    vedam_init();
    attr.id = VI_SYSTEM_ATTR_PROTOCOL_VERSION;
    msg.key = NULL;
    msg.key_len = 0;
    msg.attr_count = 1;
    msg.attr_list = &attr;
    msg.status_list = &status;
    vi_system_get(&msg);
    vedam_deinit();
    return 0;
}
```
