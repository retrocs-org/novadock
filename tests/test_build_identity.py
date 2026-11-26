from pathlib import Path
import sys,tempfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import build_identity as b
with tempfile.TemporaryDirectory() as tmp:
 r=Path(tmp);(r/'include').mkdir();(r/'src').mkdir();(r/'VERSION').write_text('VERSION=0.6\nDATE=24.11.2026\n');(r/'src/app.c').write_text('int main(void){return 0;}\n')
 b.sync(root=r);first=b.identity(r);assert (r/b.HEADER).read_text()==b.header_text(r)
 b.sync(check=True,root=r);assert b.identity(r)==first
 (r/'tests').mkdir();(r/'tests/check.c').write_text('test');(r/'docs').mkdir();(r/'docs/report.txt').write_text('QA results');assert b.identity(r)==first
 (r/'src/app.c').write_text('int main(void){return 1;}\n');assert b.identity(r)!=first
 try:b.sync(check=True,root=r)
 except ValueError:pass
 else:raise AssertionError('Stale build ID accepted')
 b.sync(root=r);second=b.identity(r);b.sync(root=r);assert b.identity(r)==second
 (r/'Icons').mkdir();(r/'Icons/icon.iff').write_bytes(b'new art');assert b.identity(r)!=second
print('Build identity: deterministic, no self-hash cycle, ignores QA/docs, detects runtime source and artwork changes.')
