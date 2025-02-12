import reframe as rfm
import reframe.utility.sanity as sn
from reframe.core.backends import getlauncher
import os

class MPIHiCRClass(rfm.RunOnlyRegressionTest):
    sourcesdir = os.environ['HICR_HOME']
    maintainers = ['Kiril Dichev']
    valid_systems = ['*']
    valid_prog_environs = ['*']

@rfm.simple_test
class ChannelsSPSC(MPIHiCRClass):
    backend = parameter(["lpf", "mpi"])
    sizing = parameter(["fixedSize", "variableSize"])
    num_tasks = 2
    executable_opts = ['3']

    @run_before('run')
    def prepare_run(self):
        self.descr = ('Channels - ' + self.sizing + ' - SPSC -' + self.backend)
        self.executable = self.stagedir + '/build/examples/channels/' + self.sizing + '/spsc/' + self.backend
        if self.backend == "lpf":
            self.job.launcher = getlauncher('bzlpfrun')()
        if self.backend == "mpi":
            self.job.launcher = getlauncher('bzmpirun')()

    @sanity_function
    def validate(self):
        return sn.assert_found(r'Received Value|CONSUMER', self.stdout)

@rfm.simple_test
class ChannelsMPSC(MPIHiCRClass):
    backend = parameter(["lpf", "mpi"])
    sizing = parameter(["fixedSize", "variableSize"])
    policy = parameter(["locking", "nonlocking"])
    num_tasks = 8
    executable_opts = ['256']

    @run_before('run')
    def prepare_run(self):
        self.descr = ('Channels - ' + self.sizing + ' - MPSC - Policy: ' + self.policy + ' - Backend: ' + self.backend)
        self.executable = self.stagedir + '/build/examples/channels/' + self.sizing + '/mpsc/' + self.policy + '/' + self.backend
        if self.backend == "lpf":
            self.job.launcher = getlauncher('bzlpfrun')()
        if self.backend == "mpi":
            self.job.launcher = getlauncher('bzmpirun')()

    @sanity_function
    def validate(self):
        return sn.assert_found(r'Recv Value|CONSUMER', self.stdout)


@rfm.simple_test
class TopologyDistributed(MPIHiCRClass):
    num_tasks = 4
    @run_before('run')
    def prepare_run(self):
        self.descr = ('TopologyDistributed')
        self.executable = self.stagedir + '/build/examples/topology/distributed/mpi'
    @sanity_function
    def validate(self):
        return sn.assert_found(r'Worker', self.stdout)

@rfm.simple_test
class ObjectStore(MPIHiCRClass):
    backend = parameter(["lpf"])
    num_tasks = 2
    @run_before('run')
    def prepare_run(self):
        self.descr = ('Object Store - Publish-Read')
        self.executable = self.stagedir + '/build/examples/objectStore/publishRead/' + self.backend
        self.job.launcher = getlauncher('bzlpfrun')()

    @sanity_function
    def validate(self):
        return sn.assert_found(r'Received block 2: This is another block', self.stdout)
