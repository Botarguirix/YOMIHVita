"""Consulta la tbfg.dll original a través del arnés (requiere tools/harness/run/ local).

Uso como módulo:
    from oracle import ask
    ask([("div", "1", "3"), ("powu", "1.1", "i:3")])  # -> ["0.33333", "1.33101"]

Uso por línea de comandos (un caso por línea en stdin, separado por TAB):
    python tools/oracle.py < casos.tsv
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUN = os.path.join(ROOT, "tools", "harness", "run")
PROJECT = os.path.join(ROOT, "tools", "harness", "project")
USER_DIR = os.path.join(os.environ.get("APPDATA", ""), "Godot", "app_userdata", "tbfg-harness")


def ask(calls, rebuild=False):
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    from make_pck import make_pck
    pck = os.path.join(RUN, "harness.pck")
    if rebuild or not os.path.exists(pck):
        make_pck(PROJECT, pck)
    os.makedirs(USER_DIR, exist_ok=True)
    with open(os.path.join(USER_DIR, "oracle_in.tsv"), "w", encoding="utf-8") as f:
        for c in calls:
            f.write("\t".join(str(x) for x in c) + "\n")
    subprocess.run([os.path.join(RUN, "YourOnlyMoveIsHUSTLE.exe"), "--main-pack", "harness.pck",
                    "--no-window", "res://oracle.tscn"], cwd=RUN, capture_output=True, timeout=300)
    with open(os.path.join(USER_DIR, "oracle_out.tsv"), encoding="utf-8") as f:
        return f.read().splitlines()


if __name__ == "__main__":
    calls = [l.rstrip("\r\n").split("\t") for l in sys.stdin if l.strip()]
    for c, r in zip(calls, ask(calls, rebuild="--rebuild" in sys.argv)):
        print("\t".join(c), "=>", r, sep="\t")
