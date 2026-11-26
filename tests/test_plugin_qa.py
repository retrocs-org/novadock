"""Coverage drift, partial results and stale QA evidence must block packaging."""
import copy,json,sys,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import check_plugin_qa as q
import run_plugin_qa as runner
root=q.ROOT
cases=q.load();assert len(cases)==len(q.inventory())
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'tests').mkdir();(p/'Plugins').mkdir();(p/'Makefile.cross').write_text('Plugins/Foo: foo.c\n\tcc nd_client.c\n')
 (p/'tests/run_host_tests.sh').write_text('python3 tests/test_foo.py\n');(p/'tests/test_foo.py').write_text('assert True\n')
 c={'id':'FOO','program':'Plugins/Foo','scenario':'fixture','host_tests':['tests/test_foo.py'],'panel':False,'idle_packet_budget':20}
 def write(rows): (p/'tests/plugin_qa.json').write_text(json.dumps({'schema':1,'plugins':rows}))
 write([c]);assert q.load(p)==[c]
 for rows in ([],[c,c],[dict(c,host_tests=['tests/test_absent.py'])]):
  write(rows)
  try:q.load(p)
  except ValueError:pass
  else:raise AssertionError('Incomplete coverage accepted')
 write([c]);(p/'NovaDock').write_bytes(b'host');(p/'SupportLog').write_bytes(b'logger');(p/'Plugins/Foo').write_bytes(b'plugin')
 evidence={name:q.sha(p/name) for name in ()}
 for name in ('host.log','native.log','plugin-results.log','compile.log','directory.log'):(p/name).write_text('Passed fixture\n');evidence[name]=q.sha(p/name)
 report={'schema':1,'status':'passed','host':'passed','native':'passed','component_lookup':'passed','coverage_sha256':q.sha(p/'tests/plugin_qa.json'),'sources':q.sources(p),'binaries':q.hashes(p,[c]),'results':{'FOO':{'status':'passed'}},'evidence':evidence}
 f=p/'report.json';f.write_text(json.dumps(report));q.validate(f,p,p)
 for key,value in [('component_lookup','failed'),('host','skipped'),('native','failed'),('results',{}),('sources',{})]:
  bad=copy.deepcopy(report);bad[key]=value;f.write_text(json.dumps(bad))
  try:q.validate(f,p,p)
  except ValueError:pass
  else:raise AssertionError('Invalid evidence accepted: '+key)
 f.write_text(json.dumps(report));(p/'host.log').write_text('changed log')
 try:q.validate(f,p,p)
 except ValueError:pass
 else:raise AssertionError('Changed evidence log accepted')
 (p/'host.log').write_text('Passed fixture\n')
 f.write_text(json.dumps(report));(p/'Plugins/Foo').write_bytes(b'changed')
 try:q.validate(f,p,p)
 except ValueError:pass
 else:raise AssertionError('Changed executable accepted')
 sample='METRIC|FOO|1|1|10|20|0|99|0|0|0|3\nCASE|FOO|passed\nSUMMARY cases=1 failures=0\n'
 assert runner.native_results(sample,[c])['FOO']['metrics']['cpu_samples']==99
 for text in (sample.replace('failures=0','failures=1'),sample+sample,sample.replace('METRIC|FOO','METRIC|OTHER')):
  try:runner.native_results(text,[c])
  except ValueError:pass
  else:raise AssertionError('Incomplete/duplicate native results accepted')
print('Plugin QA gate: complete inventory/domain coverage, native results and exact binary/source/log evidence enforced.')
