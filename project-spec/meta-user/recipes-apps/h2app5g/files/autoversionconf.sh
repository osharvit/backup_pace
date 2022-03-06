#!/bin/bash

MANAGEDLIST="h2a0/version.cpp h2b0emu102/version.cpp src/aboutdialog.cpp cppsrc/aboutdialog.cpp src/version.py pysrc/version.py wix/version.wxi"

gen_gitignore()
{
  echo "*.*_"   > .gitignore
  echo "*.*sdf" >> .gitignore
  echo "*.a"    >> .gitignore
  echo "*.aps"  >> .gitignore
  echo "*.APS"  >> .gitignore
  echo "*.bin"  >> .gitignore
  echo "*.cache"        >> .gitignore
  echo "*.cmd"  >> .gitignore
  echo "*.coe"  >> .gitignore
  echo "*.d"    >> .gitignore
  echo "*.dat"  >> .gitignore
  echo "*.dep"  >> .gitignore
  echo "*.dll"  >> .gitignore
  echo "*.elf"  >> .gitignore
  echo "*.exe"  >> .gitignore
  echo "*.fig"  >> .gitignore
  echo "*.flags"        >> .gitignore
  echo "*.json" >> .gitignore
  echo "*.ko"   >> .gitignore
  echo "*.lib"  >> .gitignore
  echo "*.lnk"  >> .gitignore
  echo "*.log"  >> .gitignore
  echo "*.map"  >> .gitignore
  echo "*.mod.c"        >> .gitignore
  echo "*.msi"  >> .gitignore
  echo "*.ncb"  >> .gitignore
  echo "*.o"    >> .gitignore
  echo "*.obj"  >> .gitignore
  echo "*.opt"  >> .gitignore
  echo "*.paf2" >> .gitignore
  echo "*.pdb"  >> .gitignore
  echo "*.plg"  >> .gitignore
  echo "*.pyc"  >> .gitignore
  echo "*.rpt"  >> .gitignore
  echo "*.sbl"  >> .gitignore
  echo "*.sim"  >> .gitignore
  echo "*.spec"  >> .gitignore
  echo "*.stackdump"    >> .gitignore
  echo "*.suo"  >> .gitignore
  echo "*.swp*" >> .gitignore
  echo "*.tlog" >> .gitignore
  echo "*.user" >> .gitignore
  echo "*_autogen*"     >> .gitignore
  echo "*_install"      >> .gitignore
  echo "*_setup"        >> .gitignore
  echo "*~*"    >> .gitignore
  echo "*copy*" >> .gitignore
  echo "*debug*"        >> .gitignore
  echo "*Debug*"        >> .gitignore
  echo "*DEBUG*"        >> .gitignore
  echo "*-install"      >> .gitignore
  echo "*old"   >> .gitignore
  echo "*release*"      >> .gitignore
  echo "*Release*"      >> .gitignore
  echo "*RELEASE*"      >> .gitignore
  echo "*-setup"        >> .gitignore
  echo "*zip"   >> .gitignore
  echo ".depend"        >> .gitignore
  echo ".deps"  >> .gitignore
  echo ".git"   >> .gitignore
  echo ".gvimrc"        >> .gitignore
  echo ".idea"  >> .gitignore
  echo ".svn"   >> .gitignore
  echo ".vimrc" >> .gitignore
  echo "_build" >> .gitignore
  echo "_out*"  >> .gitignore
  echo "~*"     >> .gitignore
  echo "about*.cpp"     >> .gitignore
  echo "Backup" >> .gitignore
  echo "build"  >> .gitignore
  echo "cvs"    >> .gitignore
  echo "CVS"    >> .gitignore
  echo "dist"   >> .gitignore
  echo "doxy"   >> .gitignore
  echo "env.mk" >> .gitignore
  echo "errorlog.html"  >> .gitignore
  echo "html"   >> .gitignore
  echo "ipch"   >> .gitignore
  echo "labview/src/builds"     >> .gitignore
  echo "latex"  >> .gitignore
  echo "lib/*.lib"      >> .gitignore
  echo "obj"    >> .gitignore
  echo "out"    >> .gitignore
  echo "out_*"  >> .gitignore
  echo "rtf"    >> .gitignore
  echo "tags"   >> .gitignore
  echo "TAGS"   >> .gitignore
  echo "temp"   >> .gitignore
  echo "tmp"    >> .gitignore
  echo "UpgradeLog.htm" >> .gitignore
  echo "venv"   >> .gitignore
  echo "vs*-*/*.cfg"    >> .gitignore
  echo "vs*-*/*.inv"    >> .gitignore
  echo "vs*-*/*.txt"    >> .gitignore
  echo "vs*-*/*.xml"    >> .gitignore
  echo "win32"  >> .gitignore
  echo "WIN32"  >> .gitignore
  echo "wix"    >> .gitignore
  echo "x64"    >> .gitignore
  echo "X64"    >> .gitignore

  git add .gitignore
}

update_gitattributes()
{
  if [ -f .gitattributes ]
  then
    # remove all lines include the "filter=git_log_version"
    sed -i "/filter=git_log_version/d" .gitattributes
  else
    # create the ".gitattributes" file
    touch .gitattributes
  fi

  for managedfile in ${MANAGEDLIST}
  do
    if [ -f ${managedfile} ]
    then
      echo "${managedfile} filter=git_log_version"       >> .gitattributes
    fi
  done

  git add .gitattributes
}

gen_hooks()
{
  echo "#!/bin/sh"    > .git/hooks/post-commit
  for managedfile in ${MANAGEDLIST}
  do
    if [ -f ${managedfile} ]
    then
      echo "touch ${managedfile}; git checkout -- ${managedfile}" >> .git/hooks/post-commit
    fi
  done
  chmod +x .git/hooks/post-commit

  cp .git/hooks/post-commit .git/hooks/post-merge
  chmod +x .git/hooks/post-merge
}

# 1) create the ".gitignore" if it does not exist.
if [ ! -f .gitignore ]
then
  gen_gitignore
fi

# 2~3) reset version strings
for managedfile in ${MANAGEDLIST}
do
  if [ -f ${managedfile} ]
  then
    sed -i "s/\(GIT_LOG_COUNT_VERSION\)\ *=\ *\".*\"\ */\1 = \"0\"/g"   ${managedfile}
    sed -i "s/\(GIT_COMMIT_HASH_VERSION\)\ *=\ *\".*\"\ */\1 = \"\"/g"  ${managedfile}
    sed -i "s/\(COMMIT_DATE\)\ *=\ *\".*\"\ */\1 = \"\"/g"              ${managedfile}
  fi
done

# 5~6) add both smudge and clean filters to gitconfig
git config filter.git_log_version.smudge 'sed -e "s/\(GIT_LOG_COUNT_VERSION\)\ *=\ *\".*\"\ */\1 = \"`git log --oneline | wc -l`\"/g" | sed -e "s/\(GIT_COMMIT_HASH_VERSION\)\ *=\ *\".*\"\ */\1 = \"`git log --pretty=format:"%h" -1`\"/g" | sed -e "s/\(COMMIT_DATE\)\ * = \ *\".*\"\ */\1 = \"`git log --pretty=format:"%ai" -1`\"/g"'
git config filter.git_log_version.clean  'sed -e "s/\(GIT_LOG_COUNT_VERSION\)\ *=\ *\".*\"\ */\1 = \"0\"/g" | sed -e "s/\(GIT_COMMIT_HASH_VERSION\)\ *=\ *\".*\"\ */\1 = \"\"/g" | sed -e "s/\(COMMIT_DATE\)\ *=\ *\".*\"\ */\1 = \"\"/g"'

# 7) update the ".gitattributes"
update_gitattributes

# 8~9) edit or create ".git/hooks/post-commit" as below
gen_hooks

# 
for managedfile in ${MANAGEDLIST}
do
  if [ -f ${managedfile} ]
  then
    sed -i "s/\(GIT_LOG_COUNT_VERSION\)\ *=\ *\".*\"\ */\1 = \"`git log --oneline | wc -l`\"/g" ${managedfile}
    sed -i "s/\(GIT_COMMIT_HASH_VERSION\)\ *=\ *\".*\"\ */\1 = \"`git log --pretty=format:"%h" -1`\"/g" ${managedfile}
    sed -i "s/\(COMMIT_DATE\)\ * = \ *\".*\"\ */\1 = \"`git log --pretty=format:"%ai" -1`\"/g" ${managedfile}

    cat ${managedfile}
  fi
done

