#!/usr/bin/env python3
"""Exercise the actual request builder/parser against a loopback HA mock."""
import ctypes as C
import http.server
import json
import os
from pathlib import Path
import subprocess
import tempfile
import sys
import threading
ROOT=Path(__file__).resolve().parents[1]
class Connection(C.Structure):
    _fields_=[('url',C.c_char*160),('poll',C.c_uint),('allow_http',C.c_int)]
class Entity(C.Structure):
    _fields_=[('id',C.c_char*96),('name',C.c_char*64),('state',C.c_char*40),('unit',C.c_char*16),('device_class',C.c_char*24),('updated',C.c_char*40)]
TOKEN='test-only-token-for-local-fixture'
current='off';level=0;received=[]
class Mock(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args): pass
    def send(self,status,body):
        payload=json.dumps(body).encode();self.send_response(status);self.send_header('Content-Length',str(len(payload)));self.end_headers();self.wfile.write(payload)
    def do_GET(self):
        received.append((self.command,self.path,self.headers.get('Authorization')))
        if self.headers.get('Authorization')!='Bearer '+TOKEN: return self.send(401,{'message':'unauthorized'})
        if self.path.endswith('/sensor.missing'):return self.send(404,{'message':'not found'})
        if self.path.endswith('/sensor.unavailable'):return self.send(200,{'entity_id':'sensor.unavailable','state':'unavailable','attributes':{}})
        return self.send(200,{'entity_id':'light.desk','state':current,'attributes':{'friendly_name':'Desk light','supported_color_modes':['brightness'],'brightness':round(level*255/100)},'last_updated':'2026-11-17T12:00:00Z'})
    def do_POST(self):
        global current,level
        received.append((self.command,self.path,self.headers.get('Authorization')))
        if self.headers.get('Authorization')!='Bearer '+TOKEN:return self.send(401,{'message':'unauthorized'})
        body=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        if self.path=='/api/services/light/turn_on':
            assert body['entity_id']=='light.desk' and set(body)=={'entity_id','brightness_pct'}
            level=body['brightness_pct'];assert isinstance(level,int) and 0<=level<=100
            current='on' if level else 'off'
        else:
            assert body=={'entity_id':'light.desk'} and self.path=='/api/services/homeassistant/toggle'
            current='on' if current=='off' else 'off';level=100 if current=='on' else 0
        return self.send(200,[])
with tempfile.TemporaryDirectory() as temporary:
    tmp=Path(temporary);libpath=tmp/('hass.dylib' if sys.platform=='darwin' else 'hass.so')
    subprocess.run([os.environ.get('CC','cc'),'-std=c99',*(['-dynamiclib'] if sys.platform=='darwin' else ['-shared','-fPIC']),'-Wno-deprecated-declarations','-I'+str(ROOT/'plugin_src/hass'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(libpath)],check=True)
    lib=C.CDLL(str(libpath));lib.amihass_request.argtypes=[C.POINTER(Connection),C.c_char_p,C.c_char_p,C.c_char_p,C.c_void_p,C.c_uint];lib.amihass_state.argtypes=[C.c_char_p,C.POINTER(Entity)]
    server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Mock);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    connection=Connection(('http://127.0.0.1:'+str(server.server_port)).encode(),30,1)
    lib.amihass_control_request.argtypes=[C.POINTER(Connection),C.c_char_p,C.c_char_p,C.c_char_p,C.c_void_p,C.c_uint]
    def request(path,entity=None,token=TOKEN,action=None):
        out=C.create_string_buffer(1700)
        if action:
            assert lib.amihass_control_request(C.byref(connection),token.encode(),entity.encode(),action.encode(),out,len(out))
        else:
            assert lib.amihass_request(C.byref(connection),token.encode(),path.encode(),entity.encode() if entity else None,out,len(out))
        config=tmp/'request.conf';config.write_bytes(out.value);response=tmp/'response'
        command=['curl','--disable','--config',str(config),'--silent','--fail','--connect-timeout','2','--max-time','3','--max-filesize','524288','--proto','=http,https','--output',str(response)]
        assert TOKEN not in ' '.join(command)
        result=subprocess.run(command,capture_output=True)
        assert TOKEN.encode() not in result.stdout+result.stderr
        config.unlink()
        return result,response.read_bytes() if response.exists() else b''
    try:
        result,data=request('/api/states/light.desk');assert result.returncode==0;e=Entity();assert lib.amihass_state(data,C.byref(e)) and e.state==b'off'
        result,data=request('/api/services/homeassistant/toggle','light.desk');assert result.returncode==0 and data==b'[]'
        result,data=request('/api/states/light.desk');assert result.returncode==0 and lib.amihass_state(data,C.byref(e)) and e.state==b'on'
        for percent in (25,100,0):
            result,data=request('',entity='light.desk',action='Brightness '+str(percent));assert result.returncode==0
            result,data=request('/api/states/light.desk');assert result.returncode==0 and lib.amihass_state(data,C.byref(e))
            assert e.state==(b'on' if percent else b'off') and abs(json.loads(data)['attributes']['brightness']*100/255-percent)<1
        result,_=request('/api/states/light.desk',token='wrong-test-only-token-value');assert result.returncode==22
        result,_=request('/api/states/sensor.missing');assert result.returncode==22
        result,data=request('/api/states/sensor.unavailable');assert result.returncode==0 and lib.amihass_state(data,C.byref(e));lib.amihass_available.argtypes=[C.POINTER(Entity)];assert not lib.amihass_available(C.byref(e))
        assert all(auth.startswith('Bearer ') for _,_,auth in received)
    finally:server.shutdown();server.server_close();thread.join()
    result,_=request('/api/states/light.desk');assert result.returncode!=0
print('HASS loopback authentication, toggle/brightness readback, unavailable, 401/404 and connection-failure tests passed.')
