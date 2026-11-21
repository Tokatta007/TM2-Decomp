#!/usr/bin/env python3
"""Build a maximal fixed-address maintained-C/retail hybrid runtime candidate.

Only complete maintained source units whose every selected function fits its
retail slot are eligible. Stock-linked function bytes replace the corresponding
retail prefix at the original VA; unused retail tail bytes and all other game/
SDK code remain unchanged. This is a runtime probe, not a source-only build.
"""
import argparse,csv,json,re,struct,subprocess
from pathlib import Path
from semantic_compile import ROOT,MIPS,sha
from semantic_elf import read_elf,global_symbol
from symbol_names import renamed_addresses

HEXNAME=re.compile(r'^(?:D_|a)?([0-9A-Fa-f]{8})$')
FUNCNAME=re.compile(r'^(?:sub|func)_([0-9A-Fa-f]{8})$')
SECTION_HEX=re.compile(r'^\.tm2_(?:string_|init_)?([0-9A-Fa-f]{8})$')
def address_for(name,known):
 if name in known:return known[name]
 for pattern in (HEXNAME,FUNCNAME):
  m=pattern.match(name)
  if m:return int(m.group(1),16)
 return None
def symbol_map():
 out={'_gp':0x801809D4}
 for r in csv.DictReader((ROOT/'config/US/ida_names.tsv').open(),delimiter='\t'):out[r['name']]=int(r['va'],16)
 for path in (ROOT/'config/US/undefined_syms_auto.txt',ROOT/'config/US/undefined_syms_extra.txt'):
  for line in path.read_text().splitlines():
   m=re.match(r'\s*([A-Za-z_.$][\w.$]*)\s*=\s*0x([0-9A-Fa-f]+)\s*;',line)
   if m:out[m.group(1)]=int(m.group(2),16)
 out.update(renamed_addresses())  # config/US/names.tsv: readable names given to retail symbols
 return out
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--selection',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.exists() or not out.is_relative_to(ROOT/'work'):p.error('Choose new work output')
 selection=json.loads(a.selection.read_text());retail_path=ROOT/'bin/SCUS_94306R.EXE';retail=retail_path.read_bytes();load=struct.unpack_from('<I',retail,24)[0]
 words={r['va'].upper():int(r['words']) for n in ('extra_funcs.tsv','ida_funcs.tsv') for r in csv.DictReader((ROOT/'config/US'/n).open(),delimiter='\t')};syms=symbol_map();out.mkdir(parents=True)
 by_source={}
 for va,f in selection['functions'].items():
  if f['kind']=='maintained':by_source.setdefault(f['source'],[]).append((va.upper(),f))
 eligible=[];skipped=[]
 for source,funcs in sorted(by_source.items()):
  unit=selection['units'][source];exports={e['name']:e for e in unit['exports']}
  selected_per_section={}
  for va,f in funcs:
   e=exports.get(f['symbol'])
   if e:selected_per_section[e['section']]=selected_per_section.get(e['section'],0)+1
  bad=[]
  for va,f in funcs:
   e=exports.get(f['symbol']);limit=4*words[va]
   size=(e['section_size'] if e and e['offset']==0 and selected_per_section.get(e['section'])==1 else e['size']) if e else None
   if not e or not e['code'] or not size or size>limit:bad.append(dict(va=va,reason='missing_or_oversized',bytes=size,limit=limit))
  if bad:skipped.append(dict(source=source,reasons=bad));continue
  eligible.append((source,funcs,unit))
 image=bytearray(retail);ranges=[];functions=[];units=[]
 for index,(source,funcs,unit) in enumerate(eligible):
  obj=ROOT/unit['object']
  if sha(ROOT/source)!=unit['source_sha256'] or sha(obj)!=unit['object_sha256']:raise ValueError('Stale unit '+source)
  exports=unit['exports'];section_bases={}
  # Selected function identity is authoritative even when a one-function object
  # uses generic .text and exports a descriptive name absent from IDA names.
  inconsistent=None
  for va,f in funcs:
   e=next(x for x in exports if x['name']==f['symbol'])
   base=int(va,16)-e['offset'];old=section_bases.setdefault(e['section'],base)
   if old!=base:inconsistent=e['section'];break
  if inconsistent:
   skipped.append(dict(source=source,reasons=[dict(reason='noncontiguous_shared_code_section',section=inconsistent)]));continue
  for e in exports:
   if e['storage']!='section':continue
   address=address_for(e['name'],syms)
   if address is None:
    m=SECTION_HEX.match(e['section']);address=int(m.group(1),16)+e['offset'] if m else None
   if address is None:continue
   base=address-e['offset'];old=section_bases.setdefault(e['section'],base)
   if old!=base:raise ValueError('Inconsistent section base '+source+' '+e['section'])
  # Every allocated section must either be placed or safely discarded metadata.
  elf0=read_elf(obj)
  allocated=[s for s in elf0['sections'] if s['flags']&2 and s['size']]
  for s in allocated:  # e.g. a compiler jump table moved to its retail address (#pragma tm2_rdata)
   m=SECTION_HEX.match(s['name'])
   if m and s['name'] not in section_bases:section_bases[s['name']]=int(m.group(1),16)
  unknown=[s['name'] for s in allocated if s['name'] not in section_bases and not s['name'].startswith('.MIPS')]
  if unknown:
   skipped.append(dict(source=source,reasons=[dict(reason='unplaced_allocated_sections',sections=unknown)]));continue
  folder=out/('unit-%04d'%index);folder.mkdir();script=folder/'unit.ld';elfpath=folder/'unit.elf'
  placed=' '.join('.s%d 0x%08X : { *(%s) }'%(n,base,sec) for n,(sec,base) in enumerate(sorted(section_bases.items())))
  imports=[]
  for name in unit['imports']:
   address=address_for(name,syms)
   if address is None:imports.append(name)
   else:syms[name]=address
  if imports:
   skipped.append(dict(source=source,reasons=[dict(reason='unresolved_imports',symbols=imports)]));continue
  definitions=' '.join('%s = 0x%08X;'%(name,syms[name]) for name in unit['imports'] if name!='_gp')+' _gp = 0x%08X;'%syms['_gp']
  script.write_text('SECTIONS { '+placed+' /DISCARD/ : { *(.reginfo .pdr .comment .gnu.attributes) } }\n'+definitions+'\n',encoding='ascii')
  q=subprocess.run([str(MIPS/'mipsel-none-elf-ld.exe'),'-EL','-T',str(script),'-o',str(elfpath),str(obj)],capture_output=True,timeout=30);(folder/'link.log').write_bytes(q.stdout+q.stderr)
  if q.returncode:
   skipped.append(dict(source=source,reasons=[dict(reason='link_failed',log=(q.stdout+q.stderr).decode(errors='replace')[-2000:])]))
   continue
  elf=read_elf(elfpath);copied=[]
  for va,f in funcs:
   address=int(va,16);sym=global_symbol(elf,f['symbol']);sec=elf['sections'][sym['section_index']];start=sym['address']-sec['address'];limit=4*words[va]
   selected_in_section=sum(1 for _,other in funcs if global_symbol(elf,other['symbol'])['section_index']==sym['section_index'])
   copy_size=sec['size'] if start==0 and selected_in_section==1 else sym['size'];data=sec['data'][start:start+copy_size]
   if sym['address']!=address or not data or len(data)>limit:raise ValueError('Linked function placement drift '+va)
   offset=address-load+0x800
   if not 0x800<=offset<=len(image)-limit:raise ValueError('Function outside image '+va)
   if any(address<old+size and old<address+limit for old,size in ranges):raise ValueError('Overlap '+va)
   image[offset:offset+len(data)]=data;ranges.append((address,limit));functions.append(va);copied.append(dict(va=va,symbol=f['symbol'],symbol_bytes=sym['size'],bytes=len(data),retail_slot=limit,
       section_tail_included=len(data)>sym['size']))
  # Placed data sections (switch jump tables from #pragma tm2_rdata, data defined at its retail address) belong to
  # the unit too: a non-identical function's jump table must point at its own case labels, not retail's.
  data_copied=[]
  for n,(sec_name,base) in enumerate(sorted(section_bases.items())):
   if not SECTION_HEX.match(sec_name):continue
   out_sec=next((s for s in elf['sections'] if s['name']=='.s%d'%n),None)
   if out_sec is None or not out_sec['size'] or out_sec['address']!=base:continue
   data=out_sec['data'];offset=base-load+0x800
   if offset>=len(image):continue  # BSS beyond the file image (zero-initialised storage): nothing to write
   if not 0x800<=offset<=len(image)-len(data):raise ValueError('Data section straddles the image end '+sec_name)
   if any(base<old+size and old<base+len(data) for old,size in ranges):raise ValueError('Data section overlaps code '+sec_name)
   if image[offset:offset+len(data)]!=data:data_copied.append(dict(section=sec_name,address='%08X'%base,bytes=len(data)))
   image[offset:offset+len(data)]=data
  units.append(dict(source=source,source_sha256=unit['source_sha256'],object=unit['object'],object_sha256=unit['object_sha256'],linked_elf_sha256=sha(elfpath),functions=copied,data_changed=data_copied))
 exe=out/'TM2_MAXIMAL_HYBRID.EXE';exe.write_bytes(image)
 report=dict(schema='tm2-maximal-fixed-hybrid-v1',image=exe.relative_to(ROOT).as_posix(),image_sha256=sha(exe),retail_sha256=sha(retail_path),selection=str(a.selection),selection_sha256=sha(a.selection),tool_sha256=sha(Path(__file__)),eligible_complete_units=len(eligible),linked_units=len(units),replaced_functions=len(functions),replaced_function_vas=functions,code_ranges=[['%08X'%a,n] for a,n in sorted(ranges)],units=units,skipped_units=skipped,other_game_code='retail dependency bytes',sdk_code='retail dependency bytes',fixed_address=True,instruction_rewriting=False,source_only=False,stage1_complete=False,runtime_validated=False,scope=__doc__)
 (out/'image-report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:report[k] for k in ('image','image_sha256','eligible_complete_units','linked_units','replaced_functions','source_only','runtime_validated')},indent=2))
if __name__=='__main__':main()
