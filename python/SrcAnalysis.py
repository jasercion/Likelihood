"""
Interface to SWIG-wrapped C++ classes.

@author J. Chiang <jchiang@slac.stanford.edu>
"""
#
# $Header: /nfs/slac/g/glast/ground/cvs/Likelihood/python/SrcAnalysis.py,v 1.2 2005/01/29 16:01:46 jchiang Exp $
#
import os, sys
import numarray as num
sys.path.insert(0, os.path.join(os.environ['LIKELIHOODROOT'], 'python'))
import pyLike as Likelihood
import hippoplotter as plot

_funcFactory = Likelihood.SourceFactory_funcFactory()

class SrcAnalysis(object):
    def __init__(self, srcModel, eventFile, scFile, expMap=None, irfs='TEST',
                 optimizer='Minuit'):
        self.optimizer = optimizer
        Likelihood.LogLike_loadResponseFunctions(irfs)
        if expMap is not None:
            Likelihood.ExposureMap_readExposureFile(expMap)
        self.logLike = Likelihood.LogLike()
        self._readData(scFile, eventFile)
        self.events = self.logLike.events();
        self.logLike.readXml(srcModel, _funcFactory)
        self.logLike.computeEventResponses()
        ebounds = Likelihood.RoiCuts_instance().getEnergyCuts()
        eMin, eMax = ebounds.first, ebounds.second
        nee = 21
        estep = num.log(eMax/eMin)/(nee-1)
        self.energies = eMin*num.exp(estep*num.arange(nee, type=num.Float))
        self.disp = None
        self.resids = None
    def _fileList(self, files):
        if isinstance(files, str):
            return [files]
        else:
            return files
    def _readData(self, scFile, eventFile):
        self._readScData(scFile)
        self._readEvents(eventFile)
    def _readScData(self, scFile):
        scFiles = self._fileList(scFile)
        Likelihood.ScData_readData(scFiles[0], True)
        for file in scFiles[1:]:
            Likelihood.ScData_readData(scFile)
    def _readEvents(self, eventFile):
        eventFiles = self._fileList(eventFile)
        Likelihood.RoiCuts_instance().readCuts(eventFiles[0])
        for file in eventFiles:
            self.logLike.getEvents(file)
    def _Nobs(self, emin, emax):
        nobs = 0
        for event in self.events:
            if emin < event.getEnergy() < emax:
                nobs += 1
        return nobs
    def plotData(self, yrange=None):
        nt = plot.newNTuple(([], [], []),
                            ('energy', 'nobs', 'nobs_err'))
        self.data_nt = nt
        for emin, emax in zip(self.energies[:-1], self.energies[1:]):
            nobs = self._Nobs(emin, emax)
            nt.addRow((num.sqrt(emin*emax), nobs, num.sqrt(nobs)))
        self.disp = plot.XYPlot(nt, 'energy', 'nobs', yerr='nobs_err',
                                xlog=1, ylog=1)
        self.disp.getDataRep().setSymbol('filled_square', 2)
        if yrange is None:
            yrange = (0.5, max(nt.getColumn('nobs'))*1.5)
        self.disp.setRange('y', yrange[0], yrange[1])
    def _srcCnts(self, source):
        cnts = []
        for emin, emax in zip(self.energies[:-1], self.energies[1:]):
            cnts.append(source.Npred(emin, emax))
        return num.array(cnts)
    def _plot(self, source, oplot=False, yrange=None, color='black'):
        if oplot and self.disp is not None:
            plot.canvas.selectDisplay(self.disp)
        else:
            self.plotData(yrange)
            
        col = lambda x: num.array(self.data_nt.getColumn(x))
        
        energies = col('energy')
        model = self._srcCnts(self.logLike.getSource(source))
        plot.scatter(energies, model, pointRep='Line', oplot=1, color=color)

        resid = (col('nobs') - model)/model
        resid_err = col('nobs_err')/model

        nt = plot.newNTuple((energies, resid, resid_err),
                            ('energy', 'resid', 'resid_err'))
        if oplot and self.resids is not None:
            plot.canvas.selectDisplay(self.resids)
            rep = plot.XYPlot(nt, 'energy', 'resid', yerr='resid_err',
                                      xlog=1, oplot=1, color=color)
            rep.setSymbol('filled_square', 2)
        else:
            self.resids = plot.XYPlot(nt, 'energy', 'resid', yerr='resid_err',
                                      xlog=1, color=color, yrange=(-1, 1))
            self.resids.getDataRep().setSymbol('filled_square', 2)
    def plot(self, srcs=None, oplot=False, yrange=None, color='black'):
        if isinstance(srcs, str):
            self._plot(srcs, oplot=oplot, yrange=yrange, color=color)
        else:
            if srcs is None:
                srcs = Likelihood.StringVector()
                self.logLike.getSrcNames(srcs)
            self._plot(srcs[0], oplot=oplot, yrange=yrange, color=color)
            if len(srcs) > 1:
                for src in list(srcs[1:]):
                    self._plot(src, oplot=True, color=color)
    def fit(self, optimizer=None, verbose=3, tol=1e-5):
        if optimizer is None:
            optimizer = self.optimizer
        myOpt = eval("self.logLike.%s()" % optimizer)
        myOpt.find_min(verbose, tol)
        return -self.logLike.value()

if __name__ == '__main__':
    srcAnalysis = SrcAnalysis('galdiffuse_model.xml',
                              'galdiffuse_events_0000.fits',
                              'galdiffuse_scData_0000.fits',
                              'expMap_test.fits')
    srcAnalysis.plot('Galactic Diffuse')