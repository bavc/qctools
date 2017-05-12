# qctools

[![Join the chat at https://gitter.im/bavc/qctools](https://badges.gitter.im/bavc/qctools.svg)](https://gitter.im/bavc/qctools?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

QCTools (Quality Control Tools for Video Preservation) is a free and open source software tool that helps users analyze and understand their digitized video files through use of audiovisual analytics and filtering. QCTools is funded by the National Endowment for the Humanities and the Knight Foundation, and developed by the Bay Area Video Coalition.

Documentation on how to use QCTools is available within the application under the Help tab or online [here](http://bavc.github.io/qctools/).

# general info

QCTools allows archivists, curators, preservationists and other moving image professionals to identify, filter, and asssess all manner of video errors and anomalies. The tool is flexible, providing a variety of viewing options, as well the ability to create and export reports (gzip, or .gz files, formatted according to the ffprobe xml standard).

For more information about the project, or to download the latest Mac/Windows/Ubuntu release, please visit [BAVC's QCTools homepage](http://www.bavc.org/qctools)

An overview of QCTools and how to use it can be found [here.](http://bavc.github.io/qctools/)

# installation

## via installers

Go to http://bavc.org/qctools or [Releases](https://github.com/bavc/qctools/releases) and download QCTools for your operating system. Initiate the install by double-clicking the icon, and follow the steps. New releases of QCTools will be periodically available at the QCTools Project website.

## via homebrew

Of if you have homebrew, get the latest by running:
```
brew install amiaopensource/amiaos/qctools
brew linkapps qctools
```

## development builds

### via homebrew

Note that occasionally QCTools uses features from git-master of FFmpeg, thus you may need to update FFmpeg to git-master as well to try the newest pre-release QCTools features.

```
brew reinstall --HEAD ffmpeg --with-freetype
brew install --HEAD amiaopensource/amiaos/qctools
brew linkapps qctools
```

### via daily builds

Or grab one of the [QCTools daily builds](https://mediaarea.net/download/snapshots/binary/qctools/) provided by MediaArea. These builds will reflect git-master and are not tied to any particular release.

# getting started

Currently QCTools accepts a variety of video formats, including *.avi, *.mkv, *.mov, and *.mp4, as well as a diverse selection of codecs. By relying upon ffmpeg's libavcodec and libformat libraries, QCTools can support a wide variety of digital audiovisual formats.

# graph descriptions

QCTools offers a variety of [Graphing Options](http://bavc.github.io/qctools/filter_descriptions.html) including: YUV Values, Temporal Outliers (TOUT), Vertical Line Repetitions (VREP), Broadcast Range (BRNG), Crop Width and Height (CropW and CropH), and Peak Signal to Noise Ratio (PSNRf) and Mean Square Error (MSEf) differences per frame.

# playback filters

The QCTools preview window is intended as an analytical playback environment that allows the user to review video through multiple filters simultaneously. The playback window includes two viewing windows which may be set to different combinations of [filters](http://bavc.github.io/qctools/playback_filters.html).

# reading a qctools document

The [QCTools document](http://bavc.github.io/qctools/data_format.html) (built upon FFprobe's [xml expression](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/doc/ffprobe.xsd)), is designed to be self-descriptive, storing analytical metadata about video and audio frames.

Incorporating a set of open source libraries developed under the ffmpeg project, the QCTools document offers metadata values derived from four evaluative filters: [signalstats](https://www.ffmpeg.org/ffmpeg-filters.html#signalstats), [cropdetect](https://www.ffmpeg.org/ffmpeg-filters.html#toc-cropdetect), [psnr](https://www.ffmpeg.org/ffmpeg-filters.html#psnr), and [ebur128](https://www.ffmpeg.org/ffmpeg-filters.html#ebur128).

# using qcli

QCTools files can also be generated via the command line with `qcli`. After installing it, you can run `qcli -i [your-file-here]` to generate a qctools report based on your file. By default, this file will be saved to the same directory and named after the file, e.g. `test.qctools.xml.gz` if your file is named `test.mkv`. This can easily be wrapped in a script to create many qctools files during the preservation process.

`qcli` can be installed via homebrew with:

```
brew install qcli
```

# a/v artifact atlas

In conjunction with using QCTools, consider using the [A/V Artifacts Atlas](https://bavc.github.io/avaa/index.html) to gain further clarification and appropriate descriptive terminology for any anomalies or errors you might encounter in your video content.  Users are invited to contribute unidentified errors they come across to the atlas.

# contributing

Please read our contributing guidelines in [this dedicated document](https://github.com/bavc/qctools/blob/master/CONTRIBUTING.md).

# license

QCTools deliverable is licensed under a GPLv3 License.
QCTools GUI part and FFmpeg statistics filter are licensed under the 3-Clause BSD license.
This software uses libraries from the FFmpeg project under the GPLv3, Qt and Qwt libraries under the LGPLv2.1, OpenJPEG library under the 2-Clause BSD license. See our [License page](http://htmlpreview.github.io/?https://github.com/bavc/qctools/blob/master/License.html) for more details.
