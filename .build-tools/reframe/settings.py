from reframe.core.backends import register_launcher
from reframe.core.launchers import JobLauncher

# This is a stupid hard-coded launcher for YZ ZRC site
@register_launcher('yzrun')
class YZLauncher(JobLauncher):
    def command(self, job):
        return ['mpirun', '--map-by','node', '-n', str(job.num_tasks), '-H', 'yzserver01:22,yzserver02:22']

@register_launcher('bzrun')
class BZLauncher(JobLauncher):
    def command(self, job):
        return ['mpirun', '-x', 'LD_LIBRARY_PATH', '--map-by','node', '-n', str(job.num_tasks), '--mca', 'plm', 'rsh']

site_configuration = {
    'systems': [
        {
            'name': 'BZ',
            'descr': 'Huawei Blue Zone cluster near Zurich',
            'hostnames': ['slurm-client'],
            'modules_system': 'spack',
            'partitions': [
                {
                    'name': 'arm',
                    'descr': 'TaiShanV110 nodes in BZ cluster',
                    'scheduler': 'slurm',
                    'launcher': 'bzrun',
                    'access':  ['-p TaiShanV110'],
                    'environs': [
                        'PrgEnv-bz',
                        ],
                    'max_jobs': 100,
                    'prepare_cmds': ['spack env activate hicr-arm'],
                    },
                ]
            },
        {
            'name': 'sergio',
            'descr': 'Sergio workstation',
            'hostnames': ['smartin','runner-kembs-ds-project'],
            'partitions': [
                {
                    'name': 'sequential',
                    'descr': 'Sergio workstation',
                    'scheduler': 'local',
                    'launcher': 'local',
                    'environs': [
                        'PrgEnv-default',
                    ],
                    'max_jobs': 4,
                },
                {
                    'name': 'mpi',
                    'descr': 'Sergio workstation',
                    'scheduler': 'local',
                    'launcher': 'mpirun',
                    'environs': [
                        'PrgEnv-default',
                    ],
                    'max_jobs': 4,
                },
            ]
        },
        {
            'name': 'YZ-ZRC',
            'descr': 'Yellow Zone cluster in ZRC',
            'hostnames': ['yzserver'],
            'partitions': [
                {
                    'name': 'default',
                    'descr': 'Default YZ partition',
                    'scheduler': 'local',
                    'launcher': 'yzrun',
                    'environs': [
                        'PrgEnv-default',
                    ],
                    'max_jobs': 4,
                },
            ]
        }
    ],
    'environments': [
        {
            'name': 'PrgEnv-default',
        },
        {
            'name': 'PrgEnv-bz',
            'modules': ['openmpi@4.1.7a1'],
            'env_vars': [
                          ['LD_LIBRARY_PATH', '$HICR_HOME/extern/lpf/build/lib:$LD_LIBRARY_PATH']
                        ]
        },
    ],
    'logging': [
        {
            'level': 'debug',
            'handlers': [
                {
                    'type': 'file',
                    'name': 'reframe.log',
                    'level': 'debug',
                    'format': '[%(asctime)s] %(levelname)s: %(check_name)s: %(message)s',   # noqa: E501
                    'append': False
                },
                {
                    'type': 'stream',
                    'name': 'stdout',
                    'level': 'info',
                    'format': '%(message)s'
                },
                {
                    'type': 'file',
                    'name': 'reframe.out',
                    'level': 'info',
                    'format': '%(message)s',
                    'append': False
                }
            ],
            'handlers_perflog': [
                {
                    'type': 'filelog',
                    'prefix': '%(check_system)s/%(check_partition)s',
                    'level': 'info',
                    'format': '%(check_job_completion_time)s|reframe %(version)s|%(check_info)s|jobid=%(check_jobid)s|%(check_perf_var)s=%(check_perf_value)s|ref=%(check_perf_ref)s (l=%(check_perf_lower_thres)s, u=%(check_perf_upper_thres)s)',  # noqa: E501
                    'datefmt': '%FT%T%:z',
                    'append': True
                }
            ]
        }
    ],
    'general': [
        {
            'check_search_path': ['tutorial/'],
        }
    ]
}
