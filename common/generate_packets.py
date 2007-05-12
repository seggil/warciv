#!/usr/bin/env python

#
# Freeciv - Copyright (C) 2003 - Raimar Falke
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#

### The following parameters change the amount of output.

# generate_stats will generate a large amount of statistics how many
# info packets got discarded and how often a field is transmitted. You
# have to call delta_stats_report to get these.
generate_stats=0

# generate_freelogs will generate freelog calls to debug the delta code.
generate_freelogs=0
freelog_log_level="LOG_NORMAL"
generate_variant_freelogs=0

### The following parameters CHANGE the protocol. You have been warned.
fold_bool_into_header=1
disable_delta=0

################# END OF PARAMETERS ####################

# This program runs under python 1.5 and 2.2 and hopefully every
# version in between. Please leave it so. In particular use the string
# module and not the function of the string type.

import re, string, os, sys

lazy_overwrite=0

def verbose(s):
    if len(sys.argv)>1 and sys.argv[1]=="-v":
        print s
        
def prefix(prefix,str):
    lines=string.split(str,"\n")
    lines=map(lambda x,prefix=prefix: prefix+x,lines)
    return string.join(lines,"\n")

def write_disclaimer(f):
    f.write('''
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/

''')

def my_open(name):
    verbose("writing %s"%name)
    f=open(name,"w")
    write_disclaimer(f)
    return f

def get_choices(all):
    def helper(helper,all, index, so_far):
        if index>=len(all):
            return [so_far]
        t0=so_far[:]
        t1=so_far[:]
        t1.append(all[index])
        return helper(helper,all,index+1,t1)+helper(helper,all,index+1,t0)

    result=helper(helper,all,0,[])
    assert len(result)==2**len(all)
    return result

def without(all,part):
    result=[]
    for i in all:
        if i not in part:
            result.append(i)
    return result
    
# A simple container for a type alias
class Type:
    def __init__(self,alias,dest):
        self.alias=alias
        self.dest=dest

# Parses a line of the form "COORD x, y; key" and returns a list of
# Field objects. types is a list of Type objects which are used to
# dereference type names.
def parse_fields(str, types):
    mo=re.search(r"^\s*(\S+(?:\(.*\))?)\s+([^;()]*)\s*;\s*(.*)\s*$",str)
    assert mo,str
    arr=[]
    for i in mo.groups():
        if i:
            arr.append(string.strip(i))
        else:
            arr.append("")
    type,fields_,flags=arr
    #print arr

    # analyze type
    while 1:
        found=0
        for i in types:
            if i.alias==type:
                type=i.dest
                found=1
                break
        if not found:
            break

    typeinfo={}
    mo=re.search("^(.*)\((.*)\)$",type)
    assert mo,repr(type)
    typeinfo["dataio_type"],typeinfo["struct_type"]=mo.groups()

    if typeinfo["struct_type"]=="float":
        mo=re.search("^float(\d+)$",typeinfo["dataio_type"])
        assert mo
        typeinfo["float_factor"]=int(mo.group(1))

    # analyze fields
    fields=[]
    for i in map(string.strip,string.split(fields_,",")):
        t={}

        def f(x):
            arr=string.split(x,":")
            if len(arr)==1:
                return [x,x,x]
            else:
                assert len(arr)==2
                arr.append("old->"+arr[1])
                arr[1]="real_packet->"+arr[1]
                return arr

        mo=re.search(r"^(.*)\[(.*)\]\[(.*)\]$",i)
        if mo:
            t["name"]=mo.group(1)
            t["is_array"]=2
            t["array_size1_d"],t["array_size1_u"],t["array_size1_o"]=f(mo.group(2))
            t["array_size2_d"],t["array_size2_u"],t["array_size2_o"]=f(mo.group(3))
        else:
            mo=re.search(r"^(.*)\[(.*)\]$",i)
            if mo:
                t["name"]=mo.group(1)
                t["is_array"]=1
                t["array_size_d"],t["array_size_u"],t["array_size_o"]=f(mo.group(2))
            else:
                t["name"]=i
                t["is_array"]=0
        fields.append(t)

    # analyze flags
    flaginfo={}
    arr=map(string.strip,string.split(flags,","))
    arr=filter(lambda x:len(x)>0,arr)
    flaginfo["is_key"]=("key" in arr)
    if flaginfo["is_key"]: arr.remove("key")
    flaginfo["diff"]=("diff" in arr)
    if flaginfo["diff"]: arr.remove("diff")
    if disable_delta:
        flaginfo["diff"] = 0
    adds=[]
    removes=[]
    remaining=[]
    for i in arr:
        mo=re.search("^add-cap\((.*)\)$",i)
        if mo:
            adds.append(mo.group(1))
            continue
        mo=re.search("^remove-cap\((.*)\)$",i)
        if mo:
            removes.append(mo.group(1))
            continue
        remaining.append(i)
    arr=remaining
    assert len(arr)==0,repr(arr)
    assert len(adds)+len(removes) in [0,1]

    if adds:
        flaginfo["add_cap"]=adds[0]
    else:
        flaginfo["add_cap"]=""

    if removes:
        flaginfo["remove_cap"]=removes[0]
    else:
        flaginfo["remove_cap"]=""

    #print typeinfo,flaginfo,fields
    result=[]
    for f in fields:
        result.append(Field(f,typeinfo,flaginfo))
    return result

# Class for a field (part of a packet). It has a name, serveral types,
# flags and some other attributes.
class Field:
    def __init__(self,fieldinfo,typeinfo,flaginfo):
        for i in fieldinfo,typeinfo,flaginfo:
            self.__dict__.update(i)
        self.is_struct=not not re.search("^struct.*",self.struct_type)

    # Helper function for the dictionary variant of the % operator
    # ("%(name)s"%dict).
    def get_dict(self,vars):
        result=self.__dict__.copy()
        result.update(vars)
        return result

    def get_handle_type(self,const=0):
        if self.dataio_type=="string":
            if const: return "const char *"
            else:     return "char *"
            
        if self.dataio_type=="worklist":
            return "%s *"%self.struct_type
        if self.is_array:
            return "%s *"%self.struct_type
        return self.struct_type+" "

    # Returns code which is used in the declaration of the field in
    # the packet struct.
    def get_declar(self):
        if self.is_array==2:
            return "%(struct_type)s %(name)s[%(array_size1_d)s][%(array_size2_d)s]"%self.__dict__
        if self.is_array:
            return "%(struct_type)s %(name)s[%(array_size_d)s]"%self.__dict__
        else:
            return "%(struct_type)s %(name)s"%self.__dict__

    # Returns code which copies the arguments of the direct send
    # functions in the packet struct.
    def get_fill(self):
        if self.dataio_type=="worklist":
            return "  copy_worklist(&real_packet->%(name)s, %(name)s);"%self.__dict__
        if self.is_array==0:
            return "  real_packet->%(name)s = %(name)s;"%self.__dict__
        if self.dataio_type=="string":
            return "  sz_strlcpy(real_packet->%(name)s, %(name)s);"%self.__dict__
        if self.is_array==1:
            tmp="real_packet->%(name)s[i] = %(name)s[i]"%self.__dict__
            return '''  {
    int i;

    for (i = 0; i < %(array_size_u) s; i++) {
      %(tmp)s;
    }
  }'''%self.get_dict(vars())
        
        return repr(self.__dict__)

    # Returns code which sets "differ" by comparing the field
    # instances of "old" and "readl_packet".
    def get_cmp(self):
        if self.dataio_type=="memory":
            return "  differ = (memcmp(old->%(name)s, real_packet->%(name)s, %(array_size_d)s) != 0);"%self.__dict__
        if self.dataio_type=="bitvector":
            return "  differ = !BV_ARE_EQUAL(old->%(name)s, real_packet->%(name)s);"%self.__dict__
        if self.dataio_type in ["string","bit_string"] and self.is_array==1:
            return "  differ = (strcmp(old->%(name)s, real_packet->%(name)s) != 0);"%self.__dict__
        if self.is_struct and self.is_array==0:
            return "  differ = !are_%(dataio_type)ss_equal(&old->%(name)s, &real_packet->%(name)s);"%self.__dict__
        if not self.is_array:
            return "  differ = (old->%(name)s != real_packet->%(name)s);"%self.__dict__
        
        if self.dataio_type=="string":
            c="strcmp(old->%(name)s[i], real_packet->%(name)s[i]) != 0"%self.__dict__
            array_size_u=self.array_size1_u
            array_size_o=self.array_size1_o
        elif self.is_struct:
            c="!are_%(dataio_type)ss_equal(&old->%(name)s[i], &real_packet->%(name)s[i])"%self.__dict__
        else:
            c="old->%(name)s[i] != real_packet->%(name)s[i]"%self.__dict__

        return '''
    {
      differ = (%(array_size_o)s != %(array_size_u)s);
      if(!differ) {
        int i;
        for (i = 0; i < %(array_size_u)s; i++) {
          if (%(c)s) {
            differ = TRUE;
            break;
          }
        }
      }
    }'''%self.get_dict(vars())

    # Returns a code fragement which updates the bit of the this field
    # in the "fields" bitvector. The bit is either a "content-differs"
    # bit or (for bools which gets folded in the header) the actual
    # value of the bool.
    def get_cmp_wrapper(self,i):
        cmp=self.get_cmp()
        if fold_bool_into_header and self.struct_type=="bool" and \
           not self.is_array:
            b="packet->%(name)s"%self.get_dict(vars())
        else:
            b="differ"
        return '''%s
  if(differ) {different++;}
  if(%s) {BV_SET(fields, %d);}

'''%(cmp,b,i)

    # Returns a code fragement which will put this field if the
    # content has changed. Does nothing for bools-in-header.    
    def get_put_wrapper(self,packet,i):
        if fold_bool_into_header and self.struct_type=="bool" and \
           not self.is_array:
            return "  /* field %(i)d is folded into the header */\n"%vars()
        put=self.get_put()
        packet_name=packet.name
        log_level=packet.log_level
        if packet.gen_freelog:
            f='    freelog(%(log_level)s, "  field \'%(name)s\' has changed");\n'%self.get_dict(vars())
        else:
            f=""
        if packet.gen_stats:
            s='    stats_%(packet_name)s_counters[%(i)d]++;\n'%self.get_dict(vars())
        else:
            s=""
        return '''  if (BV_ISSET(fields, %(i)d)) {
%(f)s%(s)s  %(put)s
  }
'''%self.get_dict(vars())

    # Returns code which put this field.
    def get_put(self):
        if self.dataio_type=="bitvector":
            return "DIO_BV_PUT(&dout, packet->%(name)s);"%self.__dict__

        if self.struct_type=="float" and not self.is_array:
            return "  dio_put_uint32(&dout, (int)(real_packet->%(name)s * %(float_factor)d));"%self.__dict__
        
        if self.dataio_type in ["worklist"]:
            return "  dio_put_%(dataio_type)s(&dout, &real_packet->%(name)s);"%self.__dict__

        if self.dataio_type in ["memory"]:
            return "  dio_put_%(dataio_type)s(&dout, &real_packet->%(name)s, %(array_size_u)s);"%self.__dict__
        
        arr_types=["string","bit_string","city_map","tech_list"]
        if (self.dataio_type in arr_types and self.is_array==1) or \
           (self.dataio_type not in arr_types and self.is_array==0):
            return "  dio_put_%(dataio_type)s(&dout, real_packet->%(name)s);"%self.__dict__
        if self.is_struct:
            c="dio_put_%(dataio_type)s(&dout, &real_packet->%(name)s[i]);"%self.__dict__
        elif self.dataio_type=="string":
            c="dio_put_%(dataio_type)s(&dout, real_packet->%(name)s[i]);"%self.__dict__
            array_size_u=self.array_size1_u

        elif self.struct_type=="float":
            c="  dio_put_uint32(&dout, (int)(real_packet->%(name)s[i] * %(float_factor)d));"%self.__dict__
        else:
            c="dio_put_%(dataio_type)s(&dout, real_packet->%(name)s[i]);"%self.__dict__

        if not self.diff:
            return '''
    {
      int i;

      for (i = 0; i < %(array_size_u)s; i++) {
        %(c)s
      }
    } '''%self.get_dict(vars())
        else:
            return '''
    {
      int i;

      assert(%(array_size_u)s < 255);

      for (i = 0; i < %(array_size_u)s; i++) {
        if(old->%(name)s[i] != real_packet->%(name)s[i]) {
          dio_put_uint8(&dout, i);
          %(c)s
        }
      }
      dio_put_uint8(&dout, 255);
    } '''%self.get_dict(vars())

    # Returns a code fragement which will get the field if the
    # "fields" bitvector says so.
    def get_get_wrapper(self,packet,i):
        get=self.get_get()
        if fold_bool_into_header and self.struct_type=="bool" and \
           not self.is_array:
            return  "  real_packet->%(name)s = BV_ISSET(fields, %(i)d);\n"%self.get_dict(vars())
        get=prefix("    ",get)
        log_level=packet.log_level
        if packet.gen_freelog:
            f="    freelog(%(log_level)s, \"  got field '%(name)s'\");\n"%self.get_dict(vars())
        else:
            f=""
        return '''  if (BV_ISSET(fields, %(i)d)) {
%(f)s%(get)s
  }
'''%self.get_dict(vars())

    # Returns code which get this field.
    def get_get(self):
        if self.struct_type=="float" and not self.is_array:
            return '''{
  int tmp;
  
  dio_get_uint32(&din, &tmp);
  real_packet->%(name)s = (float)(tmp) / %(float_factor)d.0;
}'''%self.__dict__

        if self.dataio_type=="bitvector":
            return "DIO_BV_GET(&din, real_packet->%(name)s);"%self.__dict__

        if self.dataio_type=="memory":
            return "dio_get_%(dataio_type)s(&din, real_packet->%(name)s, %(array_size_u)s);"%self.__dict__
        if self.dataio_type in ["string","bit_string","city_map","memory"] and \
           self.is_array!=2:
            return "dio_get_%(dataio_type)s(&din, real_packet->%(name)s, sizeof(real_packet->%(name)s));"%self.__dict__
        if self.is_struct and self.is_array==0:
            return "dio_get_%(dataio_type)s(&din, &real_packet->%(name)s);"%self.__dict__
        if self.dataio_type in ["tech_list"]:
            return "dio_get_%(dataio_type)s(&din, real_packet->%(name)s);"%self.__dict__
        if not self.is_array:
            if self.struct_type=="bool":
                return "dio_get_%(dataio_type)s(&din, &real_packet->%(name)s);"%self.__dict__
            return '''{
  int readin;

  dio_get_%(dataio_type)s(&din, &readin);
  real_packet->%(name)s = readin;
}'''%self.__dict__

        if self.is_struct:
            c="dio_get_%(dataio_type)s(&din, &real_packet->%(name)s[i]);"%self.__dict__
        elif self.dataio_type=="string":
            c="dio_get_%(dataio_type)s(&din, real_packet->%(name)s[i], sizeof(real_packet->%(name)s[i]));"%self.__dict__
        elif self.struct_type=="float":
            c='''int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->%(name)s[i] = (float)(tmp) / %(float_factor)d.0;'''%self.__dict__
        elif self.struct_type=="bool":
            c="dio_get_%(dataio_type)s(&din, &real_packet->%(name)s[i]);"%self.__dict__
        else:
            c='''{
  int readin;

  dio_get_%(dataio_type)s(&din, &readin);
  real_packet->%(name)s[i] = readin;
}'''%self.__dict__
        if self.is_array==2:
            array_size_u=self.array_size1_u
            array_size_d=self.array_size1_d
        else:
            array_size_u=self.array_size_u
            array_size_d=self.array_size_d

        if not self.diff:
            if array_size_u != array_size_d:
                extra='''
  if(%(array_size_u)s > %(array_size_d)s) {
    freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
    %(array_size_u)s = %(array_size_d)s;
  }'''%self.get_dict(vars())
            else:
                extra=""                
            return '''
{
  int i;
%(extra)s
  for (i = 0; i < %(array_size_u)s; i++) {
    %(c)s
  }
}'''%self.get_dict(vars())
        else:
            return '''
for (;;) {
  int i;

  dio_get_uint8(&din, &i);
  if(i == 255) {
    break;
  }
  if(i > %(array_size_u)s) {
    freelog(LOG_ERROR, "packets_gen.c: WARNING: ignoring intra array diff");
  } else {
    %(c)s
  }
}'''%self.get_dict(vars())

#'''

# Class which represents a capability variant.
class Variant:
    def __init__(self,poscaps,negcaps,name,fields,packet,no):
        self.log_level=freelog_log_level
        self.gen_stats=generate_stats
        self.gen_freelog=generate_freelogs
        self.name=name
        self.packet_name=packet.name
        self.fields=fields
        self.no=no
        
        self.no_packet=packet.no_packet
        self.want_post_recv=packet.want_post_recv
        self.want_pre_send=packet.want_pre_send
        self.want_post_send=packet.want_post_send
        self.type=packet.type
        self.delta=packet.delta
        self.is_action=packet.is_action
        
        self.poscaps=poscaps
        self.negcaps=negcaps
        if self.poscaps or self.negcaps:
            def f(cap):
                return '(has_capability("%s", pc->capability) && has_capability("%s", our_capability))'%(cap,cap)
            t=(map(lambda x,f=f: f(x),self.poscaps)+
               map(lambda x,f=f: '!'+f(x),self.negcaps))
            self.condition=string.join(t," && ")
        else:
            self.condition="TRUE"
        self.key_fields=filter(lambda x:x.is_key,self.fields)
        self.other_fields=filter(lambda x:not x.is_key,self.fields)
        self.bits=len(self.other_fields)
        self.keys_format=string.join(["%d"]*len(self.key_fields),", ")
        self.keys_arg=string.join(map(lambda x:"real_packet->"+x.name,
                                      self.key_fields),", ")
        if self.keys_arg:
            self.keys_arg=",\n    "+self.keys_arg

        if len(self.fields)==0:
            self.delta=0
            self.no_packet=1

        if len(self.fields)>5 or string.split(self.name,"_")[1]=="ruleset":
            self.handle_via_packet=1

        self.extra_send_args=""
        self.extra_send_args2=""
        self.extra_send_args3=string.join(
            map(lambda x:"%s%s"%(x.get_handle_type(1), x.name),
                self.fields),", ")
        if self.extra_send_args3:
            self.extra_send_args3=", "+self.extra_send_args3

        if not self.no_packet:
            self.extra_send_args=', const struct %(packet_name)s *packet'%self.__dict__+self.extra_send_args
            self.extra_send_args2=', packet'+self.extra_send_args2

        self.receive_prototype='static struct %(packet_name)s *receive_%(name)s(struct connection *pc, enum packet_type type)'%self.__dict__
        self.send_prototype='static int send_%(name)s(struct connection *pc%(extra_send_args)s)'%self.__dict__

    # See Field.get_dict
    def get_dict(self,vars):
        result=self.__dict__.copy()
        result.update(vars)
        return result

    # Returns a code fragement which contains the declarations of the
    # statistical counters of this packet.
    def get_stats(self):
        names=map(lambda x:'"'+x.name+'"',self.other_fields)
        names=string.join(names,", ")

        return '''static int stats_%(name)s_sent;
static int stats_%(name)s_discarded;
static int stats_%(name)s_counters[%(bits)d];
static char *stats_%(name)s_names[] = {%(names)s};

'''%self.get_dict(vars())

    # Returns a code fragement which declares the packet specific
    # bitvector. Each bit in this bitvector represents one non-key
    # field.    
    def get_bitvector(self):
        return "BV_DEFINE(%(name)s_fields, %(bits)d);\n\n"%self.__dict__

    # Returns a code fragement which is the packet specific part of
    # the delta_stats_report() function.
    def get_report_part(self):
        return '''
  if (stats_%(name)s_sent > 0 &&
      stats_%(name)s_discarded != stats_%(name)s_sent) {
    freelog(LOG_NORMAL, \"%(name)s %%d out of %%d got discarded\",
      stats_%(name)s_discarded, stats_%(name)s_sent);
    for (i = 0; i < %(bits)d; i++) {
      if(stats_%(name)s_counters[i] > 0) {
        freelog(LOG_NORMAL, \"  %%4d / %%4d: %%2d = %%s\",
          stats_%(name)s_counters[i],
          (stats_%(name)s_sent - stats_%(name)s_discarded),
          i, stats_%(name)s_names[i]);
      }
    }
  }
'''%self.__dict__

    # Returns a code fragement which is the packet specific part of
    # the delta_stats_reset() function.
    def get_reset_part(self):
        return '''
  stats_%(name)s_sent = 0;
  stats_%(name)s_discarded = 0;
  memset(stats_%(name)s_counters, 0,
         sizeof(stats_%(name)s_counters));
'''%self.__dict__

    # Returns a code fragement which is the implementation of the hash
    # function. The hash function is using all key fields.
    def get_hash(self):
        if len(self.key_fields)==0:
            return "#define hash_%(name)s hash_const\n\n"%self.__dict__
        else:
            intro='''static unsigned int hash_%(name)s(const void *vkey, unsigned int num_buckets)
{
'''%self.__dict__

            body='''  const struct %(packet_name)s *key = (const struct %(packet_name)s *) vkey;

'''%self.__dict__

            keys=map(lambda x:"key->"+x.name,self.key_fields)
            if len(keys)==1:
                a=keys[0]
            elif len(keys)==2:
                a="(%s << 8) ^ %s"%(keys[0], keys[1])
            else:
                assert 0
            body=body+('  return ((%s) %% num_buckets);\n'%a)
            extro="}\n\n"
            return intro+body+extro

    # Returns a code fragement which is the implementation of the cmp
    # function. The cmp function is using all key fields. The cmp
    # function is used for the hash table.    
    def get_cmp(self):
        if len(self.key_fields)==0:
            return "#define cmp_%(name)s cmp_const\n\n"%self.__dict__
        else:
            intro='''static int cmp_%(name)s(const void *vkey1, const void *vkey2)
{
'''%self.__dict__
            body=""
            body=body+'''  const struct %(packet_name)s *key1 = (const struct %(packet_name)s *) vkey1;
  const struct %(packet_name)s *key2 = (const struct %(packet_name)s *) vkey2;
  int diff;

'''%self.__dict__
            for field in self.key_fields:
                body=body+'''  diff = key1->%s - key2->%s;
  if (diff != 0) {
    return diff;
  }

'''%(field.name,field.name)
            extro="  return 0;\n}\n\n"
            return intro+body+extro

    # Returns a code fragement which is the implementation of the send
    # function. This is one of the two real functions. So it is rather
    # complex to create.
    def get_send(self):
        temp='''%(send_prototype)s
{
<real_packet1><delta_header>  SEND_PACKET_START(%(type)s);
<freelog><report><pre1><body><pre2>  <post>SEND_PACKET_END;
}

'''
        if self.gen_stats:
            report='''
  stats_total_sent++;
  stats_%(name)s_sent++;
'''
        else:
            report=""
        if self.gen_freelog:
            freelog='\n  freelog(%(log_level)s, "%(name)s: sending info about (%(keys_format)s)"%(keys_arg)s);\n'
        else:
            freelog=""
        if self.want_pre_send:
            pre1='''
  {
    struct %(packet_name)s *tmp = fc_malloc(sizeof(*tmp));

    *tmp = *packet;
    pre_send_%(packet_name)s(pc, tmp);
    real_packet = tmp;
  }
'''
            pre2='''
  if (real_packet != packet) {
    free((void *) real_packet);
  }
'''
        else:
            pre1=""
            pre2=""

        if not self.no_packet:
            real_packet1="  const struct %(packet_name)s *real_packet = packet;\n"
        else:
            real_packet1=""

        if not self.no_packet:
            if self.delta:
                body=self.get_delta_send_body()
                if self.is_action:
                    force_send="TRUE"
                else:
                    force_send="FALSE"
                delta_header='''  %(name)s_fields fields;
  struct %(packet_name)s *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = %(force_send)s;
  struct hash_table **hash = &pc->phs.sent[%(type)s];
  int different = 0;
'''
            else:
                body="\n"
                for field in self.fields:
                    body=body+field.get_put()+"\n"
                body=body+"\n"
                delta_header=""
        else:
            body=""
            delta_header=""

        if self.want_post_send:
            post="  post_send_%(packet_name)s(pc, real_packet);\n"
        else:
            post=""

        for i in range(2):
            for k,v in vars().items():
                if type(v)==type(""):
                    temp=string.replace(temp,"<%s>"%k,v)
        return temp%self.get_dict(vars())

    # '''

    # Helper for get_send()
    def get_delta_send_body(self):
        intro='''
  if (!*hash) {
    *hash = hash_new(hash_%(name)s, cmp_%(name)s);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = fc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

'''
        body=""
        for i in range(len(self.other_fields)):
            field=self.other_fields[i]
            body=body+field.get_cmp_wrapper(i)
        if self.gen_freelog:
            fl='    freelog(%(log_level)s, "  no change -> discard");\n'
        else:
            fl=""
        if self.gen_stats:
            s='    stats_%(name)s_discarded++;\n'
        else:
            s=""
        body=body+'''  if (different == 0 && !force_send_of_unchanged) {
%(fl)s%(s)s<pre2>    return 0;
  }

  DIO_BV_PUT(&dout, fields);
'''%self.get_dict(vars())

        for field in self.key_fields:
            body=body+field.get_put()+"\n"
        body=body+"\n"

        for i in range(len(self.other_fields)):
            field=self.other_fields[i]
            body=body+field.get_put_wrapper(self,i)
        body=body+'''

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
'''
        return intro+body

    # Returns a code fragement which is the implementation of the receive
    # function. This is one of the two real functions. So it is rather
    # complex to create.
    def get_receive(self):
        temp='''%(receive_prototype)s
{
<delta_header>  RECEIVE_PACKET_START(%(packet_name)s, real_packet);
<delta_body1><body1><freelog><body2><post>  RECEIVE_PACKET_END(real_packet);
}

'''
        if self.delta:
            delta_header='''  %(name)s_fields fields;
  struct %(packet_name)s *old;
  struct hash_table **hash = &pc->phs.received[type];
  struct %(packet_name)s *clone;
'''
            delta_body1="\n  DIO_BV_GET(&din, fields);\n"
            body1=""
            for field in self.key_fields:
                body1=body1+prefix("  ",field.get_get())+"\n"
            body1=body1+"\n"
            body2=self.get_delta_receive_body()
        else:
            delta_header=""
            delta_body1=""
            body1=""
            for field in self.fields:
                body1=body1+prefix("  ",field.get_get())+"\n"
            body1=body1+"\n"
            body2=""

        if self.gen_freelog:
            freelog='  freelog(%(log_level)s, "%(name)s: got info about (%(keys_format)s)"%(keys_arg)s);\n'
        else:
            freelog=""
        
        if self.want_post_recv:
            post="  post_receive_%(packet_name)s(pc, real_packet);\n"
        else:
            post=""

        for i in range(2):
            for k,v in vars().items():
                if type(v)==type(""):
                    temp=string.replace(temp,"<%s>"%k,v)
        return temp%self.get_dict(vars())

    # Helper for get_receive()
    def get_delta_receive_body(self):
        key1=map(lambda x:"    %s %s = real_packet->%s;"%(x.struct_type,x.name,x.name),self.key_fields)
        key2=map(lambda x:"    real_packet->%s = %s;"%(x.name,x.name),self.key_fields)
        key1=string.join(key1,"\n")
        key2=string.join(key2,"\n")
        if key1: key1=key1+"\n\n"
        if key2: key2="\n\n"+key2
        if self.gen_freelog:
            fl='    freelog(%(log_level)s, "  no old info");\n'
        else:
            fl=""
        body='''
  if (!*hash) {
    *hash = hash_new(hash_%(name)s, cmp_%(name)s);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
%(key1)s%(fl)s    memset(real_packet, 0, sizeof(*real_packet));%(key2)s
  }

'''%self.get_dict(vars())
        for i in range(len(self.other_fields)):
            field=self.other_fields[i]
            body=body+field.get_get_wrapper(self,i)

        extro='''
  clone = fc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

'''%self.get_dict(vars())
        return body+extro
    

# Class which represents a packet. A packet contains a list of fields.
class Packet:
    def __init__(self,str, types):
        self.types=types
        self.log_level=freelog_log_level
        self.gen_stats=generate_stats
        self.gen_freelog=generate_freelogs
        str=string.strip(str)
        lines=string.split(str,"\n")
        
        mo=re.search("^\s*(\S+)\s*=\s*(\d+)\s*;\s*(.*?)\s*$",lines[0])
        assert mo,repr(lines[0])

        self.type=mo.group(1)
        self.name=string.lower(self.type)
        self.type_number=int(mo.group(2))
        assert 0<=self.type_number<=255
        dummy=mo.group(3)

        del lines[0]

        arr=filter(lambda x:x,map(string.strip,string.split(dummy,",")))

        self.dirs=[]

        if "sc" in arr:
            self.dirs.append("sc")
            arr.remove("sc")
        if "cs" in arr:
            self.dirs.append("cs")
            arr.remove("cs")
        assert len(self.dirs)>0,repr(self.name)+repr(self.dirs)
            
        self.is_action="is-info" not in arr
        if not self.is_action: arr.remove("is-info")
        
        self.want_pre_send="pre-send" in arr
        if self.want_pre_send: arr.remove("pre-send")
        
        self.want_post_recv="post-recv" in arr
        if self.want_post_recv: arr.remove("post-recv")

        self.want_post_send="post-send" in arr
        if self.want_post_send: arr.remove("post-send")

        self.delta="no-delta" not in arr
        if not self.delta: arr.remove("no-delta")

        self.no_packet="no-packet" in arr
        if self.no_packet: arr.remove("no-packet")

        self.handle_via_packet="handle-via-packet" in arr
        if self.handle_via_packet: arr.remove("handle-via-packet")

        self.handle_per_conn="handle-per-conn" in arr
        if self.handle_per_conn: arr.remove("handle-per-conn")

        self.no_handle="no-handle" in arr
        if self.no_handle: arr.remove("no-handle")

        self.dsend_given="dsend" in arr
        if self.dsend_given: arr.remove("dsend")

        self.want_lsend="lsend" in arr
        if self.want_lsend: arr.remove("lsend")

        assert len(arr)==0,repr(arr)
        
        if disable_delta:
            self.delta=0

        self.fields=[]
        for i in lines:
            self.fields=self.fields+parse_fields(i,types)
        self.key_fields=filter(lambda x:x.is_key,self.fields)
        self.other_fields=filter(lambda x:not x.is_key,self.fields)
        self.bits=len(self.other_fields)
        self.keys_format=string.join(["%d"]*len(self.key_fields),", ")
        self.keys_arg=string.join(map(lambda x:"real_packet->"+x.name,
                                      self.key_fields),", ")
        if self.keys_arg:
            self.keys_arg=",\n    "+self.keys_arg

        
        self.want_dsend=self.dsend_given

        if len(self.fields)==0:
            self.delta=0
            self.no_packet=1
            assert not self.want_dsend,"dsend for a packet without fields isn't useful"

        if len(self.fields)>5 or string.split(self.name,"_")[1]=="ruleset":
            self.handle_via_packet=1

        self.extra_send_args=""
        self.extra_send_args2=""
        self.extra_send_args3=string.join(
            map(lambda x:"%s%s"%(x.get_handle_type(1), x.name),
                self.fields),", ")
        if self.extra_send_args3:
            self.extra_send_args3=", "+self.extra_send_args3

        if not self.no_packet:
            self.extra_send_args=', const struct %(name)s *packet'%self.__dict__+self.extra_send_args
            self.extra_send_args2=', packet'+self.extra_send_args2

        self.receive_prototype='struct %(name)s *receive_%(name)s(struct connection *pc, enum packet_type type)'%self.__dict__
        self.send_prototype='int send_%(name)s(struct connection *pc%(extra_send_args)s)'%self.__dict__
        if self.want_lsend:
            self.lsend_prototype='void lsend_%(name)s(struct conn_list *dest%(extra_send_args)s)'%self.__dict__
        if self.want_dsend:
            self.dsend_prototype='int dsend_%(name)s(struct connection *pc%(extra_send_args3)s)'%self.__dict__
            if self.want_lsend:
                self.dlsend_prototype='void dlsend_%(name)s(struct conn_list *dest%(extra_send_args3)s)'%self.__dict__

        # create cap variants
        all_caps={}
        for f in self.fields:
            if f.add_cap:  all_caps[f.add_cap]=1
            if f.remove_cap:  all_caps[f.remove_cap]=1
                        
        all_caps=all_caps.keys()
        choices=get_choices(all_caps)
        self.variants=[]
        for i in range(len(choices)):
            poscaps=choices[i]
            negcaps=without(all_caps,poscaps)
            fields=[]
            for field in self.fields:
                if not field.add_cap and not field.remove_cap:
                    fields.append(field)
                elif field.add_cap and field.add_cap in poscaps:
                    fields.append(field)
                elif field.remove_cap and field.remove_cap in negcaps:
                    fields.append(field)
            no=i+100

            self.variants.append(Variant(poscaps,negcaps,"%s_%d"%(self.name,no),fields,self,no))


    # Returns a code fragement which contains the struct for this packet.
    def get_struct(self):
        intro="struct %(name)s {\n"%self.__dict__
        extro="};\n\n"

        body=""
        for field in self.key_fields+self.other_fields:
            body=body+"  %s;\n"%field.get_declar()
        if not body:
            body="  char __dummy;			/* to avoid malloc(0); */\n"
        return intro+body+extro
    # '''

    # Returns a code fragement which represents the prototypes of the
    # send and receive functions for the header file.
    def get_prototypes(self):
        result=(self.receive_prototype+";\n"+
                self.send_prototype+";\n")
        if self.want_lsend:
            result=result+self.lsend_prototype+";\n"
        if self.want_dsend:
            result=result+self.dsend_prototype+";\n"
            if self.want_lsend:
                result=result+self.dlsend_prototype+";\n"
        return result+"\n"

    # See Field.get_dict
    def get_dict(self,vars):
        result=self.__dict__.copy()
        result.update(vars)
        return result
    
    # Returns a code fragment which is the implementation of the
    # ensure_valid_variant function
    def get_ensure_valid_variant(self):
        result='''static void ensure_valid_variant_%(name)s(struct connection *pc)
{
  int variant = -1;

  if(pc->phs.variant[%(type)s] != -1) {
    return;
  }

  if(FALSE) {
'''%self.get_dict(vars())
        for v in self.variants:
            cond=v.condition
            name2=v.name
            no=v.no
            result=result+'  } else if(%(cond)s) {\n    variant = %(no)s;\n'%self.get_dict(vars())
        if generate_variant_freelogs and len(self.variants)>1:
            log='  freelog(LOG_NORMAL, "%(name)s: using variant=%%d cap=%%s", variant, pc->capability);\n'%self.get_dict(vars())
        else:
            log=""
        result=result+'''  } else {
    die("unknown variant");
  }
%(log)s  pc->phs.variant[%(type)s] = variant;
}

'''%self.get_dict(vars())
        return result


    # Returns a code fragment which is the implementation of the
    # public visible receive function
    def get_receive(self):
        only_client=len(self.dirs)==1 and self.dirs[0]=="sc"
        only_server=len(self.dirs)==1 and self.dirs[0]=="cs"
        if only_client:
            restrict='''  if(is_server) {
    freelog(LOG_ERROR, "Receiving %(name)s at the server.");
  }
'''%self.get_dict(vars())
        elif only_server:
            restrict='''  if(!is_server) {
    freelog(LOG_ERROR, "Receiving %(name)s at the client.");
  }
'''%self.get_dict(vars())
        else:
            restrict=""

        result='''%(receive_prototype)s
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to read data from the closed connection %%s",
	    conn_description(pc));
    return NULL;
  }
  assert(pc->phs.variant != NULL);
%(restrict)s  ensure_valid_variant_%(name)s(pc);

  switch(pc->phs.variant[%(type)s]) {
'''%self.get_dict(vars())
        for v in self.variants:
            name2=v.name
            no=v.no
            result=result+'    case %(no)s: return receive_%(name2)s(pc, type);\n'%self.get_dict(vars())
        result=result+'    default: die("unknown variant"); return NULL;\n  }\n}\n\n'
        return result

    def get_send(self):
        only_client=len(self.dirs)==1 and self.dirs[0]=="cs"
        only_server=len(self.dirs)==1 and self.dirs[0]=="sc"
        if only_client:
            restrict='''  if(is_server) {
    freelog(LOG_ERROR, "Sending %(name)s from the server.");
  }
'''%self.get_dict(vars())
        elif only_server:
            restrict='''  if(!is_server) {
    freelog(LOG_ERROR, "Sending %(name)s from the client.");
  }
'''%self.get_dict(vars())
        else:
            restrict=""

        result='''%(send_prototype)s
{
  if(!pc->used) {
    freelog(LOG_ERROR,
	    "WARNING: trying to send data to the closed connection %%s",
	    conn_description(pc));
    return -1;
  }
  assert(pc->phs.variant != NULL);
%(restrict)s  ensure_valid_variant_%(name)s(pc);

  switch(pc->phs.variant[%(type)s]) {
'''%self.get_dict(vars())
        if not self.no_packet:
            args="pc, packet"
        else:
            args="pc"
        for v in self.variants:
            name2=v.name
            no=v.no
            result=result+'    case %(no)s: return send_%(name2)s(%(args)s);\n'%self.get_dict(vars())
        result=result+'    default: die("unknown variant"); return -1;\n  }\n}\n\n'
        return result

    def get_variants(self):
        result=""
        for v in self.variants:
            if v.delta:
                result=result+v.get_hash()
                result=result+v.get_cmp()
                result=result+v.get_bitvector()
            result=result+v.get_receive()
            result=result+v.get_send()
        result=result+self.get_ensure_valid_variant()
        return result

    # Returns a code fragement which is the implementation of the
    # lsend function.
    def get_lsend(self):
        if not self.want_lsend: return ""
        return '''%(lsend_prototype)s
{
  conn_list_iterate(*dest, pconn) {
    send_%(name)s(pconn%(extra_send_args2)s);
  } conn_list_iterate_end;
}

'''%self.__dict__

    # Returns a code fragement which is the implementation of the
    # dsend function.
    def get_dsend(self):
        if not self.want_dsend: return ""
        fill=string.join(map(lambda x:x.get_fill(),self.fields),"\n")
        return '''%(dsend_prototype)s
{
  struct %(name)s packet, *real_packet = &packet;

%(fill)s
  
  return send_%(name)s(pc, real_packet);
}

'''%self.get_dict(vars())

    # Returns a code fragement which is the implementation of the
    # dlsend function.
    def get_dlsend(self):
        if not (self.want_lsend and self.want_dsend): return ""
        fill=string.join(map(lambda x:x.get_fill(),self.fields),"\n")
        return '''%(dlsend_prototype)s
{
  struct %(name)s packet, *real_packet = &packet;

%(fill)s
  
  lsend_%(name)s(dest, real_packet);
}

'''%self.get_dict(vars())

# Returns a code fragement which is the implementation of the
# delta_stats_report() function.
def get_report(packets):
    if not generate_stats: return 'void delta_stats_report(void) {}\n\n'
    
    intro='''
void delta_stats_report(void) {
  int i;

'''
    extro='}\n\n'
    body=""

    for p in packets:
        body=body+p.get_report_part()
    return intro+body+extro

# Returns a code fragement which is the implementation of the
# delta_stats_reset() function.
def get_reset(packets):
    if not generate_stats: return 'void delta_stats_reset(void) {}\n\n'
    intro='''
void delta_stats_reset(void) {
'''
    extro='}\n\n'
    body=""

    for p in packets:
        body=body+p.get_reset_part()
    return intro+body+extro

# Returns a code fragement which is the implementation of the
# get_packet_from_connection_helper() function. This function is a big
# switch case construct which calls the appropriate packet specific
# receive function.
def get_get_packet_helper(packets):
    intro='''void *get_packet_from_connection_helper(struct connection *pc,\n    enum packet_type type)
{
  switch(type) {

'''
    body=""
    for p in packets:
        body=body+"  case %(type)s:\n    return receive_%(name)s(pc, type);\n\n"%p.__dict__
    extro='''  default:
    freelog(LOG_ERROR, "unknown packet type %d received from %s",
	    type, conn_description(pc));
    remove_packet_from_buffer(pc->buffer);
    return NULL;
  };
}

'''
    return intro+body+extro

# Returns a code fragement which is the implementation of the
# get_packet_name() function.
def get_get_packet_name(packets):
    intro='''const char *get_packet_name(enum packet_type type)
{
  switch(type) {

'''
    body=""
    for p in packets:
        body=body+'  case %(type)s:\n    return "%(type)s";\n\n'%p.__dict__
    extro='''  default:
    return "unknown";
  }
}

'''
    return intro+body+extro

# Returns a code fragement which is the declartion of
# "enum packet_type".
def get_enum_packet(packets):
    intro="enum packet_type {\n"

    mapping={}
    for p in packets:
        if mapping.has_key(p.type_number):
            print p.name,mapping[p.type_number].name
            assert 0
        mapping[p.type_number]=p
    sorted=mapping.keys()
    sorted.sort()

    last=-1
    body=""
    for i in sorted:
        p=mapping[i]
        if i!=last+1:
            line="  %s = %d,"%(p.type,i)
        else:
            line="  %s,"%(p.type)

        if (i%10)==0:
            line="%-40s /* %d */"%(line,i)
        body=body+line+"\n"

        last=i
    extro='''
  PACKET_LAST  /* leave this last */
};

'''
    return intro+body+extro

def strip_c_comment(s):
  # The obvious way:
  #    s=re.sub(r"/\*(.|\n)*?\*/","",s)
  # doesn't work with python version 2.2 and 2.3.
  # Do it by hand then.
  result=""
  for i in filter(lambda x:x,string.split(s,"/*")):
      l=string.split(i,"*/",1)
      assert len(l)==2,repr(i)
      result=result+l[1]
  return result  

# Main function. It reads and parses the input and generates the
# various files.
def main():
    ### parsing input
    input_name="packets.def"
    content=open(input_name).read()
    content=strip_c_comment(content)
    lines=string.split(content,"\n")
    lines=map(lambda x: re.sub("#.*$","",x),lines)
    lines=map(lambda x: re.sub("//.*$","",x),lines)
    lines=filter(lambda x:not re.search("^\s*$",x),lines)
    lines2=[]
    types=[]
    for i in lines:
        mo=re.search("^type\s+(\S+)\s*=\s*(.+)\s*$",i)
        if mo:
            types.append(Type(mo.group(1),mo.group(2)))
        else:
            lines2.append(i)

    packets=[]
    for str in re.split("(?m)^end$",string.join(lines2,"\n")):
        str=string.strip(str)
        if str:
            packets.append(Packet(str,types))

    ### parsing finished

    ### writing packets_gen.h
    output_h_name="packets_gen.h"

    if lazy_overwrite:
        output_h=my_open(output_h_name+".tmp")
    else:
        output_h=my_open(output_h_name)

    # write structs
    for p in packets:
        output_h.write(p.get_struct())

    output_h.write(get_enum_packet(packets))

    # write function prototypes
    for p in packets:
        output_h.write(p.get_prototypes())
    output_h.write('''
void delta_stats_report(void);
void delta_stats_reset(void);
void *get_packet_from_connection_helper(struct connection *pc, enum packet_type type);
''')
    output_h.close()

    ### writing packets_gen.c
    output_c_name="packets_gen.c"
    if lazy_overwrite:
        output_c=my_open(output_c_name+".tmp")
    else:
        output_c=my_open(output_c_name)

    output_c.write('''
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "capability.h"
#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "support.h"

#include "packets.h"

static unsigned int hash_const(const void *vkey, unsigned int num_buckets)
{
  return 0;
}

static int cmp_const(const void *vkey1, const void *vkey2)
{
  return 0;
}

''')

    if generate_stats:
        output_c.write('''
static int stats_total_sent;

''')

    if generate_stats:
        # write stats
        for p in packets:
            output_c.write(p.get_stats())
        # write report()
    output_c.write(get_report(packets))
    output_c.write(get_reset(packets))

    output_c.write(get_get_packet_helper(packets))
    output_c.write(get_get_packet_name(packets))

    # write hash, cmp, send, receive
    for p in packets:
        output_c.write(p.get_variants())
        output_c.write(p.get_receive())
        output_c.write(p.get_send())
        output_c.write(p.get_lsend())
        output_c.write(p.get_dsend())
        output_c.write(p.get_dlsend())

    output_c.close()

    if lazy_overwrite:
        for i in [output_h_name,output_c_name]:
            if os.path.isfile(i):
                old=open(i).read()
            else:
                old=""
            new=open(i+".tmp").read()
            if old!=new:
                open(i,"w").write(new)
            os.remove(i+".tmp")

    f=my_open("../server/hand_gen.h")
    f.write('''
#ifndef FC__HAND_GEN_H
#define FC__HAND_GEN_H

#include "shared.h"

#include "fc_types.h"
#include "packets.h"

struct connection;

bool server_handle_packet(enum packet_type type, void *packet,
			  struct player *pplayer, struct connection *pconn);

''')
    
    for p in packets:
        if "cs" in p.dirs and not p.no_handle:
            a=p.name[len("packet_"):]
            type=string.split(a,"_")[0]
            b=p.fields
            b=map(lambda x:"%s%s"%(x.get_handle_type(), x.name),b)
            b=string.join(b,", ")
            if b:
                b=", "+b
            if p.handle_via_packet:
                f.write('struct %s;\n'%p.name)
                f.write('void handle_%s(struct player *pplayer, struct %s *packet);\n'%(a,p.name))
            else:
                if p.handle_per_conn:
                    f.write('void handle_%s(struct connection *pc%s);\n'%(a,b))
                else:
                    f.write('void handle_%s(struct player *pplayer%s);\n'%(a,b))
    f.write('''
#endif /* FC__HAND_GEN_H */
''')
    f.close()

    f=my_open("../client/packhand_gen.h")
    f.write('''
#ifndef FC__PACKHAND_GEN_H
#define FC__PACKHAND_GEN_H

#include "packets.h"
#include "shared.h"

bool client_handle_packet(enum packet_type type, void *packet);

''')
    for p in packets:
        if "sc" not in p.dirs: continue

        a=p.name[len("packet_"):]
        b=p.fields
        #print len(p.fields),p.name
        b=map(lambda x:"%s%s"%(x.get_handle_type(), x.name),b)
        b=string.join(b,", ")
        if not b:
            b="void"
        if p.handle_via_packet:
            f.write('struct %s;\n'%p.name)
            f.write('void handle_%s(struct %s *packet);\n'%(a,p.name))
        else:
            f.write('void handle_%s(%s);\n'%(a,b))
    f.write('''
#endif /* FC__PACKHAND_GEN_H */
''')
    f.close()

    f=my_open("../server/hand_gen.c")
    f.write('''

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "packets.h"

#include "hand_gen.h"
    
bool server_handle_packet(enum packet_type type, void *packet,
			  struct player *pplayer, struct connection *pconn)
{
  switch(type) {
''')
    for p in packets:
        if "cs" not in p.dirs: continue
        if p.no_handle: continue
        a=p.name[len("packet_"):]
        c='((struct %s *)packet)->'%p.name
        b=[]
        for x in p.fields:
            y="%s%s"%(c,x.name)
            if x.dataio_type=="worklist":
                y="&"+y
            b.append(y)
        b=string.join(b,",\n      ")
        if b:
            b=",\n      "+b

        if p.handle_via_packet:
            args="pplayer, packet"
        else:
            if p.handle_per_conn:
                args="pconn"+b
            else:
                args="pplayer"+b

        f.write('''  case %s:
    handle_%s(%s);
    return TRUE;

'''%(p.type,a,args))
    f.write('''  default:
    return FALSE;
  }
}
''')
    f.close()

    f=my_open("../client/packhand_gen.c")
    f.write('''

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "packets.h"

#include "packhand_gen.h"
    
bool client_handle_packet(enum packet_type type, void *packet)
{
  switch(type) {
''')
    for p in packets:
        if "sc" not in p.dirs: continue
        if p.no_handle: continue
        a=p.name[len("packet_"):]
        c='((struct %s *)packet)->'%p.name
        b=[]
        for x in p.fields:
            y="%s%s"%(c,x.name)
            if x.dataio_type=="worklist":
                y="&"+y
            b.append(y)
        b=string.join(b,",\n      ")
        if b:
            b="\n      "+b

        if p.handle_via_packet:
            args="packet"
        else:
            args=b

        f.write('''  case %s:
    handle_%s(%s);
    return TRUE;

'''%(p.type,a,args))
    f.write('''  default:
    return FALSE;
  }
}
''')
    f.close()

main()
