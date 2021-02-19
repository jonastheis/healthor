import pandas as pd
import numpy as np
import subprocess
import os
import glob
import time
import datetime


OMNETPP_BASE_DIRECTORY = os.path.abspath('../simulation/')
OMNETPP_RESULTS_DIRECTORY = os.path.join(OMNETPP_BASE_DIRECTORY, 'results')


def compile_simulation():
    print('Compiling...')

    ret = subprocess.run([
        'make',
    ], cwd=OMNETPP_BASE_DIRECTORY, capture_output=True)
    if ret.returncode != 0:
        print(ret)
        exit(1)

    print('Compiling... done.')


def run_simulation(config_name):
    compile_simulation()

    start = time.time()
    print(format_time(time.time()), '| Simulation run %s...' % config_name)

    ret = subprocess.run([
        './simulation',
        # 'omnetpp.ini',
        # '-n', 'basic.ned',
        '-u', 'Cmdenv',
        '-c', config_name,
    ], cwd=OMNETPP_BASE_DIRECTORY, capture_output=True)
    if ret.returncode != 0:
        print(ret)
        exit(1)

    end = time.time()
    duration = end - start
    print(format_time(time.time()), '| %s duration |' % str(datetime.timedelta(seconds=duration)), 'Simulation run %s... done.' % config_name)


def export_to_csv(config_name):
    ret = subprocess.run([
        'scavetool', 'x', '%s-#0.vec' % config_name, '%s-#0.sca' % config_name, '-o', '%s.csv' % config_name,
    ], cwd=OMNETPP_RESULTS_DIRECTORY, capture_output=True)
    if ret.returncode != 0:
        print(ret)
        exit(1)


def parse_if_number(s):
    try: return float(s)
    except: return True if s=="true" else False if s=="false" else s if s else None


def parse_ndarray(s):
    return np.fromstring(s, sep=' ') if s else None


def parse_omnetpp_csv(config_name):
    path = os.path.join(OMNETPP_RESULTS_DIRECTORY, '%s.csv' % config_name)
    return pd.read_csv(path, converters={
        'value': parse_if_number,
        'attrvalue': parse_if_number,
        'binedges': parse_ndarray,
        'binvalues': parse_ndarray,
        'vectime': parse_ndarray,
        'vecvalue': parse_ndarray,
    })


def glob_csv_files(config_name, type):
    path = os.path.join(OMNETPP_RESULTS_DIRECTORY, '%s-%s_*.csv' % (config_name, type))
    return sorted(glob.glob(path))


def save_simulation_state(path):
    ret = subprocess.run([
        'tar', '-czvf', os.path.join(path, 'simulation.tar.gz'), OMNETPP_BASE_DIRECTORY,
    ], capture_output=True)
    if ret.returncode != 0:
        print(ret)
        exit(1)


def save_to_csv(df, path, name):
    df.to_csv(os.path.join(path, name + '.csv'))


def format_time(t):
    return time.strftime('%Y-%m-%d %H:%M:%S GMT', time.gmtime(t))
