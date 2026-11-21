from pathlib import Path
import tempfile,subprocess,os
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='novadock-agenda-pictures-') as tmp:
 p=Path(tmp);cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),'-I'+str(ROOT/'plugin_src/agenda'),'-I'+str(ROOT/'plugin_src/pictures'),str(ROOT/'tests/test_agenda_pictures.c'),str(ROOT/'plugin_src/agenda/nd_agenda.c'),str(ROOT/'plugin_src/pictures/nd_pictures.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],cwd=p,check=True)
