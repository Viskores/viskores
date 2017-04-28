import pickle

def compare(x,y) :
    A = ['GPU']
    if x[0] == 'titan' :
        for i in reversed(range(16)) : A.append('TBB_%d' %(i+1))
    if x[0] == 'rheaGPU' :
        for i in reversed(range(28*2)) : A.append('TBB_%d' %(i+1))
    if x[0] == 'rhea' :
        for i in reversed(range(16*2)) : A.append('TBB_%d' %(i+1))

    T = ['short','med','long']
    F = ['astro','fusion','fishtank']
    sx = (A.index(x[1]), F.index(x[2]), T.index(x[3]), x[4])
    sy = (A.index(y[1]), F.index(y[2]), T.index(y[3]), y[4])
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
        items.append((mach, k[1],k[0][:-4],k[3],k[2], v))
    return items

def printIt(x):
    mach = x[0][0]
    print mach
    print 'MACH ALG FILE DIST SEEDS TIME'
    for xi in x:
        print xi[0], xi[1], xi[2], xi[3], xi[4], xi[5]

def printCompare(tD, rcD, rgD) :
    print 'FILE DIST SEEDS Titan-GPU Titan-TBB RheaG-GPU RheaG-TBB RheaC-TBB'
    for f in ['astro.bov', 'fusion.bov', 'fishtank.bov'] :
        for t in ['short', 'med', 'long'] :
            for s in [1000, 10000, 100000, 1000000, 10000000]:
                t_gpu, rg_gpu, t_tbb, rg_tbb, rc_tbb = (0,0,0,0,0)
                key = (f,'GPU',s,t)
                if key in tD.keys() : t_gpu = tD[key]
                if key in rgD.keys() : rg_gpu = rgD[key]
                key = (f,'TBB_16', s,t)
                if key in tD.keys() : t_tbb = tD[key]
                key = (f,'TBB_28', s,t)
                if key in rgD.keys() : rg_tbb = rgD[key]
                key = (f,'TBB_16', s,t)
                if key in rcD.keys() : rc_tbb = rcD[key]
                print f[:-4],t,s, t_gpu, t_tbb, rg_gpu, rg_tbb, rc_tbb
                    

titanRaw = rawData('titan.pickle')
rheaCRaw = rawData('rhea.pickle')
rheaGRaw = rawData('rheaGPU.pickle')
printCompare(titanRaw, rheaCRaw, rheaGRaw)


titanData = sorted(parseFile('titan.pickle'), cmp=compare)
rheaGData = sorted(parseFile('rheaGPU.pickle'), cmp=compare)
rheaCData = sorted(parseFile('rhea.pickle'), cmp=compare)

printIt(titanData)
printIt(rheaGData)
printIt(rheaCData)
