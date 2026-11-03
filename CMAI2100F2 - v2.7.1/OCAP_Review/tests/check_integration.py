from pathlib import Path
import re,xml.etree.ElementTree as ET
r=Path(__file__).resolve().parents[2]
def read(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf-8-sig' if b[:3]==b'\xef\xbb\xbf' else 'cp949')
res=read(r/'resource.h');defs=dict(re.findall(r'#define\s+(\w+)\s+(\d+)',res));rc=read(r/'CMAI2100.rc')
for name in ['OCAPFAI.cpp','OCAPCosmetic.cpp','OCAPSelect.cpp']:
 s=read(r/name);dynamic=set(re.findall(r'#define\s+(\w+)',s));missing={x for x in re.findall(r'\b(?:IDC|IDD)_\w+',s) if x not in defs and x not in dynamic};assert not missing,(name,missing)
for suffix in ['', '.filters']:
 root=ET.fromstring(read(r/('CMAI2100.vcxproj'+suffix)));ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
 for tag in ['ClInclude','ClCompile']:
  names=[x.attrib['Include'] for x in root.findall('.//m:'+tag,ns) if 'Include' in x.attrib];assert len(names)==len(set(names)),(suffix,tag)
  for n in names:
   if 'OCAP' in n: assert (r/n).exists(),n
 if suffix:
  filters={e.attrib['Include'] for e in root.findall('.//m:Filter',ns) if 'Include' in e.attrib}
  for e in root.findall('.//m:Filter',ns):
   if not e.attrib:assert e.text in filters,e.text
opts=[int(defs['IDC_STC_OCAP_VALUE_'+str(i)]) for i in range(9)];assert opts==list(range(opts[0],opts[0]+9))
for d in ['IDD_OCAP_DLG','IDD_OCAP_COSMETIC_DLG','IDD_OCAP_SELECT_DLG']:
 assert len(re.findall('^'+d+r' DIALOGEX',rc,re.M))==1
for key,val in defs.items():
 if key.startswith(('IDD_OCAP','IDC_STC_OCAP','IDC_BTN_OCAP','IDC_GRD_OCAP')) or key=='IDC_BTN_MAIN_OCAP':
  assert [k for k,v in defs.items() if v==val]==[key],(key,val)
for kind in ['FAI','Cosmetic']:
 s=read(r/('OCAP'+kind+'.cpp'));assert 'gLot.' not in s.replace('gLot.nJobStatus','JOB_STATUS');assert 'MZ' not in s
 fields=set(re.findall('gCap_'+kind+r'\.(\w+)', '\n'.join(read(p) for p in r.glob('*.cpp'))))
 state=read(r/'stdafx.h');state=state[state.index('typedef struct {',state.index('struct eAccessMode')):];missing=fields-set(re.findall(r'\b(\w+)\s*(?:\[|;)',state));assert not missing,missing
 # Every implementation has a header declaration; each added method has a definition.
 header=read(r/('OCAP'+kind+'.h'))
 methods=set(re.findall(r'\b(?:void|BOOL)\s+(\w+)\s*\(',header))
 definitions=set(re.findall('OCAP'+kind+r'::(\w+)\(',s))
 assert methods<=definitions,(kind,methods-definitions)
for p in r.glob('*.cpp'):
 if p.name.startswith('OCAP'):
  s=read(p);assert '\ufffd' not in s
seq=read(r/'SequenceMain.cpp');a=seq.index('void CSequenceMain::Set_LotEnd(');b=seq.index('void CSequenceMain::Beep_Post(',a);s=seq[a:b]
assert s.index('OCAP_QueueLot(nPortNo)') < s.index('g_dlgWork.PostMessage(UM_LOT_END_MSG')
print('PASS: project XML, filter names, OCAP file registration, unique IDs, contiguous controls, dialogs, state members, class declarations and snapshot-before-clear ordering')
