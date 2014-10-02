qctools
=======

QCTools (Quality Control Tools for Video Preservation) is a free and open source software tool that helps users analyze and understand their digitized video files through use of audiovisual analytics and filtering. QCTools is funded by the National Endownment for the Humanites and developed by the Bay Area Video Coalition.

general info
=======

Now in version 0.5.0, QCTools allows archivists, curators, preservationists and other moving image professionals to identify, filter, and asssess all manner of video errors and anomalies. The tool is flexible, providing a variety of viewing options, as well the ability to create and export reports (gzip, or .gz files, formatted according to the ffprobe xml standard). 

For more information about the project, or to download the latest Mac/Winddows/Ubuntu release, please visit [BAVC's QCTools homepage](http://www.bavc.org/qctools)

getting started
=======



Currently QCTools accepts a variety of video formats, including *.avi, *.mkv, *.mov, and *.mp4, as well as a diverse selection of codecs. Uncompressed 8-bit formats are ideal for the most accurate interpretation of the video. Relying upon ffmpeg's libavcodec and libformat libraries allows QCTools to support a wide variety of digital audiovisual formats.


filter descriptions
=======
[Filter Descrptions](http://htmlpreview.github.io/?https://github.com/bavc/qctools/blob/master/Source/Resource/Help/Filter%20Descriptions/Filter%20Descriptions.html)



reading a qctools document
=======
The [QCTools document](http://htmlpreview.github.io/?https://github.com/bavc/qctools/blob/master/Source/Resource/Help/Data%20Format/Data%20Format.html) (built upon FFprobe's [xml expression](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/doc/ffprobe.xsd)), is designed to be self-descriptive, storing analytical metadata about video and audio frames.

Incorporating a set of open source libraries developed under the ffmpeg project, the QCTools document offers metadata values derived from four evaluative filters: [signalstats](https://www.ffmpeg.org/ffmpeg-filters.html#signalstats), [cropdetect](https://www.ffmpeg.org/ffmpeg-filters.html#toc-cropdetect), [psnr](https://www.ffmpeg.org/ffmpeg-filters.html#psnr), and [ebur128](https://www.ffmpeg.org/ffmpeg-filters.html#ebur128).


