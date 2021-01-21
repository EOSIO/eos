#!/usr/bin/env python3.9
# ds
# repository
# string:  repository_name
# mapping: submodules => commit hash

# get all repository urls using gh - https://github.com/EOSIO and https://github.com/b1-as
#   init each repo with it's own individual ds
# for each repository ds get all of the submodules in each repository and their respective commit hashes

## conform this script to the style that we use in the eosio/eos python tests

# get repository
  # get submodules
# main
 
# import os
# import sys
import pygithub3

#  
gh = None
#   
# def gather_clone_urls(account, no_forks=True):
#     try:
#         all_repos = gh.repos.list_by_org(account, type='all').all()
#     except pygithub3.exceptions.NotFound:
#         all_repos = gh.repos.list(user=account).all()
#  
#     for repo in all_repos:
#  
#         # Don't print the urls for repos that are forks.
#         if no_forks and repo.fork:
#             continue
#  
#         if os.environ.get('HTTP_URLS'):
#             yield repo.clone_url
#         else:
#             yield repo.ssh_url

def gather_clone_urls(organization):
    all_repos = gh.repos.list(user=organization).all()
    for repo in all_repos:
        if repo.fork:
            continue
        yield repo.clone_url
 
if __name__ == '__main__':
    gh = pygithub3.Github()
    
    eosio_organization = "EOSIO"

    # Dictionary consisting of: {organization => {repository_url => {submodule => submodule_hash}}}.
    repository_dictionary = {"EOSIO": "", "": ""}

    # Retrieve all repositories in the organization.
    # try:
    #     all_repos = gh.repos.list_by_org(account, type='all').all()
    # except pygithub3.exceptions.NotFound:
    #     all_repos = gh.repos.list(user=account).all()

    clone_urls = gather_clone_urls(eosio_organization)

    # Retrieve all submodules of all repositories in the organization.

    # Print the results.
    
    print(eosio_organization)
    print(repository_dictionary)
    for url in clone_urls:
        print(url)
    
    # argc = len(sys.argv) - 1
    # if argc < 1:
    #     print "Usage: [HTTP_URLS=1] python2.7 {} account_name [personal_token]".format(sys.argv[0])
    #     sys.exit()
    # else:
    #     account = sys.argv[1]
    # if argc > 1:
    #     token = sys.argv[2]
    #     gh = pygithub3.Github(token=token)
    # else:
    #     gh = pygithub3.Github()
    # 
    # clone_urls = gather_clone_urls(account=account)
    # for url in clone_urls:
    #     print url
