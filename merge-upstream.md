###Set upstream to track EOSIO/eos
```$xslt
git remote add upstream https://github.com/EOSIO/eos.git
```

###Fetch the branches and their respective commits from the upstream repository. Commits to master will be stored in a local branch, upstream/master.
```$xslt
git fetch upstream release/1.1.x:release/1.1.x
```

###Check out your fork's local master branch.
```$xslt
git checkout master
```

###Merge the changes from upstream/master into your local master branch. This brings your fork's master branch into sync with the upstream repository, without losing your local changes.
```$xslt
git merge release/1.1.x
```

###Submodules update
If submodules need to upgraded, massages similar to the following will be displayed with `git status`
```$xslt
	modified:   libraries/chainbase (new commits)
	modified:   libraries/softfloat (new commits)
```
To update the submodules, go into the relative directory and reset head to commit_sha.
The commit_sha is the commit in the upstream repo.
```$xslt
cd chainbase/
git reset --hard 0a347683902f3ebbf58a4dd6166d68d967e240ce
cd softfloat
git reset --hard 9942875eb704369db297eb289aa4e9912bddcfb8
```
