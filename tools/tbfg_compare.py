"""Compara una implementación de FixedMath contra los casos de referencia de tbfg.dll.

Uso:
    python tools/tbfg_compare.py tests/golden/fixedmath_golden.tsv -- <comando del CLI> [args...]

Protocolo del CLI (ver docs/CODEX_TAREA_FixedMath.md):
    stdin:  una línea por caso: metodo<TAB>arg1<TAB>arg2...
    stdout: exactamente una línea por caso con el resultado codificado.

Sale con código 0 solo si todos los casos coinciden.
"""
import argparse
import collections
import subprocess
import sys


def load_cases(path):
    cases = []
    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip("\r\n")
            if not line:
                continue
            left, sep, expected = line.partition("\t=>\t")
            if not sep:
                sys.exit(f"{path}:{n}: línea sin '=>'")
            cases.append((left.split("\t"), expected))
    return cases


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("golden")
    ap.add_argument("--show", type=int, default=3, help="fallos a mostrar por método")
    ap.add_argument("--method", help="probar solo este método")
    ap.add_argument("--timeout", type=int, default=120)
    argv = sys.argv[1:]
    split = argv.index("--") if "--" in argv else len(argv)
    args = ap.parse_args(argv[:split])
    cli = argv[split + 1:]
    if not cli:
        ap.error("falta el comando del CLI después de --")

    cases = load_cases(args.golden)
    if args.method:
        cases = [c for c in cases if c[0][0] == args.method]
    stdin = "".join("\t".join(inp) + "\n" for inp, _ in cases)
    try:
        proc = subprocess.run(cli, input=stdin, capture_output=True, text=True,
                              encoding="utf-8", timeout=args.timeout)
    except subprocess.TimeoutExpired:
        sys.exit(f"El CLI no terminó en {args.timeout} s")
    got = proc.stdout.splitlines()
    if proc.returncode != 0:
        print(f"AVISO: el CLI salió con código {proc.returncode}")
        print(proc.stderr[-2000:])
    if len(got) != len(cases):
        print(f"AVISO: {len(cases)} casos enviados, {len(got)} líneas recibidas")

    total = collections.Counter()
    fails = collections.defaultdict(list)
    for i, (inp, expected) in enumerate(cases):
        method = inp[0]
        total[method] += 1
        actual = got[i] if i < len(got) else "<sin salida>"
        if actual != expected:
            fails[method].append((inp[1:], expected, actual))

    width = max(len(m) for m in total)
    n_fail = sum(len(v) for v in fails.values())
    for method in total:
        f = fails.get(method, [])
        status = "OK  " if not f else "FALLA"
        print(f"{status} {method:<{width}} {total[method] - len(f):>5}/{total[method]}")
        for inp, expected, actual in f[:args.show]:
            print(f"        {method}({', '.join(inp)})  esperado={expected}  obtenido={actual}")
    print(f"\nTotal: {len(cases) - n_fail}/{len(cases)} casos correctos, "
          f"{len(fails)} métodos con fallos")
    sys.exit(0 if n_fail == 0 and len(got) == len(cases) else 1)


if __name__ == "__main__":
    main()
