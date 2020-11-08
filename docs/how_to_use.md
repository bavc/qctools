# How to Use

*   [Install](#install)
*   [Preferences](#preferences)
*   [Load Video Files](#load)
*   [Select Graph Filters](#select)
*   [View and Navigate Graphs](#view)
*   [Bar or Graph Charts](#bar)
*   [Playback and Visual Analysis](#playback)
*   [Create/Export a Report](#create)
*   [Parts of the Tool](#parts)
*   [Data Analysis Window](#data)
*   [List View](#list)
*   [Video Analysis Window](#analysis)
*   [Comments](#comments)

## Install {#install}

Visit [https://bavc.org](https://bavc.org/preserve-media/preservation-tools) or [QCTools on Github](https://github.com/bavc/qctools) and download QCTools for your operating system (currently Windows, Mac OS X, and many Linux-based operating systems are supported). New releases of QCTools will be periodically available at BAVC. We encourage any issues, bugs, or ideas for QCTools to be submitted via our [issue tracker](https://github.com/bavc/qctools/issues).

## Preferences {#preferences}

QCTools provides a Preferences window to configure settings for running QCTools.

### Filters

QCTool's analysis methods depend on filters from FFmpeg's libavfilter library. The "Filters" tab allows filters to be enabled or disabled. Currently this includes:

| filter name | track type | application in QCTools |
| [signalstats](http://ffmpeg.org/ffmpeg-filters.html#signalstats) | video | The `signalstats` analysis filter generates data to plot statistics on video signal levels, frame-to-frame differences, saturation and hue averages, and quantifications of visual patterns and errors. It is highly recommended to enable this filter. |
| [cropdetect](http://ffmpeg.org/ffmpeg-filters.html#cropdetect) | video | The cropdetect filter is used to determine how many columns and rows of nearly-black pixels border the visual image of the frame. The filter can detect changes in framing, letterboxing, pillarboxing, and adjustments to the aperture of the image. |
| [PSNR](http://ffmpeg.org/ffmpeg-filters.html#psnr) | video | The PSNR filter is used in QCTools specifically to generate a comparison between the two fields of the frame. The resulting data documents how different the fields are (which can find head-clogs or other videotape playback errors). |
| [SSIM](http://ffmpeg.org/ffmpeg-filters.html#ssim) | video | The SSIM filter is used in QCTools specifically to generate a comparison between the two fields of the frame. The resulting data documents how different the fields are (which can find head-clogs or other videotape playback errors). It is similar to PSNR but usually a different visual comparison algorithm. |
| [astats](http://ffmpeg.org/ffmpeg-filters.html#astats) | audio | The `astats` filter compiles statistics on audio data for small units of time, including maximum and minimum audio levels, DC Offset, the amount of consecutive audio sample differences, and RMS data. |
| [aphasemeter](http://ffmpeg.org/ffmpeg-filters.html#aphasemeter) | audio | The audio phase value represents the mean phase of current audio frame. Value is in range [-1, 1]. The -1 means left and right channels are completely out of phase and 1 means channels are in phase. |
| [EBU R.128](http://ffmpeg.org/ffmpeg-filters.html#ebur128) | audio | The EBU R.128 filter provides data on the perceived loudness of audio volume. |

Enabling all filters naturally provides more data, but results in a slower analysis and larger files. The EBU R.128 values represent perceived volume whereas the `astats` filters include metrics on actual volume (so the use of EBU R128 may not be considered essential if `astats` is in use). Additionally `PSNR` and `SSIM` both cover similar metrics by quantifying the difference between the two fields of the frame (the image of the odd-numbered lines vs the image of the even-numbered lines); `SSIM` is recommended.

### Panels

QCTools generates panels of images that are stacked horizontal to depict the timeline of audiovisual content in various ways. The panel tracks will reveal an image where the x-axis (viewing from left to right) will represent the timeline of the audiovisual content and the y-axis (viewing from top to bottom) is conditionally determined by the panel.

| filter name | track type | application in QCTools |
| Tiled Center Column | video | This image shows the center column from each subsequent image of the video timeline. |
| Tiled Center Column (Field Split) | video | Similar to 'Tiled Center Column'; however, the image is adjusted so that odd lines of video (the top field of an interlaced video) are shown at the top of the image and the even lines of video (the bottom field) are shown at the bottom. |
| Tiled Center Row | video | This image shows the center row from each subsequent image of the video timeline. The images are rotated 90 degrees counter-clockwise in the panel viewer, so the left edge of the frame's central row is on the bottom of the panel image and the right edge is at the top. |
| Horizontal Blur | video | Each column of this panel shows a variation of a Laplacian convolve of the image rotated 90 degrees counter clockwise. The lower part of the panel image depicts the left part of the source image and the top part of the panel image depicts the right part of the source image. In the panel image, brighter pixels depict content that is more in focus while darker pixels represent content that is flat or blurry. |
| Audio Waveform (Linear) | audio | This is a visualization of the audio waveform on a linear plot, each channel of the track is stacked upon one another. |
| Audio Waveform (Logarithmic) | audio | This is a visualization of the audio waveform on a logarithmic plot, each channel of the track is stacked upon one another. |
| Audio Normalized Cross-correlation | audio | This image shows the normalized cross-correlation between two channels of the audio track. Very high values (in green) show highly correlated audio, whereas very low values show highly correlated audio that is out of phase, and values in the center show that the audio is not correlated. |
| Audio Histogram | audio | A volume histogram for the audio track. |

### Tracks

The 'Tracks' Preference pane allows the user to set if they would like to analyze only the first track or all tracks of video and audio. Setting QCTools to analyze only the first track will result in a faster analyze but the other tracks would be ignored.

### Signalserver

QCTools now offers [SignalServer](https://github.com/bavc/signalserver) integration, allowing users to automatically or manually upload QCTools Reports as they are created by the application. Detailed SignalServer installation instructions can be found [here](https://github.com/bavc/signalserver/blob/master/README.md). Installation will vary based upon your specific technical infrastructure. Though designed for a Linux server environment, SignalServer can be installed on a local computer via [Docker](https://www.docker.com/).

Once SignalServer has been [installed](https://github.com/bavc/signalserver/blob/master/README.md) and an account on the server has been created, you’ll need to connect it to QCTools. Under the SignalServer tab, fill in the following: the unique URL of your SignalServer instance, your username, and your password.

![SignalServer view](media/signalserver_preferences.png)

## Load Video Files {#load}

QCTools can currently accommodate several video file types including QuickTime, MXF, AVI, Matroska, MP4, and many other audiovisual file formats. Once the QCTools application is open you may identify selected video files for QCTools analysis in three ways:

1.  Simply drag a video file (or files) into the QCTools window. Note that you may import multiple files at once into the tool, though depending on the number and size of the files, processing speed may be affected.
2.  Double click the folder icon on the upper left. This launches a window from which you can browse and select video files.
3.  Navigating to File-->Open.

As files are opened QCTools will begin immediately processing them. This involves creating thumbnails, decoding audiovisual data, and analyzing that data through FFmpeg's signalstats filter.

## Select Graph Filters {#select}

By clicking the graph checkboxes you can select particular audiovisual metrics that you wish to analyze and display. You may make these selections before uploading your video or at any time after the QCTools analysis has been done and the graph display will update dynamically. The checkboxes that enable/disable the graphs can also be dragged/dropped left or right which will reorder the presentation of the graphs accordingly. As a default, 'Y values', 'U values', 'V values', 'Diffs', and 'Sat' (saturation) are selected. To begin, you may also want to select the **Temporal Outliers** (tout) Graph Filter. This will detect any large discrepancies between pixels and can provide an initial, high-level overview of potential errors.

For descriptions of each Filter and how to read graph values, please see the Help Section, denoted by the '?' icon in the toolbox portion of the application.

## View and Navigate Graphs {#view}

Graphs display on the top portion of the screen, corresponding video thumbnails show below. The video frames may be navigated via the next, previous, or playback buttons; the frame and time for the particular selection will be displayed. Clicking and dragging your cursor over a portion of the graph will cause the thumbnails below to update accordingly. You may also double click a specific thumbnail and the playback window will appear displaying the image and with a variety of analytical playback filters.

Scrubbing your cursor over a particular point on a graph will reveal the corresponding thumbnails in the thumbnail bar along with the frame number and the timestamp of the particular place in the video you have navigated to.

You may also use the '+' and '-' icons in the tool box section of the application to zoom in/out, giving you a more or less detailed view of the graph displays, over a specific timespan of the uploaded video.

You can also 'play' the window within the graph view. When playing the graph's cursor which shows the currently selected frame will scroll to the right while the corresponding thumbnails update in the thumbnail bar below.

Some helpful **keyboard shortcuts** you may want to use are:

*   `'f'` - To enlarge the window to full screen
*   `'j'` - To rewind
*   `'k'` - To pause
*   `'l'` - To Fast Forward
*   `'m'` - Add a comment
*   space bar - Toggle between play and pause
*   left - Select the previous frame
*   right - Select the next frame
*   `CTRL+,` (Mac: `command+,`) - Jump to previous comment frame
*   `CTRL+.` (Mac: `command+.`) - Jump to next comment frame
*   `CTRL+C` (Mac: `command+C`) - Copy the current timestamp to the clipboard
*   `CTRL` + mouse-wheel (Mac: `command` + mouse-wheel) - Zooms in or out on Y axis
*   `Shift` + mouse-wheel - Zooms in or out on X axis

## Bar or Graph Charts {#bar}

In the upper navigation panel, users can select barchart profiles, with a default profile provided. New profiles may be added, deleted, and edited by clicking on the "gear" icon to the right of the profile setting dropdown.

On the right-hand side, users have the ability to toggle between the default graph view and the bar view. Settings may be configured using the "gear" icon to the right of the toggle button.

Within the barchart settings, the following values (and definitions) are available for building assertions:  

* y = y value of chart 
* yHalf = (2^bitdepth) / 2
* pow2 = pow2(exponent)
* pow = pow(base, exponent)
* maxval = 2^bitdepth
* minval = 0
* broadcastmaxval = 235 * (2^(bitdepth - 8))
* broadcastminval = 16 * (2^(bitdepth - 8))

Hovering over the conditions bar will provide a tool tip with these values, as well as their calculated values for the current file. These values will be different depending on the bit depth of the video.

Multiple conditions can be set by clicking the plus sign box to the far right of each category. To set multiple conditions on one line, use the double-ampersand (`&&`) between conditions.

If the equation is appropriate, it will display in green. If the equation isn't valid, it'll appear in red and return an error message on hover.

Barchart colors can be modified by clicking on the associated color box. 

Settings can also be configured to hide spikes caused by scene cuts by checking the "Hide spikes" checkbox.

## Playback and Visual Analysis {#playback}

By clicking on a thumbnail, you can open the preview window. The preview window serves as a playback environment that allows spot checking and manual video analysis. The preview window contains two playback windows that can be set to various selections; the filters allow the video to be processed in one of many ways which may help make particular issues more discernible. See the **Playback Filters** page for more details on these playback filters.

When playing back a media file please note that QCTools will only render the video. The audio may be visualized in one of the Audio Playback Filters but the audio will not be presently aurally.

Under situations where QCTools is not able to play back the video in real-time (for instance because the video is very large or the processing power available is not sufficient), there are options under the "View" toolbar menu to determine how playback should be prioritized under limited resources. Selecting "View>Play All Frames" will slow down the presentation of the video such that every frame can be displayed so that no frames are missed. By selecting "View>Play at Frame Rate" frames will be dropped during playback, if needed, in order to sustain the file's playback frame rate.

## Create/Export a Report {#create}

When an analysis (which may be time consuming, depending on the length of the file) is complete the analysis data may then be exported to a compressed XML file for future use. At a later point, the video may be reviewed again simply by opening the XML file--using the **Import** menu, or dragging and dropping the XML into QCTools--instead of having to reload/ reanalyze the video file itself; this process will take considerably less time. Additionally, this file may be opened in other environments such as standard spreadsheet or database applications.
Please see 'Data Format' tab for more information on the XML file compression and resulting file.
To **Export**:

*   Navigate to: Export-->To .qctools.xml.gz.
*   Click the 'XML' icon in the toolbox section of the application.

Both options will prompt you to name your file and select a select a location.
To **Import**:

*   Navigate to: Import --> From .qctools.xml.gz...
*   Drag and drop file(s) into main QCTools window

## Parts of the Tool {#parts}

### Graph Window:

This view displays the filename, graph selections, X-Axis value selector, and the frame navigator buttons.

![Data Analysis Window](media/Slide1.jpg)

Descriptions of a graphs are reveals as tooltips when hovering your cursor over the graph checkboxes:

![filter description](media/Slide4.jpg)

### Table View:

The Table View enables you to view the progress of your file download, plus associated metadata:

![List View](media/metadatascreen.jpg)

In addition to technical metadata about the file (duration, frame rate, file size, etc.) the List View also displays more detailed statistics:

| Column Name | Description |
| Yav | The average of the Y values |
| Yrang | The average of the YHIGH and YLOW values, which indicates the overall range of contrast |
| Uav | The average of the U values |
| Vav | The average of the V values |
| TOUTav | The average of the TOUT values |
| SATb | The number of frames where the maximum saturation is over 88.7, which would indicate levels outside of the broadcast range |
| SATi | The number of frames where the maximum saturation is over 118.2, which would indicate levels outside the legal YUV values |
| BRNGav | The percentage of frames with a BRNG value of more than zero |
| BRNGc | The number of frames with a BRNG value greater than zero |
| MSEfY | The number of frames with an MSEfY value over 1000 |

## Video Analysis Window {#analysis}

![Views](media/Slide3.jpg)

Many playback filters will use contextual options to tweak them, please review the Playback Filters section of the help documentation for more details.

## Comments {#comments}

Comments can be added to QCTools reports and exported alongside video metadata. Comments may be used to relay information about a specific frame in a file.

Comments can be added by selecting a frame and double-clicking or using `CTRL+m` (Mac: `command+m`) to open the comment dialogue box. There, a comment relating to the frame or charts can be written and saved. Comments can be saved by clicking the "Save" button or by using the shortcut `ALT+enter` (Mac: `option+enter`). This will cause a red diamond to appear in the comments graph. To delete a comment, open the dialogue box for the comment by double-clicking on its diamond and either remove all text or use the `Delete` button. If you attempt to close the file within QCTools, or close QCTools entirely, without exporting the report with added comments, you will be prompted to do so (or ignore the prompt and close anyway). Comment frames can be quickly jumped to using the `CTRL+,` (Mac: `command+,`) and `CTRL+.` (Mac: `command+.`) keyboard shortcuts.

Comments can be viewed in QCTools by hovering over their respective diamond or by double-clicking and opening the dialogue box. When exported, comments will appear in the QCTools XML nested under the chosen frame, and look like this: `<tag key="qctools.comment" value="This is my comment."/>`
