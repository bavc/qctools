# Playback Filter Descriptions

The QCTools preview window is intended as an analytical playback environment that allows the user to review video through multiple filters simultaneously. Often inconsistencies within the graph layout may be better understood by examining those frames in the playback window. The playback window includes two viewing windows which may be set to different combinations of filters. This allows a user to playback a video by multiple forms of analysis simultaneously just as viewing a waveform and a vectorscope side-by-side or seeing the video with highlighted pixels that are outside of broadcast range while seeing the waveform display.

---

# Table of Contents

[Arrangement](#arrangement)  

[Adjustments](#adjustments)  
[Adjust Signal](#adjust-signal)  

[Bit Plane](#bit-plane)  
[Bit Plane 10 slices](#bit-plane-10-slices)  
[Bit Plane Noise](#bit-plane-noise)  
[Broadcast Illegal Focus](#broadcast-illegal-focus)  
[Broadcast Range Pixels](#broadcast-range-pixels)  
[Chroma Shift](#chroma-shift)  
[CIE Scope](#cie-scope)  
[Color Matrix](#color-matrix)  
[Corners](#corners)  
[Datascope](#datascope)  
[EIA608 VITC Viewer](#eia608)  
[Extract Planes Equalized](#extract-planes-equalized)  
[Field Difference](#field-difference)  
[Filmstrip](#filmstrip)  
[Histogram](#histogram)  
[Histogram Temporal](#histogram-temporal)  X
[Limiter](#limiter)  
[Line Over Time](#line-over-time)  
[Line Select](#line-select)  
[Normal](#normal)  
[Oscilloscope](#oscilloscope)  
[Pixel Offset Subtraction](#pixel-offset-subtraction)  
[Pixel Scope](#pixel-scope)  
[Sample Range](#sample-range)  
[Saturation Highlight](#saturation-highlight)  
[Temporal Difference](#temporal-difference)  
[Temporal Outlier Pixels](#temporal-outlier-pixels)  
[Value Highlight](#value-highlight)  
[Value Highlight (Range)](#value-highlight-range)  
[Vectorscope](#vectorscope)  
[Vectorscope High/Low](#vectorscope-highlow)  
[Vectorscope Target](#vectorscope-target)  
[Vertical Line Repetitions](#vertical-line-repetitions)  
[Vertical Repetition Pixels](#vertical-repetition-pixels)  
[Waveform](#waveform)  
[Waveform Target](#waveform-target)  
[Zoom](#zoom)  

# Arrangement

The size of the player may be set to "Fit to screen", "100%", or "Free Scale".

- Fit to screen: The size of the player will scale dynamically to fit the size of the window.
- 100%: The player will stay at the size of the decoded video, for instance a 720x480 video shall be presented in 720x480 of monitor pixels.
- Free Scale: A slider will set the player to a set level of scale from 50% to 200%.

The QCTools player can show from one to six different visualizations, so their arrangement may be selected from the following options:

- Vertical: All selected filters will be presented in a vertically arranged stack.
- Horizontal: All selected filters will be presented in a horizontally arranged stack.
- Grid: The filters will be presented in a grid up to a 3x2 arrangement. Filters may output images that use differing sizes, so a grid presentation may lead to overlapping filters or unused space. The 'Fit to Grid' option re-scales all filters to the same size as the input video (this is nicer to look at but can distort the presentation).

# Adjustments

## Adjust Signal

If "Adjust Signal" is enabled, then the signal of the input video will be adjusted before the filters are applied or the presentation is made. For instance, if viewing the waveform filter and adjusting the 'Black' or 'Contrast' settings under 'Adjust Signal' then you'll see the waveform be subsequently adjusted. Adjustments can be helpful is you suspect a video was digitization with unoptimal settings and the brightness and color could here be adjusted, similar to the settings of a proc amp, to show the impact of the change. It is recommend to use the waveform and vectorscope filters as the 'Adjust Signal' settings are calibrated in order to see the impact of the adjustments.

This filter allows the Y values to be increased or decreased via the **Black** slider. Also the **Contrast** allows a multiplier to be applied to all Y values.

This filter enables the hue and saturation levels to be adjusted. Hue adjustments may be expressed in degrees where 0 is no change and 180 would invert the color. For saturation a value of 1 needs the saturation unchanged, 0 removes all color, and the saturation may be increased up to a maximum of 10\. The chroma values (Cb and Cr) may also be shifted by increasing or decreasing their values (similar to Red Shift/Blue Shift on a time-base corrector).

## Filters

## Normal {#normal}

This view simply shows the video as QCTools interprets it, no special effects or filtering are added. In this view, however, you also have the option of enabling a **'Field'** display which splits the two video fields for the selected frame and displays them as discrete images. Thus all odd-numbered video lines appear are resorted to appear on the top of the image and the even-numbered lines appear on the bottom. Since many analog video issues occur differently between the two interlaced fields, splitting the fields into two distinct images can make it easier to see if a given issue is from problems with the analog video playback device (such as a head clog where the two fields would react very differently) and tape damage (where the two fields would react similarly).

The **Metadata** option will overlay some metadata onto the image, including the presentation timestamp, frame size, and metadata from filters that perform crop detection and interlacement pattern detection.

The **Action/Title Safe** option will overlay boxes to show the Action Safe and Title Safe areas as defined by SMPTE ST 2046-1 (2009). The Safe Action Area is a box that includes 93%  of the width and height and is drawn in yellow. The Safe Title Area is a box that includes 90% of the width and height and is drawn in green. Thanks much to @bbgdzxng1 for supporting this feature.

## Bit Plane {#bit-plane}

This filter selects the bit position of each plane for display. Selecting 'None' for a plane will replace all values with 0x80 (middle gray for Y and no color for U or V). Selecting 'All' will send the display plane as is. Selecting 'Bit [1-8]' will display only that specific bit position of each pixel of the plane. For the Y plane a pixel will display as black if that bit is '0' or white if that bit is '1'. For U a pixel will be yellow-green if '0' purple if '1'. For V a pixel will be green for '0' and red for '1'.
Generally lossy video codecs will show blocky structured patterns at higher numbered bit positions. See the [bit plane article](https://en.wikipedia.org/wiki/Bit_plane) in Wikipedia for more information about the application of bit plane filtering.

![Bit Plane](media/1A_seattle_parade_transfer_a_messedup-1.jpg)

(Video sample and permission to use provided by [seattle.gov](https://www.seattle.gov))

## Bit Plane 10 slices {#bit-plane-10-slices}

This filter is similar to **Bit Plane**, but it shows a section of each of the first 10 bit planes at once in the selected plane. The slices are presented in most-significant to least-significant order as left to right (or top to bottom if 'Rows' is selected). Each of the 10 bit planes is marked by a green border. If the **Slice** option is enabled, the video will be sliced into 10 parts with a different bit position (from 1 to 10) presented in each slice, if **slice** is unchecked then the video is repeated as a full frame ten times with a different bit position presented in each iteration. The **Show 2** will present each bit plane with the subsequent bit plane.

![Bit Plane 10 Slices](media/playbackfilter_bit_plane_10_slices.jpg)

## Bit Plane Noise {#bit-plane-noise}

This filter is similar to **Bit Plane**, but instead of showing if the selected bit position of a selected plane is set to 0 or 1, it attempts to predict if that bit represents signal or noise. This filters uses a [method](https://en.wikipedia.org/wiki/Bit_plane) for calculating this by comparing each pixel's selected bit (X,Y) to selected bit of three adjacent pixels (X-1,Y), (X,Y-1) and (X-1,Y-1). If the bit is the same as at least two of the three adjacent bits, it may not be noise. A noisy bit-plane will have 49% to 51% pixels that are noise.

![Bit Plane Noise](media/playbackfilter_bit_plane_noise.jpg)

## Bit Plane Noise Graph {#bit-plane-noise-graph}

This filter displays the YUV plot of a selected plane (y, u, or v). Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image. The 10 least significant bits will be plotted on a scale of 0 to 1, where 1 (the top of the graph) indicates a random level of noise and 0 depicts less random data. When the video uses less than 10 bits, the higher numbered bits will be 0.

## Broadcast Illegal Focus {#broadcast-illegal-focus}

For video that uses the YUV colorspace and decode in a broadcast range. On an 8-bit scale, values from 0-16 will all decode to black on a computer monitor, while values from 235-255 will decode as white. This filter allows the users to select to feature pixel data that is outside of broadcast range. Select "above whites" to set pixels with luma values from 236-255 to a range of grays (while all other pixels are set to black). Select "below black" to set pixels with luma values from 0-15 to a range of grays (while all other pixels are set to black). If a video frame only contains pixels that have luma values within broadcast range, then this filter will play black only black pixels. This filter portrays broadcast range compliance in somewhat the opposite way as the "Broadcast Range Pixels" filter since it focuses on values that may be crushed to black or white because they fall outside of broadcast range. Note that some videos may intentionally be encoded in 'full range' where this filter is less relevant.

![Broadcast Illegal Focus](media/playbackfilter_broadcast_illegal_focus.jpg)

## Broadcast Range Pixels {#broadcast-range-pixels}

This is the same presentation as 'Normal' except that pixels that are outside of broadcast range are highlighted. Again here, you have the option of selecting **'Field'** to display field 1 (top) and field 2 (bottom) separately.

![Broadcast Range Pixels](media/playbackfilter_broadcast_range_pixels.jpg)

## CIE Scope {#cie-scope}

This filter plots the range of visible colors as defined by the Committee Internationale de l'Eclairage/International Commission on Lighting (CIE) chromaticity diagram. See Georgia State University's [Hyperphysics](http://hyperphysics.phy-astr.gsu.edu/hbase/vision/cie.html) page for more information on this color space.

![CIE Scope](media/playbackfilter_CIE_scope.jpg)

## Chroma Shift {#chroma-shift}

This filter allows the Cb and Cr planes to be offset horizontally or veritcally to check if the color data is properly timed to the luminence data. See FFmpeg's documentation on [chroma shift](https://ffmpeg.org/ffmpeg-filters.html#chromashift).

## Color Matrix {#color-matrix}

Allows for playback in various color spaces, including BT.601, BT.709, SMPTE240M and FCC. The filter includes a **Reveal** slider so that the original image and a version interpreted through the selected color matrix may be shown side-by-side for review.

![Color Matrix](media/playbackfilter_color_matrix.jpg)

## Corners {#corners}

This view presents a quadrant view of a zoom in on the four corners of the image. The W and H options can be adjusted to change the width and height of the corner to crop and display. The resulting pixels are then enlarged and shown within a grid. This view can be used to determine a precise cropping for an image or to inspect the details of the frame's edges.

When the 'field' mode is selected the four quadrants are split to show the top field on the top of each part of the quadrant and the bottom field on the bottom of each part of the quadrant.

## Datascope {#datascope}

![Datascope](media/playbackfilter_datascope.jpg)

## EIA608 VITC Viewer {#eia608}

This filter interprets potential VITC and EIA608 (often referred to as line 21) data to display the values stored as timecode and/or captions. By default the top 30 lines of video are assessed for this data but the scan_max option can adjust the number of top lines to assess from 1 to 100. EIA608 data will be shown in hexadecimal form along with the number of the line (counting from 1 as the top line). There is a 'zoom' option which will crop and scale the selected lines so that they can be viewed in more detail. If VITC or EIA608 data is not found then hyphens will be shown instead.

Several options from FFmpeg's readeia608 filter are accessible:

- scan_max: set to which line to scan for codes (default 30)
- msd: set first two max start code bits differences (from 0 to 0.5) (default 0.02)
- chp: check and apply parity bit (default false)

![EIA608 VITC Viewer](media/vitceia608.jpg)

For more information, see the documentation on FFmpeg's [readvitc](https://ffmpeg.org/ffmpeg-filters.html#readvitc) and [readeia608](https://ffmpeg.org/ffmpeg-filters.html#readeia608) filters. A sample IMX file with both VITC and EIA608 data is available at the [Internet Archive](https://archive.org/download/vitc_eia608_sample/vitc_eia608_sample.mov).

## Extract Planes Equalized {#extract-planes-equalized}

This filter extracts a specified video plane (such as Y, U, or V) which represents the luma or part of the chroma data from the video so that it may be reviewed on its own. The filter also may apply histogram equalization to redistributes the pixel intensities to equalize their distribution across the intensity range (this feature can help exaggerate or clarify the details of the plane image). This filter is useful for detecting lossy compression in video signals or establishing provenance.

This image shows the Normal display on the left and Extract Planes on the right. The Extract Planes filters reveals the macroblock pattern of lossy MPEG2 compression in the square patterns throughout the image.

![Extract Planes Equalized](media/extractplanes.jpg)

## Field Difference {#field-difference}

This presentation visualizes the difference between video field 1 and field 2\. A middle gray image would mean that field 1 and field 2 were identical, whereas deviation to white or black indicates a difference.

![Field Difference](media/fielddiff.jpg)

## Filmstrip {#filmstrip}

Displays a user-defined "tiled" mosaic of successive frames. Maximum 12x12 grid. Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image. By default, this player will scale down the frame size before tiling them together, but the "Full Size" option will avoid scaling but impact performance.

![Frame Tiles](media/playbackfilter_frame_tiles.jpg)

## Histogram {#histogram}

The histogram shows the frequency of occurrence of values per channel. Typically the histogram will show one graph per channel (one for each Y, U, and V or one for each red, green, and blue). Video with a lot of contrast and a well distributed range of luminance values will result in a histogram with an even spread. You may also select **'Field'** option which will depict fields 1 and 2 separately (field 1 on top, field 2 on bottom).

![Histogram](media/playbackfilter_histogram_3.jpg)

## Histogram (Temporal) {#histogram-temporal}

The histogram shows the frequency of occurrence of values per channel over time. See [the ffmpeg documentation on thistogram](https://ffmpeg.org/ffmpeg-filters.html#thistogram) for more information.

## Limiter {#limiter}

Constrain playback to a specified sample range of the frame. Setting min and max values will clip all values below min to min and all values above max to max. For instance with an 8 bit video, the samples above broadcast range could be examined by setting min to 235 (the upper limit of broadcast range) and 255 (the highest possible 8-bit value). The strength value will apply a global color histogram equalization to stress the difference between the sample values. See the FFmpeg [limiter](http://ffmpeg.org/ffmpeg-filters.html#limiter) filter for more information.

## Line over Time {#line-over-time}

This filter shows only 1 or 2 lines of video accumulate over time, like a slit-scan video. It can be used to focus on a single line of video (such as captioning in line 21) but showing multiple frames of an isolated line at once. The image is generated with FFmpeg's [tile filter](http://ffmpeg.org/ffmpeg-filters.html#tile).

## Line Select {#line-select}

Allows a user to select one line of video signal to display as a waveform. Includes **'Vertical'** and **'Background'** modes. When **'Vertical'** is enable the user may select to plot a waveform of a single column rather than the default plot of a single row. The **'Background'** option shows the frame image under the waveform with the highlighted row or column highlighted in yellow.

![Line Select](media/lineselect.jpg)

## Oscilloscope {#oscilloscope}

Provides a 2D Video Oscilloscope. Useful to measure spatial impulse, step responses, chroma delays, etc. See FFmpeg's [oscilloscope documentation](http://ffmpeg.org/ffmpeg-filters.html#oscilloscope) for more information.

## Pixel Offset Subtraction {#pixel-offset-subtraction}

Displays the difference betweeen a frame and the same frame offset vertically or horizontally by a specified amount. Vertical and horizontal offsets may be specified independently for luma and chroma. When all offsets are set the zero, the image should be gray (i.e. no difference).

![Pixel Offset Subtraction](media/playbackfilter_pixel_offset_subtraction.jpg)

## Pixel Scope {#pixel-scope}

Displays an image using FFmpeg's (pixscope)[https://ffmpeg.org/ffmpeg-filters.html#pixscope] filter. Note that if the image is less than 640x480, it will be scaled up to that size before displaying it in the scope.

## Sample Range {#sample-range}

This filter operates similarly to **Color Matrix** but shows the original image alongside a version interpreted through a selected sample range. Here you can see how the video would look it interpreted as either full range or broadcast range.

![Sample Range](media/playbackfilter_sample_range.jpg)

## Saturation Highlight {#saturation-highlight}

Highlights saturation with a user-defined range of minimum value to maximum value. Includes field mode.

![Saturation Highlight](media/playbackfilter_saturation_highlight_3.jpg)

## Temporal Difference {#temporal-difference}

Displays an image obtained from the temporal difference between successive frames. Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image.

![Temporal Difference](media/playbackfilter_temporal_difference.jpg)

## Temporal Outlier Pixels {#temporal-outlier-pixels}

This is the same presentation as 'Normal' except that pixels that are labelled as temporal outliers are highlighted. Temporal outliers are pixels that significantly differ from their neighbors and often correspond to tape damage or playback error. Select **'Field'** to see the fields displayed separately.

![Temporal Outlier Pixels](media/playbackfilter_temporal_outlier_pixels_1.jpg)

## Value Highlight {#value-highlight}

This filter selects a video plane and highlights a specific value. The original image of the plane will be presented in grayscale and value will be highlighted as the selected color.

## Value Highlight (Range) {#value-highlight-range}

This filter selects a video plane and highlights values with a specified range of minimum value to maximum value. The original image of the plane will be presented in grayscale and values within the range will be highlighted as the selected color; for instance to highlight Y values below NTSC broadcast range, set plane to Y, min to 0 and max to 16 (16 for 8 bit video or 64 for 10 bit video) and the resulting image will highlight Y values below broadcast range.

![Value Highlight](media/playbackfilter_value_highlight.jpg)

## Vectorscope {#vectorscope}

A vectorscope display. This display plots chroma values (U/V color placement) in two dimensional graph (which is called a vectorscope). It can be used to read of the hue and saturation of the current frame. The whiter a pixel in the vectorscope, the more pixels of the input frame correspond to that pixel (that is the more pixels have this chroma value). On an 8-bit scale, the V component is displayed on the vertical (Y) axis, with the bottom edge being V = 0 and the top being V = 255\. The U component is displayed on the horizontal (Y) axis, with the left representing U = 0 and the right representing U = 255.
Six blocks are highlighted to depict standardized color points for red (90, 16), green (54, 222), blue (240, 146), cyan (166, 240), magenta (202, 44), and yellow (16, 110). All valid chroma values fall within a circular shape from the center to the outer edge of the plot.You may also select **'Field'** option which will depict fields 1 and 2 separately (field 1 on top, field 2 on bottom).

![Vectorscope Split Screen](media/playback_layout_two_windows.jpg)

The vectorscope player provides the following options:

* Field: If the checkbox is enabled the player will show two waveforms side-by-side that depict field 1 and field 2 separately.
* Intensity: Set intensity. Smaller values are useful to find out how many values of the same luminance are distributed across input rows/columns. Default value is 0.1\. Allowed range is [0, 1].
* Mode: The vectorscope filter can be adjusted to different displays. The default is 'color3'. See [FFmpeg's vectorscope documentation](https://ffmpeg.org/ffmpeg-filters.html#vectorscope) for information on each option.

![Vectorscope](media/vectorscope_illegal.jpg)

## Vectorscope High/Low #{vectorscope-highlow}

This player is similar to Vectorscope but it shows two vectorscopes side by side showing the lower range of colors on the left and higher range of colors on the right. The threshold slider can adjust the divider between low and high in allocating plotted colors between the two vectorscopes. This technique can be helpful to isolate colors which occur in a shadow or a highlight. See [FFmpeg's vectorscope documentation](https://ffmpeg.org/ffmpeg-filters.html#vectorscope) for information, particularly the lthreshold and hthreshold options which are used here to generate the two vectorscope images.

## Vectorscope Target {#vectorscope-target}

The Vectorscope Target is similar to the **Vectorscope** filter but a box is drawn over the image by setting an x and y coordinate and size of the box. The vectorscope image drawn will depict the vectorscope plotting of only the samples within the box. The original image may be shown as a background to the vectorscope by enabling the 'Background' checkbox.

![Vectorscope Target](media/playbackfilter_vectorscope_target.jpg)

## Vertical Line Repetitions {#vertical-line-repeititons}

This filter highlights repetitive lines of video data.

![Vertical Line Repetitions](media/playbackfilter_vertical_line_repetitions_2.jpg)

## Waveform {#waveform}

The waveform plots the brightness of the image, each column of the waveform corresponds to a column of pixels in the source video. The pixels of each column are then plotted in an 8-bit scale (0-255) which is equivalent to 0 to 110 IRE. The range from 0-16 (0 to 7.5 IRE) is highlighted in blue and indicates a black value that is below traditional NTSC broadcast range. The range from 235-255 (100 to 110 IRE) is highlighted in red and indicates a white value that is above broadcast range. For most analog media the intended pixel luminosity values should exist between 7.5 and 100 IRE. You may also select **'Field'** option which will depict fields 1 and 2 separately (field 1 on top, field 2 on bottom).

Traditionally a waveform plots the values of each column of video pixels. The QCTools waveform also provides a **'Vertical'** option which plots the video as rows of pixels. The waveform will still show the value range of the whole frame whether **'Vertical'** is enabled or not, but with **'Vertical'** enabled it is often easier to detect video issues that affect rows of pixel data such as dropouts. The waveform player is based on the [waveform filter](https://ffmpeg.org/ffmpeg-filters.html#waveform) from FFmpeg.

![Waveform](media/waveform.jpg)

The waveform player provides the following options:

* Field: If the checkbox is enabled the player will show two waveforms side-by-side that depicts field 1 and field 2 separately.
* Intensity: Set intensity. Smaller values are useful to find out how many values of the same luminance are distributed across input rows/columns. Default value is 0.1\. Allowed range is [0, 1].
* Y/U/V/A: Select which plane is presented in waveform view ("A" means that all planes are shown).
* Vertical: If checked then the waveform will plot on rows rather than columns. It is the equivalent of rotating the video image by 90 degrees and applying a waveform to the result.
* Filter: The waveform filter can be adjusted to different displays. The default is 'lowpass'. See [FFmpeg's waveform documentation](https://ffmpeg.org/ffmpeg-filters.html#waveform) for information on each option.
* Peak: If enabled, the waveform will outline the extent of the plotted values to show an envelope around the plotted values. Peak may be adjusted to outline the extent frame-per-frame or over time.

## Waveform / Vectorscope {#waveform-vectorscope}

This filter plots the Waveform and Vectorscope on top of each other so that both are shown in one display. The brightness of both the waveform and vectorscope may be adjusted for clarity.

![Waveform / Vectorscope](media/playbackfilter_waveform_vectorscope.jpg)

## Waveform Target {#waveform-target}

The Waveform Target is similar to the **Waveform** filter but a box is drawn over the image by setting an x and y coordinate and size of the box. The waveform image drawn will depict the waveform plotting of only the samples within the box. The original image may be shown as a background to the vectorscope by enabling the 'Background' checkbox.

![Waveform Target](media/playbackfilter_waveform_target.jpg)

## Zoom {#zoom}

Allows a user to zoom to a particular portion of the image using X and Y coordinates. Includes "Strength" and "Intensity" modes.

![Zoom](media/playbackfilter_zoom.jpg)
