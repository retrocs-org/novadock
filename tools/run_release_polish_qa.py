#!/usr/bin/env python3
"""Repeat native display, keyboard, installer and real-time soak release checks.

All volumes are disposable clones. A successful soak requires both guest and
host elapsed time to reach the requested duration. Never mounts a user disk.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_identity
import build_provenance
import release


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--previous", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check", choices=("ui", "installer", "soak"), required=True)
    parser.add_argument("--variant", choices=("RTG", "LARGE", "PLANAR"), default="RTG")
    parser.add_argument("--hidden", action="store_true", help="hide the host emulator window; guest UI still renders")
    parser.add_argument("--soak-seconds", type=int, default=1800)
    args = parser.parse_args()
    if not 20 <= args.soak_seconds <= 3600:
        parser.error("Soak duration must be 20..3600 seconds; release validation requires 1800.")
    if args.check == "installer" and not args.previous:
        parser.error("Installer checks require the previous release archive.")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    env = release.credentials.build_environment(release.config(release.default_config()))
    version, digest = build_identity.sync(check=True)
    hashes = {name: sha(ROOT / name) for name in build_provenance.native_programs()}
    report = {"build_id": version + "-" + digest[:12], "candidate_sha256": sha(args.candidate),
              "check": args.check, "variant": args.variant, "hidden": args.hidden, "status": "running", "native_programs": hashes}
    def save():
        (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    save()
    qa_sources = ("tests/native_qa_ui.h", "tests/native_release_soak.c", "tests/native_release_ui.c",
                  "tools/run_native_fixture.py", "tools/run_release_polish_qa.py")
    snapshot = out / "qa-source"
    snapshot.mkdir()
    for name in qa_sources:
        shutil.copy2(ROOT / name, snapshot / Path(name).name)
    (out / "qa-source.json").write_text(json.dumps({name: sha(ROOT / name) for name in qa_sources}, indent=2) + "\n")
    plan = subprocess.check_output(["make", "-n", "-B", "-f", "Makefile.cross", "NovaDock"], cwd=ROOT, env=env, text=True)
    compiler = shlex.split(next(line for line in plan.splitlines() if "scripts/compile-amiga.py" in line))
    compiler = compiler[:compiler.index("--") + 1]
    def run_fixture(fixture, label, timeout, profile="a4000-rtg"):
        start = time.monotonic()
        with (out / (label + ".log")).open("w") as log:
            command = [sys.executable, "tools/run_native_fixture.py", str(fixture),
                       "--timeout", str(timeout), "--profile", profile]
            if args.hidden:
                command.append("--hidden")
            result = subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
        elapsed = time.monotonic() - start
        text = (out / (label + ".log")).read_text()
        base = Path(next(line for line in text.splitlines() if line.startswith("/private/tmp/nd-native-latest-")))
        evidence = out / label
        evidence.mkdir()
        for name in ("launch.log", "os-versions.log"):
            if (base / "AutoRun" / name).exists():
                shutil.copy2(base / "AutoRun" / name, evidence / name)
        shutil.copy2(base / "system-source.json", evidence / "system-source.json")
        for item in (base / "QATests").iterdir():
            if item.is_file() and item.suffix in (".ppm", ".csv", ".txt"):
                shutil.copy2(item, evidence / item.name)
        if result.returncode:
            raise ValueError(label + " failed; inspect " + str(evidence / "launch.log"))
        return elapsed, (evidence / "launch.log").read_text()
    try:
        if args.check == "installer":
            report["runs"] = {}
            for layout in ("standard", "custom"):
                fixture = out / (layout + "-fixture")
                with (out / (layout + "-prepare.log")).open("w") as log:
                    subprocess.run([sys.executable, "tools/prepare_release_qa.py", "--current", str(args.candidate.resolve()),
                        "--previous", str(args.previous.resolve()), "--output", str(fixture), "--layout", layout,
                        "--compiler", env.get("VC", "vc")], cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
                elapsed, log = run_fixture(fixture, layout, 900)
                report["runs"][layout] = {"host_seconds": elapsed}
        else:
            fixture = out / "fixture"
            fixture.mkdir()
            subprocess.run([env["LHA"], "-x", str(args.candidate.resolve())], cwd=fixture, stdout=subprocess.DEVNULL, check=True)
            (fixture / "NovaDock").rename(fixture / "Dock")
            for name, expected in hashes.items():
                if sha(fixture / "Dock" / name) != expected:
                    raise ValueError("Candidate contains a different binary: " + name)
            (fixture / "ISOLATED-QA-ONLY").touch()
            sources = ["tests/native_release_" + ("soak.c" if args.check == "soak" else "ui.c")]
            if args.check == "ui":
                sources += ["plugin_src/screenmode/nd_scrmode.c"]
            sources += ["src/control_client.c", "src/config.c", "src/nd_options.c"]
            command = compiler + ["-O", "-Iinclude", "-Itests", "-Iplugin_src/screenmode", "-o", str(fixture / "Regression")] + sources + ["-lamiga"]
            with (out / "compile.log").open("w") as log:
                subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
            argument = str(args.soak_seconds) if args.check == "soak" else args.variant
            (fixture / "test.json").write_text(json.dumps({"application": "QATests:Regression " + argument}))
            elapsed, log = run_fixture(fixture, args.check, args.soak_seconds + 300 if args.check == "soak" else 1000 if args.variant == "PLANAR" else 400,
                "a500-ecs" if args.variant == "PLANAR" and args.check == "ui" else "a4000-rtg")
            report["host_seconds"] = elapsed
            if args.check == "soak":
                match = re.search(r"PASS SOAK seconds=(\d+) cycles=(\d+)", log)
                if not match or int(match[1]) < args.soak_seconds or elapsed < args.soak_seconds:
                    raise ValueError("Soak did not complete the requested real-time duration.")
                report.update(guest_seconds=int(match[1]), cycles=int(match[2]), release_duration=args.soak_seconds >= 1800)
        if any(sha(ROOT / name) != value for name, value in hashes.items()) or build_identity.identity()[1] != digest:
            raise ValueError("Runtime changed during validation; rerun against the final build.")
        report["status"] = "passed"
    except Exception as error:
        report.update(status="failed", error=str(error))
        save()
        raise
    save()
    print("PASS", args.check, report["build_id"], out)


if __name__ == "__main__":
    main()
