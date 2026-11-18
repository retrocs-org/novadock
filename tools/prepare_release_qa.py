#!/usr/bin/env python3
"""Prepare disposable native Installer QA from two verified runtime packages.

Only questions/notices and package-source discovery are substituted. File,
startup, process-stop, install, upgrade and removal expressions remain intact.
Run the generated definition through AmigaMac --test; never on a normal disk.
"""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from test_installer_script import parse,tokenize,head

def prepare_script(source,volume,action,keep):
    nodes=parse(tokenize(source));changes=[]
    def emit(node):
        if isinstance(node,tuple):return '"'+node[2]+'"' if node[0]=='str' else node[2]
        name=head(node);args=node['items'][1:]
        if name=='set' and args[0][2]=='#src':changes.append('source');return '(set #src "'+volume+'")'
        if name in ('welcome','message'):changes.append(name);return '(set #qa-notice 0)'
        if name=='exit':changes.append('exit');return '(exit (quiet))'
        if name and name.startswith('ask'):
            prompt=next(x for x in args if isinstance(x,dict) and head(x)=='prompt')
            text=''.join(x[2] for x in prompt['items'][1:] if isinstance(x,tuple))
            changes.append(name+': '+text)
            if text.startswith('NovaDock is already installed.'):return str(action)
            if text.startswith('Remove NovaDock?'):return '1'
            if text.startswith('Also remove your NovaDock settings'):return str(keep)
            default=next(x for x in args if isinstance(x,dict) and head(x)=='default')
            return emit(default['items'][1])
        return '('+' '.join(emit(x) for x in node['items'])+')'
    return '(user 0)\n'+'\n'.join(emit(n) for n in nodes)+'\n',changes

def installed_path(relative):
    if relative in ('NovaDock','NovaDock.info'):return 'SYS:Utilities/'+relative
    if relative in ('NovaDockPrefs','NovaDockPrefs.info'):return 'SYS:Prefs/'+relative
    if relative in ('NovaDock.guide','NovaDock.guide.info'):return 'HELP:'+relative
    if relative=='SaveConfig':return 'SYS:NovaDock/Tools/SaveConfig'
    if relative=='novadock.cfg':return 'SYS:NovaDock/novadock.cfg.default'
    if relative.startswith('Catalogs/'):return 'LOCALE:'+relative
    if relative.startswith(('Plugins/','Rexx/','Music/')) or relative.startswith('Icons/') and relative.endswith('.iff') or relative.startswith('Themes/') and relative.endswith('/theme.cfg'):
        return 'SYS:NovaDock/'+relative
    return None

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--current',type=Path,required=True);ap.add_argument('--previous',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);record={}
    for name,package in [('Release03',args.previous),('Release04',args.current)]:
        package=package.resolve();dest=out/name;dest.mkdir()
        subprocess.run(['lha','xw='+str(dest),str(package)],stdout=subprocess.DEVNULL,check=True)
        drawer=dest/'NovaDock';assert drawer.is_dir()
        record[name]={'package_sha256':hashlib.sha256(package.read_bytes()).hexdigest()}
        source=(drawer/'Install-NovaDock').read_text('ascii');record[name]['installer_sha256']=hashlib.sha256(source.encode()).hexdigest()
        for mode,action,keep in [('Install',0,1),('Keep',1,1),('Remove',1,0)]:
            text,changes=prepare_script(source,'QATests:'+name+'/NovaDock',action,keep)
            (out/(name+'-'+mode)).write_text(text,'ascii');record[name][mode]=changes
        lines=[]
        for file in sorted(drawer.rglob('*')):
            if file.is_file():
                rel=file.relative_to(drawer).as_posix();target=installed_path(rel)
                if target:lines.append('QATests:'+name+'/NovaDock/'+rel+'\t'+target)
        (out/(name+'-files.tsv')).write_text('\n'.join(lines)+'\n','ascii');record[name]['file_checks']=len(lines)
    (out/'inputs.json').write_text(json.dumps(record,indent=2)+'\n')
    (out/'ISOLATED-QA-ONLY').write_text('Use only with AmigaMac --test. All guest disks must be disposable.\n')
    (out/'test.json').write_text(json.dumps({'name':'NovaDock 0.3 to 0.4 native Installer lifecycle','application':'QATests:NovaDockReleaseCheck','timeoutSeconds':300,'expectedReturnCode':0},indent=2)+'\n')
    print('Prepared native Installer inputs:',out)
if __name__=='__main__':main()
