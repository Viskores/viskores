import sys, os, pickle, math
import subprocess

def mkSeeds(N) :
    seeds = []
    n = 0
    while n <= N :
          seeds.append(int(math.pow(10,n)))
          n = n+1
    return seeds

FILES = ['astro.bov', 'fusion.bov', 'fishtank.bov']
#STEPSIZE = {'astro.bov':0.0025, 'fusion.bov':0.01, 'fishtank.bov':0.0002}
STEPSIZE = {'astro.bov':0.005, 'fusion.bov':0.005, 'fishtank.bov':0.0002}
TERMINATE = {'short' : 10, 'med' : 100, 'long' : 1000, 'long2' : 2000, 'long5' : 5000, 'long10' : 10000, 'long20':20000, 'long100': 100000}
#TERMINATE = {'short' : 10, 'med' : 100, 'long' : 1000}
SEED_TYPE = ["dense", "medium", "sparse"]

def buildMachineMap(machineMap, machName, exeDir='.',dataDir='.', hasGPU=False, hasTBB=False, maxThreads=-1) :
    mi = {'exeDir':exeDir,
          'dataDir':dataDir,
          'hasGPU':hasGPU,
          'hasTBB':hasTBB,
          'maxThreads':maxThreads,
          'dbFile':'%s/%s.pickle'%(dataDir,machName),
          'name':machName}

    machineMap[machName] = mi
    return machineMap

TBB_LIST = {'titan':[1,2,4,8,16, 3,5,6,7,9,10,11,12,13,14,15],
            'summit':[1,2,4,8,16, 3,5,6,7,9,10,11,12,13,14,15,16,17,18,19],
            'rhea':[1,2,4,8,16,32, 3,5,6,7,9,10,11,12,13,14,15],
            'rheaGPU':[1,2,4,8,14,28,56, 3,4,6,7,9,10,11,12,13,15,16,17,18,19,20,21,22,23,24,25,26,27]}

def makeAlg(mach, doTBBScaling=False):
    alg = []
    if mach['hasGPU'] :
        alg.append('GPU')
    if mach['hasTBB'] :
        alg.append('TBB_%d'%mach['maxThreads'])
    if doTBBScaling :
        maxThreads = mach['maxThreads']
        if maxThreads > 1 :
           for n in TBB_LIST[mach['name']] :
               alg.append('TBB_%d'%n)
    return alg

def GetDB(dbFile) :
    print dbFile
    if not os.path.isfile(dbFile) :
        db = {}
        pickle.dump(db, open(dbFile, 'wb'))
    db = pickle.load(open(dbFile, 'rb'))
    return db

def needToRun(db, dataFile, alg, seeds, term, pt, st) :
    oldKey = (dataFile, alg, seeds, term, pt)
    key = (dataFile, alg, seeds, term, pt, st)
    if oldKey in db.keys() :
       val = db[oldKey]
       db[key] = val
       del db[oldKey]

    if key in db.keys() :
        print 'Test was run:', key, db[key]
        return False
    else:
        print 'need to run: ', key
        return True

def recordTest(db, output, dataFile, alg, seeds, term, pt, st) :
    key = (dataFile, alg, seeds, term, pt, st)
    print key, output
    for l in output :
        if 'Runtime =' in l :
            time = int(l.split()[2])
            db[key] = time
            print 'Record:', key, time
            return
        if 'Error' in l :
           db[key] = -1
           print 'ERROR:', key
           return

def createCommand(db, machineInfo, dataFile, alg, seeds, term, pt, st, test=False) :
    exe = 'Particle_Advection_TBB'
    if 'GPU' in alg :
        exe = 'Particle_Advection_CUDA'
    if machineInfo['name'] == 'titan' :
       exe = 'cd %s; aprun -n 1 %s' %(machineInfo['exeDir'], exe)
    elif machineInfo['name'] == 'summit' :
       exe = 'cd %s; mpirun -np 1 %s' %(machineInfo['exeDir'], exe)
    else:
       exe = machineInfo['exeDir'] + '/' + exe

    if 'streamline' in pt :
       stepsPerRound = int(pt.split()[1])
       if stepsPerRound > 0 :
          if TERMINATE[term] <= stepsPerRound :
             return ''

    args = ''
    args = args + '-seeds %d ' % seeds
    args = args + '-file %s/%s ' % (machineInfo['dataDir'],dataFile)
    args = args + '-h %f '% STEPSIZE[dataFile]
    args = args + '-steps %d '% TERMINATE[term]
    args = args + '-%s '%pt
    args = args + '-%s '%st
    nt = -1
    if 'TBB_' in alg :
        nt = int(alg[4:])
        args = args + '-t %d ' % nt

    cmd = ''
    if test or needToRun(db, dataFile, alg, seeds, term, pt, st) :
        cmd = '%s %s' % (exe, args)
    return cmd

lustreDataDir = '/lustre/atlas/scratch/pugmire/csc094/vtkm/titan'
lustreSummitExeDir = '/lustre/atlas/scratch/pugmire/csc094/vtkm/summit'

machineMap = {}
machineMap = buildMachineMap(machineMap, 'titan', lustreDataDir, lustreDataDir,
                             hasGPU=True, hasTBB=True, maxThreads=16)
machineMap = buildMachineMap(machineMap, 'summit', lustreSummitExeDir, lustreDataDir,
                             hasGPU=False, hasTBB=True, maxThreads=20)
machineMap = buildMachineMap(machineMap, 'rhea', 'build.rheaC/bin/', lustreDataDir,
                             hasGPU=False, hasTBB=True, maxThreads=16)  ##HT 32
machineMap = buildMachineMap(machineMap, 'rheaGPU', 'build.rheaG/bin', lustreDataDir,
                             hasGPU=True, hasTBB=True, maxThreads=28) ##HT 56
machineMap = buildMachineMap(machineMap, 'whoopingcough', './build/bin', 'data', hasGPU=True, hasTBB=True, maxThreads=24)


#########################
machine = ''
tbbScale = False
doTest = False

for i in range(len(sys.argv)) :
    arg = sys.argv[i]
    if arg == '-mach' :
        i = i+1
        machine = sys.argv[i]
    elif arg == '-tbbscale' : tbbScale = True
    elif arg == '-test' : doTest = True

if machine == '' :
    print 'Usage: python %s -mach <machine>' %sys.argv[0]
    sys.exit(0)

machineInfo = machineMap[machine]
db = GetDB(machineInfo['dbFile'])

ALG = makeAlg(machineInfo, doTBBScaling=tbbScale)


PT = ['particle', 'streamline']
PT = ['particle', 'streamline -1', 'streamline 100', 'streamline 1000']
PT = ['particle', 'streamline 100', 'streamline 1000', 'streamline -1']

if machine == 'titan' :
   SEEDS = mkSeeds(6)
if machine == 'summit' :
   SEEDS = mkSeeds(7)
elif machine == 'rhea' :
   SEEDS = mkSeeds(8)
elif machine == 'rheaGPU' :
   SEEDS = mkSeeds(8)

print SEEDS

def runTests() :
    for p in PT :
        for s in SEEDS :
            for st in SEED_TYPE :
                for t in TERMINATE.keys() :
                    for a in ALG :
                        for f in FILES :
                            #print (a,s,t,p,st), f
                            cmd = createCommand(db, machineInfo, f, a, s, t, p, st)
                            if cmd == '' : continue
                            #print 'running....', cmd
                            result = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE)
                            recordTest(db, result.stderr.readlines(), f, a, s, t, p, st)
                            pickle.dump(db, open(machineInfo['dbFile'], 'wb'))
    pickle.dump(db, open(machineInfo['dbFile'], 'wb'))


if not doTest :
   runTests()
else:
  a = 'GPU'
  s = 10000
  t = 'short'
  f = 'astro.bov'
  p = 'particle'
  st = 'sparse'

  cmd = createCommand(db, machineInfo, f, a, s, t, p, st, test=True)
  print 'running....', cmd
  key = (f,a,s,t,p)
  if key in db.keys() :
     print 'Test was run, time= ', db[key]
  else:
     print 'Test NOT run'
  result = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE)
  print key
  print result.stderr.readlines()
