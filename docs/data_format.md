# Reading a QCTools Document

The QCTools Document is designed to be a self-descriptive metadata document that stores analytical metadata about video and audio frames and may be quickly rendered in QCTools version 0.5 or later. The QCTools Document is an XML expression of FFprobe's XML expression as defined by the ffprobe schema at [http://www.ffmpeg.org/schema/ffprobe.xsd](http://www.ffmpeg.org/schema/ffprobe.xsd). For more information about FFprobe's XML expression please visit: [http://www.ffmpeg.org/schema/ffprobe.xsd](http://ffmpeg.org/ffprobe.html#xml); for additional information about FFprobe's XML expression: [http://ffmpeg.org/ffprobe.html#xml](http://ffmpeg.org/ffprobe.html#xml).

In addition to frame metadata, the QCTools Document stores metadata values output from four evaluative filters: signalstats, cropdetect, psnr, ssim, and ebur128.

1.  **signal stats**
    The signalstats filter is a project of the QCTools project. Signalstats outputs information about the range of values used in luminance and chrominance video planes as well as details of hue and saturation and pixel labeling tests such as vertical line repetition, temporal outliers, and broadcast range adherence. More info: [http://ffmpeg.org/ffmpeg-filters.html#signalstats.](http://ffmpeg.org/ffmpeg-filters.html#signalstats)
2.  **cropdetect**
    The cropdetect filter detects how a given frame may be cropped to remove black (or nearly black) borders from the edges of the video. It can detect if video has been letterboxed or pillarboxed or other bordering rows or columns of black pixels. More info: [http://ffmpeg.org/ffmpeg-filters.html#cropdetect.](http://ffmpeg.org/ffmpeg-filters.html#cropdetect)
3.  **psnr**
    Within the QCTools Document the psnr filter is used specifically to visually compare field 1 and field 2\. The filter accepts field 1 and field 2 as two separate inputs and provides quantification of the visual difference between those two images for each plane (Y, U, and V) as well as for the overall image. This data is presented in both Peak Signal to Noise Ratio (PSNR) and Mean Square Error (MSE). More info: [ffmpeg.org/ffmpeg-filters.html#psnr.](ffmpeg.org/ffmpeg-filters.html#psnr)
4.  **ssim**
    Within the QCTools Document the SSIM (Structural SImilarity Metric) between two input videos. The filter is used specifically to visually compare field 1 and field 2\. The filter accepts field 1 and field 2 as two separate inputs and provides quantification of the visual difference between those two images for each plane (Y, U, and V) as well as for the overall image. More info: [ffmpeg.org/ffmpeg-filters.html#ssim.](ffmpeg.org/ffmpeg-filters.html#ssim)
5.  **ebur128**
    The ebur128 filter is a volume detection filter that measures perceived loudness according to the EBU standard. More info: [http://ffmpeg.org/ffmpeg-filters.html#ebur128.](http://ffmpeg.org/ffmpeg-filters.html#ebur128)

The data format of a QCTools xml export contains two main sections:

### 1\. Library Versions

QCTools incorporates a set of open-source libraries, developed under the FFmpeg project, that were built to process audiovisual data. This section of the data format calls out the specific libraries referenced by QCTools. They include:

*   libavutil
*   libavcodec
*   libavformat
*   libavdevice
*   libavfilter
*   libswscale
*   libswresample
*   libpostproc

### 2\. Frames and Tags

The document is further divided into Frames and Tags per **media type**.

A [frame](https://github.com/bavc/FFmpeg/blob/master/doc/ffprobe.xsd#L55-L88) describes a particular point in the video or audio and has a set of attributes that describe and identify that point (or frame)--including media type, duration, size, position, etc.
A [tag](https://github.com/bavc/FFmpeg/blob/master/doc/ffprobe.xsd#L207-L210) contains a key--in this case a reference to the filter name-- and the value for that filter.

#### Media Type: Audio

The Audio frame element contains descriptive information that identifies a particular frame. The tag attributes include values for the following filters (based on EBU R 128):

*   **lavfi.r128.M**: or 'Momentary loudness' reflects sudden changes in volume over brief intervals of time (up to 400ms)
*   **lavri.r128.S**: 'Short term loudness', measured in intervals of up to 3s in duration
*   **lavfi.r128.I**: 'Integrated loudness', or overall loudness occurring over the entire duration of the audiovisual asset, or from 'start to stop'
*   **lavfi.r128.LRA**: 'Loudness Range' quantifies the variation across loudness; measured in units of LU (1 LU is equal to 1 dB)
*   **lavfi.r128.LRA.low**
*   **lavfi.r128LRA.high**

#### Media Type: Video

Each frame element contains descritpive information that identifies a particular point in the video. The tag attributes present the various QCTools filters and their values for that frame. For example:
<tag = key "lavfi.signalstats.YMIN" value = "15"/>
<tag = key "lavfi.signalstats.YLOW" value = "18"/>
For more info, please see **Filter Descriptions.**

### Creating a QCTools Document

The QCTools Document can be created in either of the following ways:

1.  Via QCTools (version 0.5 or later)
    Open a video file for evaluation and then select "Export > To qctools.xml.gz".
2.  Via FFmpeg (version 2.3 or later)
    For a file with video and audio named EXAMPLE.mov
    ffprobe -f lavfi -i "movie=EXAMPLE.mov:s=v+a[in0][in1],[in0]signalstats=stat=tout+vrep+brng,cropdetect=reset=1:round=1,idet=half_life=1,split[a][b];[a]field=top[a1];[b]field=bottom,split[b1][b2];[a1][b1]psnr[c1];[c1][b2]ssim[out0];[in1]ebur128=metadata=1,astats=metadata=1:reset=1:length=0.4[out1]" -show_frames -show_versions -of xml=x=1:q=1 -noprivate | gzip > EXAMPLE.mov.qctools.xml.gz
    For a file with video and no audio named EXAMPLE.mov
    ffprobe -f lavfi -i "movie=EXAMPLE.mov,signalstats=stat=tout+vrep+brng,cropdetect=reset=1:round=1,idet=half_life=1,split[a][b];[a]field=top[a1];[b]field=bottom,split[b1][b2];[a1][b1]psnr[c1];[c1][b2]ssim" -show_frames -show_versions -of xml=x=1:q=1 -noprivate | gzip > EXAMPLE.mov.qctools.xml.gz

### About gzip Compression

Because of the large volume of data contained in a QCTools document, it is necessary to compress the XML file to ensure it is a manageable size. QCTools uses a compression software called gzip to do this ([en.wikipedia.org/wiki/Gzip](http://en.wikipedia.org/wiki/Gzip)) which accounts for the .gz file extension.

### Additional Resources

For further information about FFmpeg, EBU audio specifications, etc., please see:  
**FFmpeg /** [ffmpeg.org](https://ffmpeg.org)  
**FFmpeg Documentation /** [http://ffmpeg.org/documentation.html](http://ffmpeg.org/documentation.html)  
**EBU 3341, Loudness Metering: 'EBU Mode' metering to supplement loudness normalizqation in accordance with EBU R 128 /** [https://tech.ebu.ch/docs/tech/tech3341.pdf](https://tech.ebu.ch/docs/tech/tech3341.pdf)  
**EBU R 128, Loudness Normalisation and permitted maximum level of audio signals /** [https://tech.ebu.ch/docs/r/r128-2014.pdf](https://tech.ebu.ch/docs/r/r128-2014.pdf)  
**GZIP /** [www.gzip.org](http://www.gzip.org/)  
