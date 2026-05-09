# Vedam

Private C implementation. Public application messages live in [vedam-interface](https://github.com/ltbhappyness/vedam-interface) and are mounted here as `proto/`.

```
proto/           submodule — vedam-interface
src/vedamd/      C daemon (dlopen only)
lib/             always-loaded shared objects
plugins/         optional shared objects
tools/codegen/   API generator (next)
include/vedam/   generated headers only
config/ cmake/ tests/
```

Product applications (chanting, later neural apps) are separate repositories.
