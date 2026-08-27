
# clang.llvm-optrec

** LLVM Optimization Record module **

Compiles each source with `-fsave-optimization-record`, then runs
`opt-viewer.py` over the resulting YAML records.
Puts the raw `.opt.yaml` records in `bin/$profile/res` and the rendered
HTML report in `bin/$profile/res_html`.

## Prerequisites
  [maiken](https://github.com/mkn/mkn)

## Usage

```yaml
mod:
- name: clang.llvm-optrec
  link:
    bin: $str  #[optional, default="/usr/share/opt-viewer/opt-viewer.py", see: $OPT_VIEWER]
```

## Environment Variables

    Key             OPT_VIEWER
    Type            string
    Default         "/usr/share/opt-viewer/opt-viewer.py"
    Description     Path to opt-viewer.py. Overridden by the `bin` option above if set.

## Building

  *nix gcc:

    mkn clean build -tSa "-O2 -fPIC" -d -l "-pthread -ldl"

## Testing

  *nix gcc:

    mkn clean build -tSa "-O2 -fPIC" -dp test -l "-pthread -ldl" run
