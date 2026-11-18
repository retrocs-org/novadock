#!/usr/bin/env python3
"""Execute production layout helpers using source-derived gadget coordinates."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(s,sig):
 a=s.index(sig);b=s.index('{',a);i=b+1;n=1
 while n:
  if s[i]=='"':
   i+=1
   while s[i]!='"':i+=2 if s[i]=='\\' else 1
  else:n+=(s[i]=='{')-(s[i]=='}')
  i+=1
 return s[a:i]
def calls(s,name):
 for m in re.finditer(r'\b'+name+r'\s*\(',s):
  i=m.end();start=i;n=1;args=[];quoted=False
  while n:
   c=s[i]
   if quoted:
    if c=='\\':i+=2;continue
    if c=='"':quoted=False
   elif c=='"':quoted=True
   elif c=='(':n+=1
   elif c==')':
    n-=1
    if not n:args.append(s[start:i].strip());break
   elif c==',' and n==1:args.append(s[start:i].strip());start=i+1
   i+=1
  yield args,s[i+1:i+180]
def literal(s):
 return s if re.fullmatch(r'"(?:[^"\\]|\\.)*"',s.strip()) else 'NULL'
pre=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_prefs_layout.h"
typedef unsigned long ULONG;typedef unsigned short UWORD;typedef char *STRPTR;
#define PLACETEXT_LEFT 1
#define PLACETEXT_RIGHT 2
#define PLACETEXT_ABOVE 3
#define PLACETEXT_IN 4
struct Screen {void *Font;};
struct NewGadget {int ng_LeftEdge,ng_TopEdge,ng_Width,ng_Height,ng_GadgetID,ng_Flags;char *ng_GadgetText;void *ng_VisualInfo,*ng_TextAttr;};
struct Page {struct NDPrefsLayout layout;struct Screen *screen;void *vi,*visual;int compact,focused;};
static int checks,avoid_overlap,rect_count;static int rectangles[64][4];static const char *page;
static void check(struct Page *p,struct NewGadget *g,int flags){
 int x=g->ng_LeftEdge,y=g->ng_TopEdge,w=g->ng_Width,h=g->ng_Height;
 int left=x,right=x+w;const char *label=g->ng_GadgetText;
 if(label && *label){int text=(int)strlen(label)*8;
  if(flags==PLACETEXT_LEFT)left=x-text-6;
  if(flags==PLACETEXT_RIGHT)right=x+w+text+6;
  if(flags==PLACETEXT_IN && text+8>w){fprintf(stderr,"%s button too narrow: %s %d < %d\n",page,label,w,text+8);assert(0);}
 }
 if(left<4 || right>p->layout.window_width-4 || y<12 || y+h>p->layout.window_height-4 || w<12 || h<10){
  fprintf(stderr,"%s %s: %d,%d %dx%d text %d..%d window %dx%d\n",page,label ? label:"",x,y,w,h,left,right,p->layout.window_width,p->layout.window_height);assert(0);}
 if(avoid_overlap){int j;for(j=0;j<rect_count;++j){int *r=rectangles[j];
  if(!(left>=r[2] || r[0]>=right || y>=r[3] || r[1]>=y+h)){
   fprintf(stderr,"%s overlap near %s %d,%d..%d,%d and %d,%d..%d,%d\n",page,label ? label:"",left,y,right,y+h,r[0],r[1],r[2],r[3]);assert(0);}}
  rectangles[rect_count][0]=left;rectangles[rect_count][1]=y;rectangles[rect_count][2]=right;rectangles[rect_count++][3]=y+h;
 }
 ++checks;
}
'''
# Each page uses its actual field helper; compact transforms are not duplicated.
helpers=[];cases=[]
specs=[
 ('prefs/novadockprefs.c','static int open_editor(', 'setup_gadget','prefs_general',208, [('create_button',False)]),
 ('prefs/novadockappearanceprefs.c','static int open_editor(', 'setup_gadget','prefs_appearance',None,[('create_button',False)]),
 ('prefs/novadockitemprefs.c','static int open_ui(', 'ng_init','prefs_organize',219,[('button',False)]),
 ('prefs/novadockmanagerprefs.c','static int open_manager(', 'm_field','prefs_plugins',240,[('m_button',False)]),
 ('prefs/novadockdiagnosticsprefs.c','static int open_ui(', 'd_field','prefs_diagnostics',207,[('d_button',False)]),
 ('prefs/novadockadditemprefs.c','static int open_add_ui(', 'add_ng','prefs_add',None,[('add_button',False),('add_string',False),('add_readonly_string',False)]),
 ('prefs/novadocklauncherprefs.c','static int open_ui(', 'ng_init','prefs_launcher',None,[('button',False)]),
 ('prefs/novadocklayoutprefs.c','static int open_page(', 'ng_init','prefs_layouts',175,[('button',False)]),
 ('prefs/novadockaboutprefs.c','static int open_about_page(', 'about_field','prefs_about',188,[('about_button',False)])]
for file,sig,helper,name,height,buttons in specs:
 s=(ROOT/file).read_text()
 if sig not in s:
  # Names differ between standalone pages, but the builder is unambiguous.
  found=re.findall(r'static int (open_\w+)\(',s);assert len(found)==1,(file,found);sig='static int '+found[0]+'('
 body=function(s,sig)
 hf=function(s,'static void '+helper+'(')
 hf=re.sub(r'struct (?:Prefs|AppearancePrefs|ItemPage|ManagerPage|DiagnosticsPage|AddItemPage|Editor|LayoutPage|AboutPage) \*','struct Page *',hf).replace(helper+'(',name+'(')
 helpers.append(hf)
 for focused in ([0,1] if name=='prefs_launcher' else [0]):
  rows=[]
  # Focused/nonfocused launcher branches have different field columns.
  source=body
  if name=='prefs_launcher':
   a=source.index('    if (e->focused) {');b=source.index('    } else {',a)
   source=source[a:b] if focused else source[b:]
  for args,tail in calls(source,helper):
   if len(args)<8:continue
   vals=args[2:6]
   if any(re.search(r'[A-Za-z_]',v) for v in vals):continue
   if int(vals[2])<=1 or int(vals[3])<=1:continue # CreateContext placeholder, not a gadget.
   label=literal(args[7]);flags='PLACETEXT_LEFT'
   found=re.search(r'ng\.ng_Flags\s*=\s*(PLACETEXT_\w+)',tail)
   if found:flags=found[1]
   rows.append(f'{name}(p,&g,{",".join(vals)},0,{label});check(p,&g,{flags});')
  for btn,_ in buttons:
   for args,tail in calls(source,btn):
    if len(args)<6:continue
    vals=args[1:4]
    if any(re.search(r'[A-Za-z_]',v) for v in vals):continue
    label=literal(args[5]);flag='PLACETEXT_LEFT' if 'string' in btn else 'PLACETEXT_IN'
    rows.append(f'{name}(p,&g,{",".join(vals)},16,0,{label});check(p,&g,{flag});')
  if name=='prefs_add':
   # Drawer/group fields authored relative to their type's first row.
   for top in (56,84):
    for args,_ in calls(source,'add_string'):
     vals=args[1:4]
     if len(vals)!=3 or not any('top' in v for v in vals):continue
     vals=[v.replace('top',str(top)) for v in vals]
     rows.append(f'{name}(p,&g,{",".join(vals)},16,0,{literal(args[5])});check(p,&g,PLACETEXT_LEFT);')
  if name=='prefs_appearance':h='compact ? 239:267'
  elif name=='prefs_add':h='compact ? 230:330'
  elif name=='prefs_launcher':h=f'compact ? 238:{301 if focused else 320}'
  else:h=str(height)
  compact_height=279 if name=='prefs_appearance' else 342 if name=='prefs_add' else (313 if focused else 332)
  compact=f'height-11<{compact_height}' if name in ('prefs_appearance','prefs_add','prefs_launcher') else '0'
  width=500 if name=='prefs_layouts' else 620
  overlap=1 if name in ('prefs_appearance','prefs_launcher') else 0
  cases.append(f'''avoid_overlap={overlap};rect_count=0;page="{name}{focused}";compact={compact};p->compact=compact;p->focused={focused};
 nd_prefs_layout_init(&p->layout,8,8,640,height,11,{width},{h});assert(nd_prefs_layout_fits(&p->layout,10,12));
 '''+'\n'.join(rows))
code=pre+'\n'.join(helpers)+'''\nint main(void){int height,compact;struct Page value,*p=&value;struct Screen screen;struct NewGadget g;
 memset(p,0,sizeof(*p));p->screen=&screen;
 for(height=256;height<=480;height+=224){\n'''+ '\n'.join(cases)+'''
 }
 printf("Small screens: %d source-derived Preferences gadget bounds/captions checked at 640x256 and 640x480.\\n",checks);return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'check.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'check.c'),str(ROOT/'src/prefs_layout.c'),'-o',str(p/'check')]
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)

# Run production grid/hit testing: every app remains reachable on every page.
apps=(ROOT/'plugin_src/runningapps/runningapps.c').read_text()
grid='\n'.join(function(apps,sig) for sig in ('static void tile_origin(', 'static int panel_hit(', 'static void choose_grid('))
app_code=r'''
#include <assert.h>
#include <stdio.h>
struct Font {int tf_XSize;};struct RP {struct Font *Font;};
struct Screen {int Width,Height,BarHeight;struct RP RastPort;};
struct AppPanel {int count,first,per_page,columns,rows,tile_width,tile_height;};
'''+ '\n'.join(re.findall(r'^#define APP_(?:GAP|PADDING|HEADER|TILE_W|TILE_H) .*',apps,re.M))+'\n'+grid+r'''
int main(void){struct Screen screen;struct Font font;struct AppPanel p;int h,n,w,height,i,x,y,total=0;
 font.tf_XSize=8;screen.Width=640;screen.BarHeight=11;screen.RastPort.Font=&font;
 for(h=256;h<=480;h+=224){screen.Height=h;
  for(n=0;n<=48;++n){p.count=n;choose_grid(&p,&screen,&w,&height);
   assert(w<=640 && height<=h-11 && p.per_page>0);
   for(i=0;i<n;++i){p.first=i/p.per_page*p.per_page;tile_origin(&p,i,&x,&y);
    assert(x>=0 && x+p.tile_width<w && y>=APP_HEADER);
    assert(y+p.tile_height<=height-(n>p.per_page ? 18:0));
    assert(panel_hit(&p,x+1,y+1)==i);++total;}
   assert(panel_hit(&p,0,0)==-1);
  }
 }
 printf("APPS: %d app/page positions and hit targets fit both screens.\n",total);return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'apps.c').write_text(app_code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89',str(p/'apps.c'),'-o',str(p/'apps')],check=True)
 subprocess.run([str(p/'apps')],check=True)

# Obtain all built-in schemas (including expanded sensor/music macros) from C.
schema_code=r'''
#include <stdio.h>
#include "nd_options.h"
int main(void){const struct NDOptionDef *d;int n,i;d=nd_builtin_options(&n);
 for(i=0;i<n;++i)printf("%s\t%s\t%s\t%s\t%s\n",d[i].id,d[i].key,d[i].label,d[i].type,d[i].choices);return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'schema.c').write_text(schema_code)
 subprocess.run([os.environ.get('CC','cc'),'-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'schema.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),'-o',str(p/'schema')],check=True)
 schemas={}
 for line in subprocess.check_output([str(p/'schema')],text=True).splitlines():
  plugin,key,label,kind,choices=line.split('\t');schemas.setdefault(plugin,{})[key]=(label,kind,choices)
# Installed descriptors may add or override options; include these in the audit.
for manifest in (ROOT/'Plugins').glob('*.ndplugin'):
 lines=manifest.read_text().splitlines();plugin=next(line.split('=',1)[1] for line in lines if line.startswith('NOVADOCK_ID='))
 for line in lines:
  if line.startswith('NOVADOCK_OPTION='):
   key,label,kind,choices,_=line.split('=',1)[1].split('|');schemas.setdefault(plugin,{})[key]=(label,kind,choices)
settings_checks=0
for height in (256,480):
 page_rows=min(12,(height-11-57)//24)
 for plugin,all_fields in schemas.items():
  fields=all_fields
  if plugin=='HASS':fields={key:fields[key] for key in ('LABEL','TYPE','AREA','ENTITY','ACTION','READONLY','REFRESH','DISPLAY','TIMESTAMP')}
  # NET has separate basic/advanced groups: using the union is stricter here.
  width=580 if plugin in ('WHDLOAD','MUSICLIB','ACTIONS','HASS') else 500
  column=min(width-150,max([190]+[22+len(label)*8 for label,_,_ in fields.values()]))
  scroll=len(fields)>page_rows
  for key,(label,kind,choices) in fields.items():
   picker=kind=='FILE' or key=='ICON' and plugin in ('WHDLOAD','MUSICLIB','ACTIONS') or key=='ENTITY' and plugin=='HASS'
   available=(400 if picker else 460 if scroll else 485)-column
   assert column-len(label)*8-6>=4,(plugin,label,'caption')
   assert available>=40,(plugin,label,'field')
   if kind in ('BOOL','CHOICE'):
    required=max(len(choice)*8 for choice in (choices.split(',') if kind=='CHOICE' else ['NO','YES']))+32
    assert available>=required,(plugin,label,available,required)
   settings_checks+=1
  for start in range(0,max(1,len(fields)),page_rows):
   rows=min(page_rows,max(1,len(fields)-start));button_y=25+rows*24
   assert button_y+32+12<=height,(plugin,'buttons')
print(f'Plugin settings: {settings_checks} bundled option labels/choices checked; scrollbars only when rows exceed screen capacity.')

# Compile the real Warp/Prism constructor, capturing each GadTools rectangle.
warp=(ROOT/'plugin_src/warp/nd_warp_ui.c').read_text()
fragment=warp[warp.index(' memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=visual;'):warp.index(' win=OpenWindowTags(')]
fragment=fragment.replace('goto cleanup','return 0')
tags=sorted(set(re.findall(r'\b(?:GTTX_\w+|GTIN_\w+|GTST_\w+|GTCY_\w+|GTLV_\w+|GA_\w+|\w+_KIND|TAG_DONE|TRUE)\b',fragment)))
warp_code=pre+ '\n'.join(f'#define {tag} {i+10}' for i,tag in enumerate(tags))+r'''
#define HOOKFUNC void *
struct TextAttr {int ta_YSize;};struct Hook {void *h_Entry,*h_Data;};
struct UI {struct Hook hook;int networks;};struct WarpScreen {int WBorTop;struct TextAttr *Font;};
static void nd_warp_password_hook_entry(void){}static struct NewGadget captured[32];static int captured_count;
static void *CreateGadget(int kind,void *last,struct NewGadget *ng,...){(void)kind;(void)last;captured[captured_count++]=*ng;return ng;}
static int construct(int compact,int mode){
 struct UI value,*ui=&value;struct WarpScreen screen_value,*screen=&screen_value;struct TextAttr font;
 struct NewGadget ng;int top,w=560,step=compact ? 20:28,wifi=mode!=0,prism=mode==2,row;
 void *visual=0,*current,*last,*unit_gadget,*ssid,*password,*level,*security_gadget,*network_list,*status[3],*apply,*refresh,*levels=0,*security=0;
 screen->WBorTop=2;screen->Font=&font;font.ta_YSize=8;captured_count=0;
'''+fragment+r'''
 return ng.ng_TopEdge+32;
}
int main(void){int h,mode,win_height,i,j;struct Page p;
 for(h=256;h<=480;h+=224)for(mode=0;mode<3;++mode){
  win_height=construct(h-11<400,mode);assert(win_height+13<=h);
  page=mode==2 ? "Prism Wi-Fi":mode==1 ? "Warp Wi-Fi":"Warp turbo";
  nd_prefs_layout_init(&p.layout,8,8,640,h,11,560,win_height);
  for(i=0;i<captured_count;++i){check(&p,&captured[i],captured[i].ng_Flags);
   for(j=0;j<i;++j){struct NewGadget *a=&captured[i],*b=&captured[j];
    assert(a->ng_LeftEdge>=b->ng_LeftEdge+b->ng_Width || b->ng_LeftEdge>=a->ng_LeftEdge+a->ng_Width ||
     a->ng_TopEdge>=b->ng_TopEdge+b->ng_Height || b->ng_TopEdge>=a->ng_TopEdge+a->ng_Height);}
  }
 }
 printf("Wi-Fi/turbo: %d production gadget rectangles fit without overlap.\n",checks);return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'warp.c').write_text(warp_code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'warp.c'),str(ROOT/'src/prefs_layout.c'),'-o',str(p/'warp')],check=True)
 subprocess.run([str(p/'warp')],check=True)

# Capture the production AMINET and AmiHASS picker constructors too. No HTTP,
# bridge or OS calls run: only the constructor's gadget creation is exercised.
aminet=(ROOT/'plugin_src/aminet/nd_aminet_ui.h').read_text()
am_fragment=aminet[aminet.index(' top=ui->screen->WBorTop'):aminet.index(' ui->window=OpenWindowTags(')].replace('goto fail','return 0')
hass=(ROOT/'plugin_src/hass/nd_hass_ui.c').read_text()
ha_fragment=hass[hass.index(' memset(&ng,0,sizeof(ng));'):hass.index(' if(nd_hass_global_type(domain))sprintf(title,')].replace('goto finished','return 0')
all_tags=sorted(set(re.findall(r'\b(?:GTTX_\w+|GTST_\w+|GTLV_\w+|\w+_KIND|TAG_DONE)\b',am_fragment+ha_fragment)))
browser_code=pre+'\n'.join(f'#define {tag} {i+10}' for i,tag in enumerate(all_tags))+r'''
struct TextAttr {int ta_YSize;};struct BrowserScreen {int WBorTop;struct TextAttr *Font;};
struct Browser {int width,height,labels;void *visual,*query,*list,*filter,*room,*room_button,*extra,*status;struct {int capacity;char area[96];} model;struct BrowserScreen *screen;};
static struct NewGadget captured[32];static int captured_count;
static void *CreateGadget(int kind,void *last,struct NewGadget *ng,...){(void)kind;(void)last;captured[captured_count++]=*ng;return ng;}
static int nd_hass_picker_rows(int h,int fy){return (h-4)/fy;}
static int construct_am(struct Browser *ui){
 struct NewGadget ng;struct TextAttr font;int top,i,gap=6;void *last=0;
 static const char *nav[]={"Recent","Categories","Back"},*buttons[]={"Open","Readme","Get","Close"};
'''+am_fragment+r'''
 return 1;}
static int construct_ha(struct Browser *p,int w,int h,int button_count){
 struct NewGadget ng;struct TextAttr font;int capacity,top=p->screen->WBorTop+8+8,i;void *last=0;char filter[64]="";int devices=1;
 static const char *buttons[]={"Previous","Next","Choose","Refresh","Cancel","Devices..."};font.ta_YSize=8;
'''+ha_fragment+r'''
 return 1;}
int main(void){struct Browser ui;struct BrowserScreen screen;struct TextAttr font;struct Page p;int h,mode,i,j;
 font.ta_YSize=8;screen.Font=&font;screen.WBorTop=2;memset(&ui,0,sizeof(ui));ui.screen=&screen;
 for(h=256;h<=480;h+=224)for(mode=0;mode<3;++mode){captured_count=0;
  ui.width=mode ? 620:628;ui.height=h-11-(mode ? 12:8);if(ui.height>(mode ? 350:460))ui.height=mode ? 350:460;
  if(mode)assert(construct_ha(&ui,ui.width,ui.height,mode==1 ? 5:6));else assert(construct_am(&ui));
  assert(ui.height+15<=h);page=mode ? "AmiHASS picker":"AMINET browser";
  nd_prefs_layout_init(&p.layout,8,8,640,h,11,ui.width,ui.height);
  for(i=0;i<captured_count;++i){check(&p,&captured[i],captured[i].ng_Flags ? captured[i].ng_Flags:PLACETEXT_IN);
   for(j=0;j<i;++j){struct NewGadget *a=&captured[i],*b=&captured[j];
    assert(a->ng_LeftEdge>=b->ng_LeftEdge+b->ng_Width || b->ng_LeftEdge>=a->ng_LeftEdge+a->ng_Width ||
     a->ng_TopEdge>=b->ng_TopEdge+b->ng_Height || b->ng_TopEdge>=a->ng_TopEdge+a->ng_Height);}
  }
 }
 printf("Browsers: %d production gadget rectangles fit without overlap.\n",checks);return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'browsers.c').write_text(browser_code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'browsers.c'),str(ROOT/'src/prefs_layout.c'),'-o',str(p/'browsers')],check=True)
 subprocess.run([str(p/'browsers')],check=True)

# Notes, Clippy, PLACES and SCREENS share this paged quick picker.
quick=(ROOT/'plugin_src/clipboard/nd_clipboard_popup.c').read_text()
quick_fragment=quick[quick.index('    q->tile_width=144;'):quick.index('    q->window=OpenWindowTags(')]
quick_code=r'''
#include <assert.h>
#include <stdio.h>
struct Screen {int Width,Height,BarHeight;};
struct NDClipQuick {int tile_width,tile_height,columns,rows,page_size,pages,page,hover;struct {int count;} clips;};
'''+ '\n'.join(re.findall(r'^#define Q_\w+ .*',quick,re.M))+'\n'+function(quick,'static void origin(')+'\n'+function(quick,'static int hit(')+r'''
int main(void){struct Screen value,*screen=&value;struct NDClipQuick panel,*q=&panel;
 int width,height,left,top,maxcols,maxrows,count,n,h,i,x,y,total=0;
 screen->Width=640;screen->BarHeight=11;
 for(h=256;h<=480;h+=224)for(n=0;n<=64;++n){screen->Height=h;q->clips.count=n;count=n ? n:1;
'''+quick_fragment+r'''
  assert(width<=640 && height+top<=h && top>=11);
  for(i=0;i<n;++i){int slot=i%q->page_size;q->page=i/q->page_size;origin(q,slot,&x,&y);
   assert(x+q->tile_width<width && y+q->tile_height<=height-Q_FOOT);
   assert(hit(q,x+1,y+1)==slot);++total;}
 }
 printf("Shared quick pickers: %d paged tile positions/hit targets fit both screens.\n",total);return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'quick.c').write_text(quick_code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89',str(p/'quick.c'),'-o',str(p/'quick')],check=True)
 subprocess.run([str(p/'quick')],check=True)
