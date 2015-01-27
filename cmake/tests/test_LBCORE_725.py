'''
Test to check the correct override order of projects.
'''
import os
from subprocess import Popen, PIPE

base_dir = os.path.join(os.path.dirname(__file__), 'data', 'LBCORE-725')
build_dir = os.path.join(base_dir, 'tmp')
release_dir = os.path.join(base_dir, 'dest')

build_log = None
build_returncode = None

def clean():
    import shutil
    shutil.rmtree(build_dir, ignore_errors=True)
    shutil.rmtree(release_dir, ignore_errors=True)

def build():
    global build_log, build_returncode
    build_proc = Popen([os.path.join(base_dir, 'prepare.sh')], cwd=base_dir,
                       stdout=PIPE, stderr=PIPE)
    build_log, _err = build_proc.communicate()

    build_returncode = build_proc.returncode

def setup():
    build()


def test_A_env():
    assert build_returncode == 0

    envfile = os.path.join(release_dir,
                           'A', 'InstallArea', 'A.xenv')
    assert os.path.exists(envfile), envfile

    getenv = Popen(['xenv', '--xml', envfile, '--py'],
                   stdout=PIPE, stderr=PIPE)
    out, _err = getenv.communicate()
    assert getenv.returncode == 0, getenv.returncode
    out = eval(out)

    assert out['A_PROJECT_ROOT'] == os.path.join(release_dir, 'A')

def test_B_env():
    assert build_returncode == 0

    envfile = os.path.join(release_dir,
                           'B', 'InstallArea', 'B.xenv')
    assert os.path.exists(envfile), envfile

    getenv = Popen(['xenv', '--xml', envfile, '--py'],
                   stdout=PIPE, stderr=PIPE)
    out, _err = getenv.communicate()
    assert getenv.returncode == 0, getenv.returncode
    out = eval(out)

    assert out['A_PROJECT_ROOT'] == os.path.join(release_dir, 'A')
    assert out['B_PROJECT_ROOT'] == os.path.join(release_dir, 'B')
    assert out['PKGAROOT'] == os.path.join(release_dir, 'DBASE', 'PkgA', 'v1r0')

def test_C_env():
    assert build_returncode == 0

    envfile = os.path.join(release_dir,
                           'C', 'build', 'config', 'C-build.xenv')
    assert os.path.exists(envfile), envfile

    getenv = Popen(['xenv', '--xml', envfile, '--py'],
                   stdout=PIPE, stderr=PIPE)
    out, _err = getenv.communicate()
    assert getenv.returncode == 0, getenv.returncode
    out = eval(out)

    assert out['A_PROJECT_ROOT'] == os.path.join(release_dir, 'A')
    assert out['B_PROJECT_ROOT'] == os.path.join(release_dir, 'B')
    assert out['PKGAROOT'] == os.path.join(release_dir, 'DBASE', 'PkgA', 'v1r0')


def teardown():
    clean()
