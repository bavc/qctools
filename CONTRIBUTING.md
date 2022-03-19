# Contributing

General non-code contributions can be made by way of filing bug reports in our [Issue tracker](https://github.com/bavc/qctools/issues). When filing a bug report, please be sure to note a description of the problem, your current QCTools version (listed under QCTools > About QCTools), your operating system, and steps that can be taken to reproduce this issue.

Please note that this project is released with a [Contributor Code of Conduct](CODE_OF_CONDUCT.md). By participating in this project you agree to abide by its terms.
 
# Contributing to Documentation

QCTools documentation lives within the application and is mirrored online [here](http://bavc.github.io/qctools/). The documentation is written in Markdown and converted to HTML automatically. Contributions can be made by cloning and forking the repository and editing the Markdown files found in the `/docs` folder.

Documentation updates can also be made via the Github website by clicking the pencil icon on the top right of a file's Github page. This will fork (make you a personal copy of) the repository and allow you to commit your changes all on the same page. You can also preview your proposed changes on this page.

After making changes, select "Create a new branch for this commit and start a pull request." There, you can describe the changes you want to make and a maintainer will review your changes and merge them into the source code. Documentation changes that have been merged can be seen nearly-immediately on the documentation webpage, but will not be applied to the QCTools application documentation until next release.

# Contributing to Source

QCTools is written in C++ using the Qt framework [under the GPLv3 License](https://github.com/bavc/qctools/blob/master/License.html).

## Testing your contributions locally

In case you don't have qmake version 5 run:  
`brew install qt5`  
`brew link --force qt5`  

Get a QCTools repo (or use your own copy):  

`git clone https://github.com/bavc/qctools.git`  
`cd qctools`  

Build QCTools via homebrew install process:  
`cd Project/QtCreator/`  
`export USE_BREW=true && qmake && make`  
`open QCTools.app`  

After testing, push your changes to your fork/branch and [submit a pull request](https://github.com/bavc/qctools/compare?expand=1) and describe your proposed changes. Then you will have to wait for a maintainer to review your code.
