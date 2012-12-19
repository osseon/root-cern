# Copyright CERN, CH-1211 Geneva 23, 2004-2006, All rights reserved.
#
# Permission to use, copy, modify, and distribute this software for any
# purpose is hereby granted without fee, provided that this copyright and
# permissions notice appear in all copies and derivatives.
#
# This software is provided "as is" without express or implied warranty.

import sys, os, gendict, selclass, gencapa, genrootmap, string, getopt, subprocess

class genreflex:
#----------------------------------------------------------------------------------
  def __init__(self):
    self.files           = []
    self.output          = None
    self.outputDir       = None
    self.outputFile      = None
    self.capabilities    = None
    self.rootmap         = None
    self.rootmaplib      = None
    self.select          = None
    self.cppopt          = ''
    self.deep            = False
    self.opts            = {}
    self.gccxmlpath      = None
    self.gccxmlpost      = ''
    self.gccxmlopt       = ''
    self.gccxmlvers      = '[UNKNOWN]'
    self.selector        = None
    self.gccxml          = ''
    self.quiet           = False
#----------------------------------------------------------------------------------
  def usage(self, status = 1) :
    print 'Usage:'
    print '  genreflex headerfile1.h [headerfile2.h] [options] [preprocesor options]'
    print 'Try "genreflex --help" for more information.'
    sys.exit(status)
#----------------------------------------------------------------------------------
  def help(self) :
    print """Generates the LCG dictionary file for each header file\n
    Usage:
      genreflex headerfile1.h [headerfile2.h] [options] [preprocesor options]\n    
    Options:
      -s <file>, --selection_file=<file>
         Class selection file to specify for which classes the dictionary
         will be generated
         Format (XML):
           <lcgdict>
           [<selection>]
             <class [name="classname"] [pattern="wildname"] 
                    [file_name="filename"] [file_pattern="wildname"] 
                    [id="xxxx"] [type="vector"]/>
             <class name="classname" >
               <field name="m_transient" transient="true"/>
               <field name="m_anothertransient" transient="true"/>
               <properties prop1="value1" [prop2="value2"]/>
             </class>
             <function [name="funcname"] [pattern="wildname"] />
             <enum [name="enumname"] [pattern="wildname"] />
             <variable [name="varname"] [pattern="wildname"] />
           [</selection>]
           <exclusion>
             <class [name="classname"] [pattern="wildname"] />
               <method name="unwanted" />
             </class>
           ...
           </lcgdict>\n
      -o <file>, --output <file>
         Output file name. If an existing directory is specified instead of a file,
         then a filename will be build using the name of the input file and will
         be placed in the given directory. <headerfile>_rflx.cpp \n
      --pool, --dataonly
         Generate minimal dictionary required for POOL persistency\n
      --interpreteronly
         Generate minimal dictionary required for interpreter\n
      --deep
         Generate dictionary for all dependend classes\n
      --split=[classdef]
         Generate a separate file for the given dictionary parts, currently supported part: classdef implementations.\n
      --reflex  (OBSOLETE)
         Generate Reflex dictionaries.\n
      --comments
         Add end-of-line comments in data and functions members as a property called "comment" \n
      --iocomments
         Add end-of-line comments in data and functions members as a property called "comment", but only for comments relevant for ROOT I/O \n
      --no_membertypedefs
         Disable the definition of class member typedefs \n
      --no_templatetypedefs
         Disable resolving of typedefs in template parameters for selection names. E.g. std::vector<MYINT>.\n
      --fail_on_warnings
         The genreflex command fails (retuns the value 1) if any warning message is issued \n
      --gccxmlpath=<path>
         Path path where the gccxml tool is installed.
         If not defined the standard PATH environ variable is used\n
      --gccxmlopt=<gccxmlopt>
         Options to be passed directly to gccxml\n
      --gccxmlpost=<xmlfile>
         Instead of invoking GCCXML process the xmlfile generated by GCCXML. Mainly useful for debugging.\n
      -c <file>, --capabilities=<file>
         Generate the capabilities file to be used by the SEAL Plugin Manager. This file
         lists the names of all classes for which the reflection is formation is provided.\n
      --rootmap=<file>
         Generate the rootmap file to be used by ROOT/CINT. This file lists the names of 
         all classes for which the reflection is formation is provided.\n
      --rootmap-lib=<library>
         Library name for the rootmap file.\n
      --debug
         Print extra debug information while processing. Keep intermediate files\n
      --quiet
         Do not print informational messages\n
      -h, --help
         Print this help\n
     """ 
    sys.exit()
#----------------------------------------------------------------------------------
  def parse_args(self, argv = sys.argv) :
    options = []
    #----Ontain the list of files to process------------
    for a in argv[1:] :
      if a[0] != '-' :
        self.files.append(a)
      else :
        options = argv[argv.index(a):]
        break
    #----Process options--------------------------------
    try:
      opts, args = getopt.getopt(options, 'ho:s:c:I:U:D:PC', \
      ['help','debug=', 'output=','selection_file=','pool','dataonly','interpreteronly','deep','gccxmlpath=',
       'capabilities=','rootmap=','rootmap-lib=','comments','iocomments','no_membertypedefs',
       'fail_on_warnings', 'quiet', 'gccxmlopt=', 'reflex', 'split=','no_templatetypedefs','gccxmlpost='])
    except getopt.GetoptError, e:
      print "--->> genreflex: ERROR:",e
      self.usage(2)
    self.output = '.'
    self.select = None
    self.gccxmlpath = None
    self.cppopt = ''
    self.pool   = 0
    self.interpreter = 0
    for o, a in opts:
      if o in ('-h', '--help'):
        self.help()
      if o in ('--no_templatetypedefs',):
        self.opts['resolvettd'] = 0
      if o in ('--debug',):
        self.opts['debug'] = a
      if o in ('-o', '--output'):
        self.output = a
      if o in ('-s', '--selection_file'):
        self.select = a
      if o in ('--pool',):
        self.opts['pool'] = True
      if o in ('--dataonly',):
        self.opts['pool'] = True
      if o in ('--interpreteronly',):
        self.opts['interpreter'] = True
      if o in ('--deep',):
        self.deep = True
      if o in ('--split',):
        if self.opts.has_key('split') :
          self.opts['split'] += ':' + a.lower()
        else :
          self.opts['split'] = a
      if o in ('--reflex',):
        print '--->> genreflex: WARNING: --reflex option is obsolete'
      if o in ('--comments',):
        self.opts['comments'] = True
      if o in ('--iocomments',):
        self.opts['iocomments'] = True
      if o in ('--no_membertypedefs',):
        self.opts['no_membertypedefs'] = True
      if o in ('--fail_on_warnings',):
        self.opts['fail_on_warnings'] = True
      if o in ('--quiet',):
        self.opts['quiet'] = True
        self.quiet = True
      if o in ('--gccxmlpath',):
        self.gccxmlpath = a
      if o in ('--gccxmlopt',):
        self.gccxmlopt += a +' '
      if o in ('--gccxmlpost',):
        self.gccxmlpost = a
      if o in ('-c', '--capabilities'):
        self.capabilities = a
      if o in ('--rootmap',):
        self.rootmap = a
      if o in ('--rootmap-lib',):
        self.rootmaplib = a
      if o in ('-I', '-U', '-D', '-P', '-C') :
        # escape quotes; we need to use " because of windows cmd
        poseq = a.find('=')
        if poseq > 0 and a[poseq + 1] == '"' and a[-1] == '"' :
          a = a[0:poseq + 1] + '\\' + a[poseq+1:-1] + '\\"'
        self.cppopt += '"' + o + a + '" '
#----------------------------------------------------------------------------------
  def check_files_dirs(self):
    #---Check existance of input files--------------------
    if self.files :
      for f in self.files :
        if not os.path.exists(f) : 
          print '--->> genreflex: ERROR: C++ file "' + f + '" not found'
          self.usage()
    else:
      if self.gccxmlpost == '' :
        print '--->> genreflex: ERROR: No input file specified'
        self.usage()
      else:
        self.files = ['GCCXML_postproc_' + self.gccxmlpost]
    #---Check existance of output directory----------------
    if os.path.isdir(self.output) :
      self.outputDir  = self.output
      self.outputFile = None
    else :
      self.outputDir, self.outputFile = os.path.split(self.output)
    if self.outputDir and not os.path.isdir(self.outputDir) :
      print '--->> genreflex: ERROR: Output directory ', self.outputDir, ' not found'
      self.usage()
    #---Hande selection class file-------------------------
    classes = []
    if self.select :
      if not os.path.exists(self.select) :
        print '--->> genreflex: ERROR: Class selection file "' + self.select + '" not found'
        self.usage()
      for l in open(self.select).readlines() : classes.append(l[:-1])
    #----------GCCXML command------------------------------
    self.gccxml = ''
    if not self.gccxmlpath:
      try:
        import gccxmlpath
        self.gccxmlpath = gccxmlpath.gccxmlpath
      except:
        pass
    if self.gccxmlpath :
      if sys.platform == 'win32' :
        self.gccxml = self.gccxmlpath + os.sep + 'gccxml.exe'
      else :
        self.gccxml = self.gccxmlpath + os.sep + 'gccxml'
      if not os.path.isfile(self.gccxml) :
        print '--->> genreflex: ERROR: Path to gccxml given, but no executable found at', self.gccxml
        self.gccxml = ''
    if len(self.gccxml) == 0 :
      if self.which('gccxml') :
        self.gccxml = 'gccxml'
        print '--->> genreflex: INFO: Using gccxml from', self.which('gccxml')
      else :
        print '--->> genreflex: ERROR: Cannot find gccxml executable, aborting!'
        sys.exit(1)
    #---------------Open selection file-------------------
    try :
      if self.select : self.selector = selclass.selClass(self.select,parse=1)
      else           : self.selector = None
    except :
      sys.exit(1)

#----------------------------------------------------------------------------------
  def genGccxmlInfo(self):
    s = ''
    p = subprocess.Popen('"' + self.gccxml + '" --print', shell=True,
                         bufsize=-1, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         close_fds = (sys.platform != 'win32'))
    (inp, out, err) = (p.stdin, p.stdout, p.stderr)
    sout = out.read()
    serr = err.read()
    if serr :
      print '--->> genreflex: WARNING: Could not invoke %s --print' % self.gccxml
      print '--->> genreflex: WARNING: %s' % serr
      return s
    self.gccxmlvers = sout.split('\n')[0].split()[-1]
    recommendedvers = ( '0.7.0_20070615', '0.9.' )
    foundgoodvers = filter(lambda c: self.gccxmlvers.startswith(c), recommendedvers)
    if len(foundgoodvers) == 0:
      plural = ''
      if len(recommendedvers) > 1 : plural = 's'
      print '--->> genreflex: WARNING: gccxml versions differ. Used version: %s. Recommended version%s: %s ' \
            % ( self.gccxmlvers, plural, ', '.join(recommendedvers) )
      print '--->> genreflex: WARNING: gccxml binary used: %s' % ( self.gccxml )
    s += sout    
    compiler = ''
    for l in sout.split('\n'):
      ll = l.split('"')
      if ll[0].find('GCCXML_COMPILER') != -1 :
        compiler = ll[1]
        break
    bcomp = os.path.basename(compiler)
    vopt = ''
    if   bcomp in ('msvc7','msvc71','msvc8')  : return s
    elif bcomp in ('gcc','g++','c++') : vopt = '--version'
    elif bcomp in ('cl.exe','cl')     : vopt = '' # there is no option to print only the version with cl
    else :
      print '--->> genreflex: WARNING: While trying to retrieve compiler version, found unknown compiler %s' % compiler
      return s
    p = subprocess.Popen('"%s" %s'%(compiler,vopt), shell=True, bufsize=-1,
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         close_fds=(sys.platform != 'win32'))
    (inp,out,err) = (p.stdin, p.stdout, p.stderr)
    serr = err.read()
    # cl puts its version into cerr!
    if serr:
      if bcomp in ('cl.exe','cl') and serr.find('is not recognized') == -1:
        s += '\nCompiler info:\n' + serr
        return s
      else:
        print '--->> genreflex: WARNING: While trying to retrieve compiler information. Cannot invoke %s %s' % (compiler,vopt)
        print '--->> genreflex: WARNING: %s' % serr
        return s
    s += '\nCompiler info:\n' + out.read()
    return s
#----------------------------------------------------------------------------------
  def process_files(self):
    total_errors = 0
    total_warnings = 0
    file_extension = '_rflx.cpp'
    #----------Loop oover all the input files--------------
    gccxmlinfo = self.genGccxmlInfo()
    for source in self.files :
      path, fullname = os.path.split(source)
      name = fullname[:fullname.find('.')]
      xmlfile = os.path.join(self.outputDir,name+'_gccxmlout.xml')
      if( self.outputFile ) :
        dicfile = os.path.join(self.outputDir,self.outputFile)
      else :
        dicfile = os.path.join(self.outputDir,name+file_extension)
      if self.gccxmlpost == '' :
        #---------------Parse the header file with GCC_XML
        if sys.platform == 'win32' :
          cmd  = '"%s" %s "%s" -fxml=%s %s -D__REFLEX__' %(self.gccxml, self.gccxmlopt, source, xmlfile, self.cppopt)
        else :
          cmd  = '%s %s "%s" -fxml=%s %s -D__REFLEX__' %(self.gccxml, self.gccxmlopt, source, xmlfile, self.cppopt)
          if 'debug' in self.opts : print '--->> genreflex: INFO: invoking ', cmd
          if not self.quiet : print '--->> genreflex: INFO: Parsing file %s with GCC_XML' % source,
        if sys.platform == 'win32' :
          # bug http://bugs.python.org/issue1524: os.system fails if the
          # command is quoted and something else in the command is quoted.
          # Workaround: prepend "call ":
          cmd = "call " + cmd
        status = os.system(cmd)
        if status :
          print '\n--->> genreflex: ERROR: processing file with gccxml. genreflex command failed.'
          total_errors += 1
          continue
        else: 
          if not self.quiet : print 'OK'
      else:
        if not self.quiet : print '--->> genreflex: INFO: Postprocessing GCC_XML output file', self.gccxmlpost
        xmlfile = self.gccxmlpost
     #---------------Generate the dictionary---------------
      if not self.quiet : print '--->> genreflex: INFO: Generating Reflex Dictionary'
      dg = gendict.genDictionary(source, self.opts, self.gccxmlvers)
      dg.parse(xmlfile)
      classes   = dg.selclasses(self.selector, self.deep)
      functions = dg.selfunctions(self.selector)
      enums     = dg.selenums(self.selector)
      variables = dg.selvariables(self.selector)
      try:
        if self.selector :
          cnames, warnings, errors = dg.generate(dicfile, classes, functions, enums, variables, gccxmlinfo,
                                                 self.selector.io_read_rules, self.selector.io_readraw_rules )
        else :
          cnames, warnings, errors = dg.generate(dicfile, classes, functions, enums, variables, gccxmlinfo)
        if errors or (warnings and self.opts.get('fail_on_warnings', False)): os.remove(dicfile)
      except:
        # remove output file even if evil things happened
        os.remove(dicfile)
        raise
      total_errors += errors
      total_warnings += warnings
    #------------Produce Seal Capabilities source file------
      if self.capabilities :
        if os.path.isdir(self.capabilities) :
          capfile = os.path.join(self.capabilities, 'capabilities.cpp')
        else :
          capfile = os.path.join(self.outputDir, self.capabilities)
        gencapa.genCapabilities(capfile, name,  cnames)
    #------------Produce rootmap file-----------------------
      if self.rootmap :
        if os.path.isdir(self.rootmap) :
          mapfile = os.path.join(self.rootmap, 'rootmap')
        else :
          mapfile = os.path.join(self.outputDir, self.rootmap)
        if not self.rootmaplib :  self.rootmaplib = 'lib'+name+'.so'
        for td in dg.typedefs_for_usr:
          # Autoload entries are classes by default. Only include non-free typedefs
          # such that CINT gets notified that they are not classes when their enclosed
          # scope's dictionary is set up.
          # Scoped names have name != fullname; ensure that the typedef's scope is
          # part of the dictionary, too.
          if td['fullname'] != td['name'] \
                and len(td['fullname']) > 3 \
                and td['fullname'][0:-len(td['name']) - 2] in cnames:
            cnames += [ td['fullname'] ]
        classes+= dg.typedefs_for_usr
        genrootmap.genRootMap(mapfile, name,  self.rootmaplib, cnames, classes)
    #------------Delete intermediate files------------------
      if 'debug' not in self.opts and self.gccxmlpost == '':
         os.remove(xmlfile)
    #------------Report unused class selections in selection
    if self.selector : 
      total_warnings += self.selector.reportUnusedClasses()
    #------------Exit with status if errors ----------------
    if total_errors:
      sys.exit(1)
    #------------Exit with status if warnings --------------
    if total_warnings and self.opts.get('fail_on_warnings',False) :
      os.remove(dicfile)
      print '--->> genreflex: ERROR: Exiting with error due to %d warnings ( --fail_on_warnings enabled )' % total_warnings
      sys.exit(1)
#---------------------------------------------------------------------
  def which(self, name) :
    if 'PATH' in os.environ :
      if sys.platform == 'win32' : name += '.exe'
      for p in os.environ['PATH'].split(os.pathsep) :
        path = os.path.join(p,name)
        if os.path.exists(path) : return path
    return None
#---------------------------------------------------------------------
if __name__ == "__main__":
  l = genreflex()
  l.parse_args()
  l.check_files_dirs()
  l.process_files()