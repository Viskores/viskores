import sys, os, pickle
import subprocess

FILES = ['astro.bov', 'fusion.bov', 'fishtank.bov']
#STEPSIZE = {'astro.bov':0.0025, 'fusion.bov':0.01, 'fishtank.bov':0.0002}
STEPSIZE = {'astro.bov':0.01, 'fusion.bov':0.01, 'fishtank.bov':0.01}
TERMINATE = {'short' : 50, 'med' : 500, 'long' : 1000}
SEEDS = [100, 200]

def buildMachineMap(machineMap, machName, exeDir='.',dataDir='.', hasGPU=False, hasTBB=False, maxThreads=-1) :
    mi = {'exeDir':exeDir,
          'dataDir':dataDir,
          'hasGPU':hasGPU,
          'hasTBB':hasTBB,
          'maxThreads':maxThreads,
          'dbFile':'%s/%s.pickle'%(dataDir,machName)}
        
    machineMap[machName] = mi
    return machineMap
              
def makeAlg(mach, doTBBScaling=False):
    alg = []
    if mach['hasGPU'] :
        alg.append('GPU')
    if mach['hasTBB'] :
        alg.append('TBB_%d'%mach['maxThreads'])
    if doTBBScaling :
        maxThreads = mach['maxThreads']
        if maxThreads > 1 :
            nt = 1
            while nt < maxThreads :
                alg.append('TBB_%d'%nt)
                nt = nt+1
    return alg

def GetDB(dbFile) :
    if not os.path.isfile(dbFile) :
        db = {}
        pickle.dump(db, open(dbFile, 'wb'))
    db = pickle.load(open(dbFile, 'rb'))
    return db

def needToRun(db, dataFile, alg, seeds, term) :
    key = (dataFile, alg, seeds, term)
    return key not in db.keys()

def recordTest(db, output, dataFile, alg, seeds, term, pt) :
    key = (dataFile, alg, seeds, term)
    for l in output :
        if 'Runtime =' in l :
            time = int(l.split()[2])
            db[key] = time
            print key, time

def createCommand(db, machineInfo, dataFile, alg, seeds, term, pt) :
    exe = 'Particle_Advection_TBB'
    if 'GPU' in alg :
        exe = 'Particle_Advection_CUDA'
    exe = machineInfo['exeDir'] + '/' + exe
        
    args = ''
    args = args + '-seeds %d ' % seeds
    args = args + '-file %s/%s ' % (machineInfo['dataDir'],dataFile)
    args = args + '-h %f '% STEPSIZE[dataFile]
    args = args + '-steps %d '% TERMINATE[term]
    args = args + '-%s '%pt
    nt = -1
    if 'TBB_' in alg :
        nt = int(alg[4:])
        args = args + '-t %d ' % nt

    cmd = ''
    if needToRun(db, dataFile, alg, seeds, term) :
        cmd = '%s %s' % (exe, args)
    return cmd


machineMap = {}
machineMap = buildMachineMap(machineMap, 'titan', 'lustre', 'lustre', hasGPU=True, hasTBB=True, maxThreads=16)
machineMap = buildMachineMap(machineMap, 'rhea', 'build/bin/', '.', hasGPU=False, hasTBB=True, maxThreads=16)  ##HT 32
machineMap = buildMachineMap(machineMap, 'rheaGPU', 'build.rhea/bin', '.', hasGPU=True, hasTBB=True, maxThreads=28) ##HT 56
machineMap = buildMachineMap(machineMap, 'whoopingcough', './build/bin', 'data', hasGPU=True, hasTBB=True, maxThreads=24)


#########################
machine = ''
fileDir = ''

for i in range(len(sys.argv)) :
    arg = sys.argv[i]
    if arg == '-mach' :
        i = i+1
        machine = sys.argv[i]

if machine == '' :
    print 'Usage: python %s -mach <machine>' %sys.argv[0]
    sys.exit(0)

machineInfo = machineMap[machine]
db = GetDB(machineInfo['dbFile'])

ALG = makeAlg(machineInfo, doTBBScaling=False)
PT = ['particle', 'streamline']

for f in FILES :
    for t in ['short', 'long'] :
        for a in ALG :
            for s in SEEDS :
                for p in PT :
                    cmd = createCommand(db, machineInfo, f, a, s, t, p)
                    if cmd == '' : continue
                    print 'running....', cmd
                    result = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE)
                    recordTest(db, result.stderr.readlines(), f, a, s, t, p)
                    pickle.dump(db, open(machineInfo['dbFile'], 'wb'))
                    
pickle.dump(db, open(machineInfo['dbFile'], 'wb'))
