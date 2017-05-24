import pickle, math, sys

def mkSeeds(N) :
    seeds = []
    n = 0
    while n <= N :
          seeds.append(int(math.pow(10,n)))
          n = n+1
    return seeds

params = {'files':['astro.bov','fusion.bov','fishtank.bov'],
          'term': ['short','med','long', 'long2', 'long5', 'long10', 'long20', 'long100'],
          'ptype' : ['particle', 'streamline -1', 'streamline 100', 'streamline 1000'],
          'alg' : ['GPU', 'TBB_16', 'TBB_28', 'TBB_20'],
          'seeds' : mkSeeds(9),
          'sType' : ['sparse', 'medium', 'dense']
          }

def compare(x,y) :
    A = ['GPU']
    if x[0] == 'titan' :
        for i in reversed(range(16)) : A.append('TBB_%d' %(i+1))
    if x[0] == 'rheaGPU' :
        for i in reversed(range(28*2)) : A.append('TBB_%d' %(i+1))
    if x[0] == 'rhea' :
        for i in reversed(range(16*2)) : A.append('TBB_%d' %(i+1))

    PT = ['particle', 'streamline -1', 'streamline 100', 'streamline 1000']
    T = ['short','med','long', 'long2', 'long5', 'long10', 'long20', 'long100']
    F = ['astro','fusion','fishtank']
#    print x
#    print y
    sx = (A.index(x[1]), F.index(x[2]), T.index(x[3]), x[4], PT.index(x[5]))
    sy = (A.index(y[1]), F.index(y[2]), T.index(y[3]), y[4], PT.index(y[5]))
    if sx < sy : return -1
    if sx > sy : return 1
    return 0

def rawData(f) :
    db = pickle.load(open(f, 'rb'))
    return db

def parseFile(f) :
    mach = f[:-7]
    db = pickle.load(open(f, 'rb'))
    items = []
    for k,v in db.items() :
        if 'particle' in k or 'streamline -1' in k or 'streamline 100' in k or 'streamline 1000' in k:
           items.append((mach, k[1],k[0][:-4],k[3],k[2],k[4], v))
    return items

def printIt(x):
    mach = x[0][0]
    print mach
    print 'MACH ALG PT FILE DIST SEEDS TIME'
    for xi in x:
        PT = 'UNKNOWN'
        if 'particle' in xi[5] :
           PT = 'P'
        elif 'streamline -1' in xi[5] :
           PT = 'SL'
        elif 'streamline 100' in xi[5] :
           PT = 'SL_100'
        elif 'streamline 1000' in xi[5] :
           PT = 'SL_1000'
        print xi[0], xi[1], PT, xi[2], xi[3], xi[4], xi[6]

def printCompare2(tD, rcD, rgD) :

    print 'FILE DIST SEEDS p_Titan-GPU p_Titan-TBB p_RheaG-GPU p_RheaG-TBB p_RheaC-TBB s_Titan-GPU s_Titan-TBB s_RheaG-GPU s_RheaG-TBB s_RheaC-TBB'
    for f in ['astro.bov', 'fusion.bov', 'fishtank.bov'] :
        for t in ['short', 'med', 'long', 'long2', 'long5', 'long10', 'long20', 'long100'] :
            for s in [1,10,100,1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000]:
                #print f[:-4],t,s, p_t_gpu, p_t_tbb, p_rg_gpu, p_rg_tbb, p_rc_tbb,   s_t_gpu, s_t_tbb, s_rg_gpu, s_rg_tbb, s_rc_tbb
                data = '%s %s %d ' % (f[:-4], t,s)
                for x in ['particle', 'streamline -1', 'streamline 100', 'streamline 1000'] :
                    t_gpu, rg_gpu, t_tbb, rg_tbb, rc_tbb = (0,0,0,0,0)
                    key = (f,'GPU',s,t, x)
                    if key in tD.keys() : t_gpu = tD[key]
                    if key in rgD.keys() : rg_gpu = rgD[key]
                    key = (f,'TBB_16', s,t, x)
                    if key in tD.keys() : t_tbb = tD[key]
                    key = (f,'TBB_28', s,t, x)
                    if key in rgD.keys() : rg_tbb = rgD[key]
                    key = (f,'TBB_16', s,t, x)
                    if key in rcD.keys() : p_rc_tbb = rcD[key]
                    data = data + ' %d %d %d %d %d' %(t_gpu, t_tbb, rg_gpu, rg_tbb, rc_tbb)
                print data


##New stuff
def printCompare3(tD, rcD, rgD) :
    print ',,,PARTICLES,,,,,STREAMLINES_-1,,,,,STREAMLINES_100,,,,,STREAMLINES_1000'
    print 'FILE,DIST,SEEDS,TGPU,TCPU,R2GPU,R2CPU,RCPU,TGPU,TCPU,R2GPU,R2CPU,RCPU,TGPU,TCPU,R2GPU,R2CPU,RCPU,TGPU,TCPU,R2GPU,R2CPU,RCPU'
    for f in params['files'] :
        for t in params['term'] :
            for s in params['seeds'] :
            
                data = '%s,%s,%d' % (f[:-4], t,s)
                for p in params['ptype'] :
                    t_gpu,t_cpu = (-1,-1)

                    ##Titan
                    key = (f,'GPU',s,t,p)
                    if key in tD.keys() : t_gpu = tD[key]
                    key = (f,'TBB_16',s,t,p)
                    if key in tD.keys() : t_cpu = tD[key]
                    data = data + ',%d,%d' % (t_gpu, t_cpu)

                    ##RheaGPU
                    key = (f,'GPU',s,t,p)
                    if key in rgD.keys() : t_gpu = rgD[key]
                    key = (f,'TBB_28',s,t,p)
                    if key in rgD.keys() : t_cpu = rgD[key]
                    data = data + ',%d,%d' % (t_gpu, t_cpu)

                    ##Rhea
                    key = (f,'TBB_16',s,t,p)
                    if key in rcD.keys() : t_cpu = rcD[key]
                    data = data + ',%d' % (t_cpu)
                    
                print data
                
def getValue(key, db) :
    val = -1
    if key in db.keys() :
        val = db[key]
    #print key, val
    return ',%d'%val

def printCompare(tD, sD, rcD, rgD, ptype, fname) :
    fp = open(fname, 'w')
    fp.write('FILE,DIST,SEEDS,STYPE,,TGPU,TCPU,SGPU,SCPU,R1GPU,R1CPU,R2CPU\n')
    for f in params['files'] :
        for t in params['term'] :
            for st in params['sType'] :
                for s in params['seeds'] :
                    data = '%s,%s,%d,%s,' % (f[:-4], t,s,st)
                    
                    ##Titan
                    data = data + getValue((f,'GPU',s,t,ptype, st),tD)
                    data = data + getValue((f,'TBB_16',s,t,ptype, st),tD)

                    ##Summit
                    data = data + getValue((f,'GPU',s,t,ptype, st),sD)
                    data = data + getValue((f,'TBB_20',s,t,ptype, st),sD)

                    ##RheaG
                    data = data + getValue((f,'GPU',s,t,ptype, st),rgD)
                    data = data + getValue((f,'TBB_28',s,t,ptype, st),rgD)

                    ##RheaC
                    data = data + getValue((f,'TBB_16',s,t,ptype, st),rcD)

                    fp.write('%s\n'%data)
            fp.write('\n')
        fp.write('\n')
    fp.close()

            
titanRaw = rawData('titan.pickle')
summitRaw = rawData('summit.pickle')
rheaCRaw = rawData('rhea.pickle')
rheaGRaw = rawData('rheaGPU.pickle')

printCompare(titanRaw, summitRaw, rheaCRaw, rheaGRaw, 'particle', 'particle.txt')
printCompare(titanRaw, summitRaw, rheaCRaw, rheaGRaw, 'streamline -1', 'streamline.txt')
printCompare(titanRaw, summitRaw, rheaCRaw, rheaGRaw, 'streamline 100', 'streamline_100.txt')
printCompare(titanRaw, summitRaw, rheaCRaw, rheaGRaw, 'streamline 1000', 'streamline_1000.txt')
sys.exit()

key = ('astro.bov', 'TBB_16', 100000, 'short', 'particle')
print rheaCRaw[key]

sys.exit(0)


titanData = sorted(parseFile('titan.pickle'), cmp=compare)
rheaGData = sorted(parseFile('rheaGPU.pickle'), cmp=compare)
rheaCData = sorted(parseFile('rhea.pickle'), cmp=compare)

printIt(titanData)
printIt(rheaGData)
printIt(rheaCData)
