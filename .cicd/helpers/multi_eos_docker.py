#!/usr/bin/env python

import configparser
import os
import re
import requests
import shutil
import subprocess
import sys
import tarfile


def is_tag(ref):
    regex = re.compile('v[0-9]+\.[0-9]+\..*')
    return regex.match(ref)

def get_commit_for_branch(branch):
    commit = None
    r = requests.get('https://api.github.com/repos/EOSIO/eos/git/refs/heads/{}'.format(branch))
    if r.status_code == 200:
        commit = r.json().get('object').get('sha')

    return commit

def get_commit_for_tag(tag):
    commit = None
    r = requests.get('https://api.github.com/repos/EOSIO/eos/git/refs/tags/{}'.format(tag))
    if r.status_code == 200:
        commit = r.json().get('object').get('sha')

    return commit

def sanitize_label(label):
    invalid = [' ', '..', '.', '/', '~', '=']
    for c in invalid:
        label = label.replace(c, '-')

    return label.strip('-')

CURRENT_COMMIT = os.environ.get('BUILDKITE_COMMIT')
BK_TOKEN = os.environ.get('BUILDKITE_API_KEY')
if not BK_TOKEN:
    sys.exit('Buildkite token not set')
headers = {
    'Authorization': 'Bearer {}'.format(BK_TOKEN)
}

try:
    shutil.rmtree('builds')
except OSError:
    pass
os.mkdir('builds')

existing_build_found = False
if CURRENT_COMMIT:
    print 'Attempting to get build directory for this branch ({})...'.format(CURRENT_COMMIT)
    r = requests.get('https://api.buildkite.com/v2/organizations/EOSIO/pipelines/eosio/builds?commit={}'.format(CURRENT_COMMIT), headers=headers)
    if r.status_code == 200:
        resp = r.json()
        if resp:
            build = resp.pop(0)
            for job in build.get('jobs'):
                job_name = job.get('name')
                if job_name == ':ubuntu: 18.04 Build':
                    dir_r = requests.get(job.get('artifacts_url'), headers=headers)
                    if dir_r.status_code == 200:
                        download_url = dir_r.json().pop().get('download_url')
                        if download_url:
                            dl_r = requests.get(download_url, headers=headers)
                            open('current_build.tar.gz', 'wb').write(dl_r.content)
                            tar = tarfile.open('current_build.tar.gz')
                            tar.extractall(path='builds/current')
                            tar.close()
                            os.remove('current_build.tar.gz')
                            existing_build_found = True
        else:
            print 'No builds found for this branch ({})'.format(CURRENT_COMMIT)

if not os.path.exists('../tests/multiversion.conf'):
    sys.exit('Unable to find config file')

commits = {}
config = configparser.ConfigParser()
config.read('../tests/multiversion.conf')
for item in config.items('eosio'):
    label = sanitize_label(item[0])
    if is_tag(item[1]):
        commits[label] = get_commit_for_tag(item[1])
    elif get_commit_for_branch(item[1]):
        commits[label] = get_commit_for_branch(item[1])
    else:
        commits[label] = item[1]

PWD = os.getcwd()
config_path = '../tests/multiversion_paths.conf'
if existing_build_found:
    config_path = '{0}/builds/current/build/tests/multiversion_paths.conf'.format(PWD)

with open(config_path, 'w') as fp:
    fp.write('[eosio]\n')
    for label in commits.keys():
        fp.write('{}={}/builds/{}/build\n'.format(label, PWD, label))

print 'Getting build data...'
artifact_urls = {}
for label, commit in commits.items():
    r = requests.get('https://api.buildkite.com/v2/organizations/EOSIO/pipelines/eosio/builds?commit={}'.format(commit), headers=headers)
    if r.status_code == 200:
        resp = r.json()
        if resp:
            build = resp.pop(0)
            for job in build.get('jobs'):
                job_name = job.get('name')
                if job_name == ':ubuntu: 18.04 Build':
                    artifact_urls[label] = job.get('artifacts_url')
        else:
            print 'No builds found for {}'.format(label)
    else:
        print r.text
        sys.exit('Something went wrong getting build data for {}'.format(label))

print 'Downloading and extracting build directories...'
for label, artifact_url in artifact_urls.items():
    print ' +++ {}'.format(label)
    r = requests.get(artifact_url, headers=headers)
    if r.status_code == 200:
        download_url = r.json().pop().get('download_url')
        if download_url:
            dl_r = requests.get(download_url, headers=headers)
            open('{}_build.tar.gz'.format(label), 'wb').write(dl_r.content)
            tar = tarfile.open('{}_build.tar.gz'.format(label))
            tar.extractall(path='builds/{}'.format(label))
            tar.close()
            os.remove('{}_build.tar.gz'.format(label))
        else:
            sys.exit('Unable to get artifact download url for {}'.format(label))
    else:
        sys.exit('Something went wrong getting artifact data for {}'.format(label))
