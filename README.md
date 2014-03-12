# SyntroCore

The core SyntroNet libraries and applications for running a SyntroNet cloud.

Check out www.richards-tech.com for more details.

#### Applications

* SyntroAlert
* SyntroControl
* SyntroDB
* SyntroExec
* SyntroLog

#### Libraries

* SyntroControlLib
* SyntroLib
* SyntroGUI

#### C++ Headers

* /usr/include/syntro

#### Package Config

* /usr/lib/pkgconfig/syntro.pc


### Build Dependencies

* Qt4 or Qt5 development libraries and headers
* pkgconfig

### Fetch

        git clone git://github.com/richards-tech/SyntroCore.git


### Build (Linux)

        qmake 
        make 
        sudo make install


After the install step on Linux you will be able to build and run other SyntroNet apps.

There are VS2010 solution files for building SyntroCore binaries on Windows.
They are configured for use with Qt5 libraries, but the code is Qt4 compatible.

### Run

Instructions for running the various SyntroCore applications can be found on http://richardstechnotes.wordpress.com
