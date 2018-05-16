# suidsh

## Compile

``` ksh
mkdir build ; cd build ; cmake ..
make
```

## Install

``` ksh
cd build ; make install
```

## Configure
Add an entry for the script to the suidsh.conf file in /etc.
Lines which begin with the # character denote comments, blank lines are skipped.
Each line contains 4 fields, the fully qualified path to the wrapper, the fully qualified path to the actual script, the allowed setuid user, and the allowed setgid group.
Also if the script is to be setgid only, then the user name should be given as '-'. If the script is not to be setgid the group may be specified as '-' or omitted. 

Once the configuration file has been updated, the script file should have the owner and group changed to reflect what was put in the suidsh.conf file.
It should also have the setuid/setgid bit set.

A symbolic link is now needed from the suidsh binary to the wrapper name.

## Example

Assuming that suidsh is installed in /usr/local/bin/suidsh and the setuid user will be funcuser1:

/usr/local/bin/myscript.sh:

``` ksh
#!/bin/ksh

echo "I'm a test script"
```

/etc/suidsh.conf:

``` text
/usr/local/bin/myscript /usr/local/bin/myscript.sh funcuser1 -
```

Create symbolic link for wrapper:

``` ksh
ln -s /usr/local/bin/suidsh /usr/local/bin/myscript
```

