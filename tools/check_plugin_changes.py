#!/usr/bin/env python3
"""Require an independent version bump for changed bundled plugin inputs."""
from pathlib import Path
import argparse,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
def git_bytes(root,base,path):
    p=subprocess.run(['git','show',base+':'+path],cwd=root,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL)
    return p.stdout if p.returncode==0 else None

def audit(root,base):
    old=git_bytes(root,base,'include/nd_plugin_versions.h')
    if old is None:raise ValueError('Base tag does not contain plugin versions: '+base)
    pattern=r'#define ND_PLUGIN_VERSION_(\w+) "([^"]+)"'
    previous=dict(re.findall(pattern,old.decode('ascii')));current=dict(re.findall(pattern,(root/'include/nd_plugin_versions.h').read_text()))
    rows=[]
    for line in (root/'Makefile.cross').read_text().splitlines():
        if not line.startswith('Plugins/') or ':' not in line:continue
        program,deps=line.split(':',1);files=deps.split();key=None
        for name in files:
            if name.endswith('.c') and (root/name).is_file():
                found=re.search(r'ND_PLUGIN_VER_STRING\((\w+),',(root/name).read_text())
                if found:key=found[1];break
        if not key:raise ValueError('Cannot resolve independent version: '+program)
        bank=root/(program+'.assets')
        if bank.exists():files += [p.relative_to(root).as_posix() for p in bank.rglob('*') if p.is_file()]
        manifest=program+'.ndplugin'
        if (root/manifest).is_file():files.append(manifest)
        changes=[]
        for name in files:
            if name=='include/nd_plugin_versions.h':continue
            p=root/name
            if not p.is_file():continue
            now=p.read_bytes();before=git_bytes(root,base,name)
            if name.endswith('.ndplugin') and before is not None:
                clean=lambda b:re.sub(rb'^NOVADOCK_VERSION=.*\n?',b'',b,flags=re.M)
                now,before=clean(now),clean(before)
            if now!=before:changes.append(name)
        old_version=previous.get(key)
        needs_bump=bool(changes and old_version is not None and tuple(map(int,current[key].split('.')))<=tuple(map(int,old_version.split('.'))))
        rows.append({'id':key,'program':program,'previous':old_version,'current':current[key],'changed':changes,'needs_bump':needs_bump})
    return rows

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--base',required=True);ap.add_argument('--fix',action='store_true');args=ap.parse_args()
    rows=audit(ROOT,args.base);missing=[r for r in rows if r['needs_bump']]
    if missing and args.fix:
        from datetime import date
        p=ROOT/'include/nd_plugin_versions.h';text=p.read_text()
        for r in missing:
            numbers=list(map(int,r['previous'].split('.')));numbers[-1]+=1;version='.'.join(map(str,numbers))
            text=re.sub(r'(#define ND_PLUGIN_VERSION_'+r['id']+r' )"[^"]+"',r'\g<1>"'+version+'"',text)
            text=re.sub(r'(#define ND_PLUGIN_DATE_'+r['id']+r' )"[^"]+"',r'\g<1>"'+date.today().strftime('%d.%m.%Y')+'"',text)
            print(r['id']+': '+r['current']+' -> '+version)
        p.write_text(text);subprocess.run([sys.executable,str(ROOT/'tools/update_plugin_versions.py')],check=True)
    elif missing:
        for r in missing:print(r['id']+' changed since '+args.base+' but remains '+r['current'])
        return 1
    else:print('Independent versions cover changed inputs for '+str(len(rows))+' bundled plugins since '+args.base+'.')
    return 0
if __name__=='__main__':raise SystemExit(main())
