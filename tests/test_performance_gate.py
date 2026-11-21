from pathlib import Path
import importlib.util
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('performance_check',ROOT/'tools/performance_check.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
rows='\n'.join('PERF,%s,%s,%d'%(side,key,100) for side in ['Before','After'] for key in m.BUDGETS)
assert all(x['passed'] for x in m.compare(rows))
for metric,(ratio,slack) in m.BUDGETS.items():
 bad=rows.replace('After,%s,100'%metric,'After,%s,%d'%(metric,int(100*ratio+slack)+1))
 assert not all(x['passed'] for x in m.compare(bad)),metric
for bad in [rows+'\nPERF,Before,idle_wakes,100',rows.replace('Before,startup_ticks,100','Before,startup_ticks,-1'),rows.replace('PERF,Before,startup_ticks,100','')]:
 try:m.compare(bad)
 except (ValueError,KeyError):pass
 else:raise AssertionError('Invalid/incomplete performance report accepted')
print('Native performance budgets reject wakeup/latency/memory regressions and incomplete reports.')
