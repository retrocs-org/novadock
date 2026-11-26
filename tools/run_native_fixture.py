#!/usr/bin/env python3
"""Run an offline fixture in a disposable clone of the newest imported AmigaOS.

The selected installation is checked by its Workbench library version, rather
than trusting a bundle directory name. No persistent guest drive is mounted.
"""
from pathlib import Path
import argparse, fcntl, json, os, re, shutil, subprocess, tempfile, time

ROOT=Path(__file__).resolve().parents[1]

def version(path):
    match=re.search(rb"workbench\.library (\d+)\.(\d+)",path.read_bytes())
    if not match: raise ValueError("Cannot identify Workbench library: "+str(path))
    return tuple(map(int,match.groups()))

def latest_system():
    imports=ROOT.parent/"AmigaMac/imports/workbench"
    choices=[p for p in imports.glob("workbench-*") if (p/"S/Startup-Sequence").is_file()]
    if not choices: raise ValueError("No complete imported Workbench installation found")
    return max(choices,key=lambda p:tuple(map(int,re.findall(r"\d+",p.name))))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture",type=Path)
    parser.add_argument("--system",type=Path)
    parser.add_argument("--resources",type=Path,default=Path("/Applications/Amiga Workbench.app/Contents/Resources"))
    parser.add_argument("--timeout",type=int,default=180)
    parser.add_argument("--profile",choices=("a4000-rtg","a500-ecs"),default="a4000-rtg")
    parser.add_argument("--hidden",action="store_true",help="hide the host emulator window while preserving guest rendering")
    parser.add_argument("--network",action="store_true",help="enable host-socket emulation for explicit online QA")
    args=parser.parse_args()
    fixture=args.fixture.resolve();system=(args.system or latest_system()).resolve()
    wb=version(system/"Libs/workbench.library")
    if wb<(47,42):raise ValueError("Native QA requires AmigaOS 3.2.3 / Workbench 47.42 or newer; found "+str(wb))
    definition=json.loads((fixture/"test.json").read_text())
    base=Path(tempfile.mkdtemp(prefix="nd-native-latest-",dir="/private/tmp"))
    shutil.copytree(system,base/"System");shutil.copytree(fixture,base/"QATests")
    auto=base/"AutoRun";auto.mkdir()
    versions=["Version","Version exec.library FULL","Version workbench.library FULL","Version intuition.library FULL","Version gadtools.library FULL","Version layers.library FULL","Version graphics.library FULL","Version C:SetPatch FULL"]
    script="FailAt 32767\n"+"\n".join(cmd+(" >" if i==0 else " >>")+"AutoRun:os-versions.log" for i,cmd in enumerate(versions))+"\n"
    script+=definition["application"]+" >AutoRun:launch.log <NIL:\nEcho >AutoRun:result.txt $RC\n"
    (auto/"Startup").write_text(script)
    startup=base/"System/S/Startup-Sequence";text=startup.read_text("latin1")
    if "AutoRun:Startup" not in text:
        if "LoadWB\n" not in text:raise ValueError("Cannot place fixture hook after LoadWB")
        text=text.replace("LoadWB\n","LoadWB\nIf EXISTS AutoRun:Startup\n  Execute AutoRun:Startup\nEndIf\n",1)
        startup.write_text(text,"latin1")
    (base/"System/S/Network-Startup").write_text("; Offline native QA\n")
    (base/"System/S/User-Startup").write_text("; Disposable native QA\n")
    for p in (base/"System/WBStartup").iterdir():
        if p.is_file():p.unlink()
    rom=args.resources/("Kickstarts/A4000.3.2.3.47.115.rom" if args.profile=="a4000-rtg" else "Kickstarts/CDTVA500A600A2000.3.2.3.47.115.rom")
    if not rom.is_file():raise ValueError("Missing 3.2.3 A4000 ROM")
    config="""[fs-uae]
amiga_model = A4000
cpu = 68060
chip_memory = 2048
zorro_iii_memory = 16384
motherboard_ram = 8192
graphics_card = uaegfx
fullscreen = 0
window_width = 1024
window_height = 600
mouse_integration = 1
joystick_port_1 = none
network_card = 0
bsdsocket_library = 0
sound_output = none
title = NovaDock QA - AmigaOS 3.2.3
uae_cpu_24bit_addressing = false
uae_cpu_speed = max
"""
    if args.profile=="a500-ecs":
        config=config.replace("amiga_model = A4000","amiga_model = A500+").replace("cpu = 68060","cpu = 68000").replace("zorro_iii_memory = 16384","fast_memory = 8192").replace("motherboard_ram = 8192","motherboard_ram = 0").replace("graphics_card = uaegfx","graphics_card = none").replace("uae_cpu_24bit_addressing = false","uae_cpu_24bit_addressing = true")
    if args.hidden:
        config=config.replace("mouse_integration = 1","mouse_integration = 0")
        config+="window_hidden = 1\nautomatic_input_grab = 0\n"
    if args.network:config=config.replace("bsdsocket_library = 0","bsdsocket_library = 1")
    if args.profile=="a500-ecs":
        # UI/instruction compatibility check, not a cycle-exact speed benchmark.
        config+="uae_cpu_cycle_exact = false\nuae_cpu_compatible = false\n"
    config+="base_dir = "+str(base/"FS-UAE")+"\nkickstart_file = "+str(rom)+"\n"
    for n,name in enumerate(["System","QATests","AutoRun"]):config+="hard_drive_%d = %s\nhard_drive_%d_label = %s\n"%(n,base/name,n,name)
    (base/"test.fs-uae").write_text(config)
    (base/"system-source.json").write_text(json.dumps({"source":str(system),"workbench_library":list(wb),"rom":str(rom),"offline":not args.network,"profile":args.profile,"hidden":args.hidden},indent=2)+"\n")
    print(base,flush=True);print("Selected",system,"Workbench %d.%d"%wb,flush=True)
    result=None;failure=None
    child_env=dict(os.environ)
    if args.hidden:
        # SDL otherwise promotes the hidden Cocoa app to the foreground.
        child_env["SDL_MAC_BACKGROUND_APP"]="1"
    with (base/"console.log").open("wb") as log:
        proc=subprocess.Popen([str(args.resources/"Emulator.app/Contents/MacOS/fs-uae"),str(base/"test.fs-uae"),"--logs-dir="+str(base/"logs")],stdout=log,stderr=subprocess.STDOUT,env=child_env)
        try:
            deadline=time.monotonic()+args.timeout
            while time.monotonic()<deadline:
                output=auto/"result.txt"
                if output.exists() and output.read_text().strip():result=int(output.read_text().strip());break
                if proc.poll() is not None:failure="Emulator exited before the fixture reported a result";break
                time.sleep(1)
            else:failure="Fixture timed out"
        finally:
            proc.terminate()
            try:proc.wait(timeout=5)
            except subprocess.TimeoutExpired:proc.kill();proc.wait()
    os_log=auto/"os-versions.log"
    runtime=re.search(r"(?m)^workbench\.library (\d+)\.(\d+)",os_log.read_text("latin1") if os_log.exists() else "")
    if not runtime or tuple(map(int,runtime.groups()))<(47,42):failure="Running Workbench library did not verify as 47.42 or newer"
    for name in ["os-versions.log","launch.log"]:
        path=auto/name
        if path.exists():print(name+":\n"+path.read_text("latin1")[-10000:])
    if failure:print(failure);return 1
    print("Result",result)
    return 0 if result==definition.get("expectedReturnCode",0) else 1

if __name__=="__main__":
    # SDL/Cocoa quit events can affect concurrent instances of the bundled
    # emulator. Serialize native QA, including callers in other QA tools.
    with (Path(tempfile.gettempdir())/("novadock-native-qa-"+str(os.getuid())+".lock")).open("a") as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        raise SystemExit(main())
