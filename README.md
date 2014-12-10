qctools
=======

QCTools (Quality Control Tools for Video Preservation) is a free and open source software tool that helps users analyze and understand their digitized video files through use of audiovisual analytics and filtering. QCTools is funded by the National Endownment for the Humanites and developed by the Bay Area Video Coalition.

Our (in progress) wiki is starting to develop [here](https://github.com/bavc/qctools/wiki).

general info
=======

Now in version 0.5.0, QCTools allows archivists, curators, preservationists and other moving image professionals to identify, filter, and asssess all manner of video errors and anomalies. The tool is flexible, providing a variety of viewing options, as well the ability to create and export reports (gzip, or .gz files, formatted according to the ffprobe xml standard). 

For more information about the project, or to download the latest Mac/Winddows/Ubuntu release, please visit [BAVC's QCTools homepage](http://www.bavc.org/qctools)

An overview of QCTools and how to use it can be found [here.] (http://htmlpreview.github.io/?https://github.com/bavc/qctools/blob/master/Source/Resource/Help/How%20To%20Use/How%20To%20Use.html) 


installation
=======
Go to http://bavc.org/qctools and download QCTools for your operating system. Initiate the install by double-clicking the icon, and follow the steps. New releases of QCTools will be periodically available at the QCTools Project website.


getting started
=======
Currently QCTools accepts a variety of video formats, including *.avi, *.mkv, *.mov, and *.mp4, as well as a diverse selection of codecs. Uncompressed 8-bit formats are ideal for the most accurate interpretation of the video. By relying upon ffmpeg's libavcodec and libformat libraries, QCTools can support a wide variety of digital audiovisual formats.


filter descriptions
=======

QCTools offers a variety of [Filtering Options](http://htmlpreview.github.io/?https://github.com/bavc/qctools/blob/master/Source/Resource/Help/Filter%20Descriptions/Filter%20Descriptions.html) including: YUV Values, Temporal Outliers (TOUT), Vertical Line Repetitions (VREP), Broadcast Range (BRNG), Crop Width and Height (CropW and CropH), Peak Signal to Noise Ratio (PSNRf) and Mean Square Error (MSEf).

playback filters
=======
The QCTools preview window is intended as an analytical playback environent that allows the user to review video through multiple filters simultaneously. The playback window includes two viewing windows which may be set to different combinations of [filters] (http://htmlpreview.github.io/?https://github.com/bavc/qctools/blob/master/Source/Resource/Help/Playback%20Filters/Playback%20Filters.html).

reading a qctools document
=======
The [QCTools document](http://htmlpreview.github.io/?https://github.com/bavc/qctools/blob/master/Source/Resource/Help/Data%20Format/Data%20Format.html) (built upon FFprobe's [xml expression](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/doc/ffprobe.xsd)), is designed to be self-descriptive, storing analytical metadata about video and audio frames.

Incorporating a set of open source libraries developed under the ffmpeg project, the QCTools document offers metadata values derived from four evaluative filters: [signalstats](https://www.ffmpeg.org/ffmpeg-filters.html#signalstats), [cropdetect](https://www.ffmpeg.org/ffmpeg-filters.html#toc-cropdetect), [psnr](https://www.ffmpeg.org/ffmpeg-filters.html#psnr), and [ebur128](https://www.ffmpeg.org/ffmpeg-filters.html#ebur128).


a/v artifact atlas
=======
Once you check your files using QCTools, consider using the [A/V Artifacts Atlas](http://avaa.bavc.org/artifactatlas/index.php/A/V_Artifact_Atlas) to gain further clarification and appropriate descriptive terminology for any anomalies or errors you might encounter in your video content.  Users are invited to contribute unidentified errors they come across to the atlas.

license
=======
QCTools deliverable is licensed under a GPLv3 License.  
QCTools GUI part and FFmpeg statistics filter are licensed under the 3-Clause BSD license.  
This software uses libraries from the FFmpeg project under the GPLv3, Qt and Qwt libraries under the LGPLv2.1, OpenJPEG library under the 2-Clause BSD license.  
