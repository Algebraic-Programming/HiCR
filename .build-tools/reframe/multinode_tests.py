import reframe as rfm
import reframe.utility.sanity as sn
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

    @sanity_function
    def validate(self):
        return sn.assert_found(r'Recv Value|CONSUMER', self.stdout)


@rfm.simple_test
class TopologyRPC(MPIHiCRClass):
    num_tasks = 4
    @run_before('run')
    def prepare_run(self):
        self.descr = ('TopologyRPC')
        self.executable = self.stagedir + '/build/examples/topologyRPC/mpi'
    @sanity_function
    def validate(self):
        return sn.assert_found(r'Worker', self.stdout)

@rfm.simple_test
class Deployer(MPIHiCRClass):
    num_tasks = 7
    @run_before('run')
    def prepare_run(self):
        self.descr = ('Deployer (multiple)')
        self.executable = self.stagedir + '/build/examples/deployer/multiple/mpi'
        self.executable_opts = [self.stagedir + '/examples/deployer/multiple/machineModel.json']

    @sanity_function
    def validate(self):
        return sn.assert_found(r'Reached End', self.stdout)
