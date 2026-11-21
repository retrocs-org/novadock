#!/usr/bin/env python3
"""Prepare disposable native Installer QA from two verified runtime packages.

Questions/notices, logical user-level references, primitive-operation user mode and
source discovery are substituted. File,
startup, process-stop, install, upgrade and removal expressions remain intact.
Run the generated definition through AmigaMac --test; never on a normal disk.
"""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from test_installer_script import parse,tokenize,head

def layouts_for(mode):
    if mode=='standard':return {n:{'appdir':'SYS:Utilities','prefs':'SYS:Prefs','shared':'SYS:NovaDock'} for n in (0,1,2)}
    if mode=='custom':return {1:{'appdir':'SYS:QA App Drawers','prefs':'SYS:QA Pref Drawers','shared':'SYS:NovaDock'},2:{'appdir':'QATests:QA App Drawers','prefs':'QATests:QA Pref Drawers','shared':'QATests:QA Shared Data/NovaDock'}}
    raise ValueError('Unknown QA layout')

def prepare_script(source,volume,action,keep,level=0,layout=None):
    nodes=parse(tokenize(source));changes=[]
    def emit(node):
        if isinstance(node,tuple):return '"'+node[2]+'"' if node[0]=='str' else str(level) if node[2]=='@user-level' else node[2]
        name=head(node);args=node['items'][1:]
        if name=='user':changes.append('user: automatic primitive operations');return '(user 0)'
        if name=='set' and args[0][2]=='#src':changes.append('source');return '(set #src "'+volume+'")'
        if name in ('welcome','message'):changes.append(name);return '(set #qa-notice 0)'
        if name=='exit':changes.append('exit');return '(exit (quiet))'
        if name and name.startswith('ask'):
            prompt=next(x for x in args if isinstance(x,dict) and head(x)=='prompt')
            text=''.join(x[2] for x in prompt['items'][1:] if isinstance(x,tuple))
            changes.append(name+': '+text)
            if name=='askdir' and layout:
                if text.startswith('Where should the NovaDock application'):return json.dumps(layout['appdir'])
                if text.startswith('Where should NovaDock Preferences'):return json.dumps(layout['prefs'])
                if text.startswith('Choose a parent drawer for shared data'):return json.dumps(layout['shared'].rsplit('/',1)[0] if '/' in layout['shared'] else layout['shared'].split(':',1)[0]+':')
            if text.startswith('NovaDock is already installed.'):return str(action)
            if text.startswith('Remove NovaDock?'):return '1'
            if text.startswith('Also remove your NovaDock settings'):return str(keep)
            default=next(x for x in args if isinstance(x,dict) and head(x)=='default')
            return emit(default['items'][1])
        return '('+' '.join(emit(x) for x in node['items'])+')'
    return '(user 0)\n'+'\n'.join(emit(n) for n in nodes)+'\n',changes

def installed_path(relative,layout=None):
    layout=layout or layouts_for("standard")[0];app=layout["appdir"];prefs=layout["prefs"];shared=layout["shared"]
    if relative in ('NovaDock','NovaDock.info'):return app+'/'+relative
    if relative in ('NovaDockPrefs','NovaDockPrefs.info'):return prefs+'/'+relative
    if relative in ('NovaDock.guide','NovaDock.guide.info'):return 'HELP:'+relative
    if relative=='SaveConfig':return shared+'/Tools/SaveConfig'
    if relative=='novadock.cfg':return shared+'/novadock.cfg.default'
    if relative.startswith('Catalogs/'):return 'LOCALE:'+relative
    if relative.startswith(('Plugins/','Rexx/','Music/')) or relative.startswith('Icons/') and relative.endswith('.iff') or relative.startswith('Themes/') and relative.endswith('/theme.cfg'):
        return shared+'/'+relative
    return None

def package_version(drawer):
    matches=set(re.findall(rb'\$VER: NovaDock ([0-9]+(?:\.[0-9]+)+) ',(drawer/'NovaDock').read_bytes()))
    if len(matches)!=1:raise ValueError('Package must contain one unambiguous NovaDock executable version')
    result=matches.pop().decode('ascii')
    if len(result)>31:raise ValueError('Package version is too long')
    return result

def prepare(current,previous,output,layout_mode="standard"):
    layouts=layouts_for(layout_mode)
    out=output.resolve();out.mkdir(parents=True,exist_ok=False);record={'schema':2,'harness_sha256':hashlib.sha256((ROOT/'tests/native_release_check.c').read_bytes()).hexdigest(),'packages':{}};versions={}
    for name,package in [('Previous',previous),('Current',current)]:
        package=package.resolve();dest=out/name;dest.mkdir()
        subprocess.run(['lha','xw='+str(dest),str(package)],stdout=subprocess.DEVNULL,check=True)
        drawer=dest/'NovaDock'
        if not drawer.is_dir():raise ValueError('Missing NovaDock drawer in '+str(package))
        versions[name]=package_version(drawer)
        item=record['packages'][name]={'version':versions[name],'package_name':package.name,'package_sha256':hashlib.sha256(package.read_bytes()).hexdigest()}
        script=next((drawer/n for n in ('Install','Install-NovaDock') if (drawer/n).is_file()),None)
        if script is None:raise ValueError('Missing NovaDock installer in '+str(package))
        source=script.read_text('ascii');item['installer_name']=script.name;item['installer_sha256']=hashlib.sha256(source.encode()).hexdigest()
        for mode,action,keep in [('Install',0,1),('Keep',1,1),('Remove',1,0)]:
            text,changes=prepare_script(source,'QATests:'+name+'/NovaDock',action,keep)
            (out/(name+'-'+mode)).write_text(text,'ascii');item[mode]=changes
            for level,layout in layouts.items():
                variant,_=prepare_script(source,'QATests:'+name+'/NovaDock',action,keep,level,layout)
                (out/(name+'-'+mode+'-'+str(level))).write_text(variant,'ascii')
        item['logical_user_levels']=list(layouts)
        lines=[]
        for file in sorted(drawer.rglob('*')):
            if file.is_file():
                rel=file.relative_to(drawer).as_posix();target=installed_path(rel)
                if target:lines.append('QATests:'+name+'/NovaDock/'+rel+'\t'+target)
        (out/(name+'-files.tsv')).write_text('\n'.join(lines)+'\n','ascii');item['file_checks']=len(lines)
        for level,layout in layouts.items():
            mapped=[]
            for file in sorted(drawer.rglob('*')):
                if file.is_file():
                    rel=file.relative_to(drawer).as_posix();target=installed_path(rel,layout)
                    if target:mapped.append('QATests:'+name+'/NovaDock/'+rel+'\t'+target)
            (out/(name+'-'+str(level)+'-files.tsv')).write_text('\n'.join(mapped)+'\n','ascii')

    if tuple(map(int,versions['Current'].split('.')))<=tuple(map(int,versions['Previous'].split('.'))):raise ValueError('Current package must be newer than Previous')
    record.update(previous_version=versions['Previous'],current_version=versions['Current'],layout_mode=layout_mode,layouts=layouts)
    (out/'layouts.tsv').write_text(''.join(str(n)+'\t'+p['appdir']+'\t'+p['prefs']+'\t'+p['shared']+'\n' for n,p in layouts.items()),'ascii')
    (out/'versions.txt').write_text(versions['Previous']+'\n'+versions['Current']+'\n','ascii')
    (out/'inputs.json').write_text(json.dumps(record,indent=2)+'\n')
    (out/'ISOLATED-QA-ONLY').write_text('Use only with AmigaMac --test. All guest disks must be disposable.\n')
    (out/'test.json').write_text(json.dumps({'name':'NovaDock '+versions['Previous']+' to '+versions['Current']+' native Installer lifecycle ('+layout_mode+' locations)' ,'application':'QATests:NovaDockReleaseCheck','timeoutSeconds':600,'expectedReturnCode':0},indent=2)+'\n')
    print('Prepared native Installer inputs:',out)
    return record

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--current',type=Path,required=True);ap.add_argument('--previous',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--layout',choices=['standard','custom'],default='standard',help='Custom tests Average/Expert paths containing spaces');ap.add_argument('--compiler',help='Optional vc command: also build the native harness into the fresh output');args=ap.parse_args()
    prepare(args.current,args.previous,args.output,args.layout)
    if args.compiler:
        for source,program in [('tests/native_release_check.c','NovaDockReleaseCheck'),('tests/native_installer_welcome.c','InstallerWelcome')]:
            subprocess.run([sys.executable,str(ROOT/'scripts/compile-amiga.py'),'--compiler',args.compiler,'--','-O','-Iinclude','-o',str(args.output.resolve()/program),source,'-lamiga'],cwd=ROOT,check=True)
    else:print('Compile tests/native_release_check.c and tests/native_installer_welcome.c as NovaDockReleaseCheck and InstallerWelcome in this fixture before running test.json; --compiler can do this automatically.')
if __name__=='__main__':main()
