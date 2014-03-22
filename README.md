# SyntroCore

The core SyntroNet libraries and applications.

Check out www.richards-tech.com for more details.

#### Applications

* SyntroAlert
* SyntroControl
* SyntroDB

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

### Build (Windows)

There are Visual Studio solution files for building SyntroCore binaries on Windows. They are configured for use with Qt5 libraries, but the code is Qt4 compatible.

Visual Studio will need to have th Qt plugin installed. Download the plugin from here https://qt-project.org/downloads.

There are a couple of changes required to the environment (accessed via the control panel, Advanced system settings and pressing the Environment Variables button.

Add an environment variable SYNTRODIR to be the location of the repos. By convention this would be c:\SyntroNet. Then add:

	;%SYNTRODIR%\bin

to the end of the PATH variable.

To build the libraries, open the BuildLibs solution, select Build -> Batch build. Press the Select All and Build. This will build the libraries and install them to the correct locations.

To build the core apps, open the All solution. This allows all the core apps to be compiled at the same time.

### Run

Instructions for running the various SyntroCore applications can be found on http://richardstechnotes.wordpress.com
