# Playback Filter Descriptions

The QCTools preview window is intended as an analytical playback environment that allows the user to review video through multiple filters simultaneously. Often inconsistencies within the graph layout may be better understood by examining those frames in the playback window. The playback window includes two viewing windows which may be set to different combinations of filters. This allows a user to playback a video by multiple forms of analysis simultaneously just as viewing a waveform and a vectorscope side-by-side or seeing the video with highlighted pixels that are outside of broadcast range while seeing the waveform display.

---

## Table of Contents

| A-C          | D-S       | T-Z         |
| -------------| ---------- | ---------- |
| [Audio Bit Scope](#audio-bit-scope) | [Datascope](#datascope) | [Temporal Difference](#temporal-difference) |
| [Audio Frequency](#audio-frequency) | [Extract Planes Equalized](#extract-planes-equalized) | [Temporal Outlier Pixels](#temporal-outlier-pixels) |
| [Audio Phase Meter](#audio-phase-meter) | [Extract Planes UV Equalized](#extract-planes-uv-equalized) | [Value Highlight](#value-highlight) |
| [Audio Waveform](#audio-waveform)| [Field Difference](#field-difference) | [Vectorscope](#vectorscope) |
| [Audio Spectrum](#audio-spectrum) | [Frame Tiles](#frame-tiles) | [Vertical Line Repetitions](#vertical-line-repetitions) |
| [Audio Vectorscope](#audio-vectorscope) | [Help](#help) | [Vertical Repetition Pixels](#vertical-repetition-pixels) |
| [Audio Volume](#audio-volume) | [Histogram](#histogram) | [Vectorscope Target](#vectorscope-target) |
| [Audio Waveform](#audio-waveform) | [Line Select](#line-select) | [Waveform](#waveform) |
| [Bit Plane](#bit-plane) | [No Display](#no-display) | [Waveform / Vectorscope](#waveform-vectorscope) |
| [Bit Plane 10 slices](#bit-plane-10-slices) | [Normal](#normal) | [Zoom](#zoom) |
| [Bit Plane Noise](#bit-plane-noise) | [Pixel Offset Subtraction](#pixel-offset-subtraction) |
| [Broadcast Illegal Focus](#broadcast-illegal-focus) | [Sample Range](#sample-range) |
| [Broadcast Range Pixels](#broadcast-range-pixels) | [Saturation Highlight](#saturation-highlight) |
| [Chroma Adjust](#chroma-adjust) |
| [CIE Scope](#cie-scope) |
| [Color Matrix](#color-matrix) |


---

## [Audio Bit Scope](#audio-bit-scope)

Shows an audio bit scope visualization of the audio. See FFmpeg's [abitscope](https://ffmpeg.org/ffmpeg-filters.html#abitscope) filter.
![Audio Bit Scope](media/playbackfilter_audio_bit_scope.jpg)

## [Audio Frequency](#audio-frequency)

Shows the output of FFmpeg's [showfreqs](https://ffmpeg.org/ffmpeg-filters.html#showfreqs) filter, representing the audio power spectrum.
![Audio Frequency](media/playbackfilter_audio_frequency.jpg)

## [Audio Phase Meter](#audio-phase-meter)

Shows the output of FFmpeg's [aphasemeter](https://ffmpeg.org/ffmpeg-filters.html#aphasemeter) filter, displaying the audio phase.
![Audio Phase Meter](media/playbackfilter_audio_phase_meter.jpg)

## [Audio Spectrum](#audio-spectrum)

Displays a visualization of the audio spectrum. Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image.
![Audio Spectrum](media/playbackfilter_audio_spectrum.jpg)

## [Audio Vectorscope](#audio-vectorscope)

Plots two channels of audio against each other on different axis. This display can show if audio is out-of-phase (displays as a horizontal line), dual-mono (displays as a verical line), or stereo (displays as a two dimensional complex shape).
![Audio Vectorscope](media/playbackfilter_audio-vectorscope.jpg)

## [Audio Volume](#audio-volume)

Shows the output of FFmpeg's [showvolume](https://ffmpeg.org/ffmpeg-filters.html#showvolume) filter.
![Audio Volume](media/playbackfilter_audio_volume.jpg)

## [Audio Waveform](#audio-waveform)

Displays a visualization of the audio waveform. Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image.
![Audio Waveform](media/playbackfilter_audio_waveform.jpg)

## [Bit Plane](#bit-plane)

This filter selects the bit position of each plane for display. Selecting 'None' for a plane will replace all values with 0x80 (middle gray for Y and no color for U or V). Selecting 'All' will send the display plane as is. Selecting 'Bit [1-8]' will display only that specific bit position of each pixel of the plane. For the Y plane a pixel will display as black if that bit is '0' or white if that bit is '1'. For U a pixel will be yellow-green if '0' purple if '1'. For V a pixel will be green for '0' and red for '1'.
Generally lossy video codecs will show blocky structured patterns at higher numbered bit positions. See the [bit plane article](https://en.wikipedia.org/wiki/Bit_plane) in Wikipedia for more information about the application of bit plane filtering.
![Bit Plane](media/1A_seattle_parade_transfer_a_messedup-1.jpg)

(Video sample and permission to use provided by [seattle.gov](https://www.seattle.gov))

## [Bit Plane 10 slices](#bit-plane-10-slices)

This filter is similar to **Bit Plane**, but it shows a section of each of the first 10 bit planes at once in the selected plane. The slices are presented in most-significant to least-significant order as left to right (or top to bottom if 'Rows' is selected). Each of the 10 bit planes is marked by a green border.
![Bit Plane 10 Slices](media/playbackfilter_bit_plane_10_slices.jpg)

## [Bit Plane Noise](#bit-plane-noise)

This filter is similar to **Bit Plane**, but instead of showing if the selected bit position of a selected plane is set to 0 or 1, it attempts to predict if that bit represents signal or noise. This filters uses a [method](https://en.wikipedia.org/wiki/Bit_plane) for calculating this by comparing each pixel's selected bit (X,Y) to selected bit of three adjacent pixels (X-1,Y), (X,Y-1) and (X-1,Y-1). If the bit is the same as at least two of the three adjacent bits, it may not be noise. A noisy bit-plane will have 49% to 51% pixels that are noise.
![Bit Plane Noise](media/playbackfilter_bit_plane_noise.jpg)

## [Bit Plane Noise Graph](#bit-plane-noise-graph)

This filter displays the YUV plot of a selected bit position. Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image.
![Bit Plane Noise Graph](media/playbackfilter_bit_plane_noise_graph.jpg)

## [Broadcast Illegal Focus](#broadcast-illegal-focus)

For video that uses the YUV colorspace and decode in a broadcast range. Values from 0-16 (on an 8 bit scale) will all decode to black on a computer monitor, while values from 235-255 will decode as white. This filter allows the users to select to feature pixel data that is outside of broadcast range. Select "above whites" to set pixels with luma values from 236-255 to a range of grays (while all other pixels are set to black). Select "below black" to set pixels with luma values from 0-15 to a range of grays (while all other pixels are set to black). If a video frame only contains pixels that have luma values within broadcast range, then this filter will play black only black pixels. This filter portrays broadcast range compliance in somewhat the opposite way as the "Broadcast Range Pixels" filter since it focuses on values that may be crushed to black or white because they fall outside of broadcast range. Note that some videos may intentionally be encoded in 'full range' where this filter is less relevant.
![Broadcast Illegal Focus](media/playbackfilter_broadcast_illegal_focus.jpg)

## [Broadcast Range Pixels](#broadcast-range-pixels)

This is the same presentation as 'Normal' except that pixels that are outside of broadcast range are highlighted as white. Again here, you have the option of selecting **'Field'** to display field 1 (top) and field 2 (bottom) separately.
![Broadcast Range Pixels](media/playbackfilter_broadcast_range_pixels.jpg)

## [Chroma Adjust](#chroma-adjust)

This filter enables the hue and saturation levels to be adjusted. Hue adjustments may be expressed in degrees where 0 is no change and 180 would invert the color. For saturation a value of 1 needs the saturation unchanged, 0 removes all color, and the saturation may be increased up to a maximum of 10\. The chroma values (Cb and Cr) may also be shifted by increasing or decreasing their values (similar to Red Shift/Blue Shift on a time-base corrector).
![Chroma Adjust](media/playbackfilter_chroma_adjust.jpg)

## [CIE Scope](#cie-scope)

This filter plots the range of visible colors as defined by the Committee Internationale de l'Eclairage/International Commission on Lighting (CIE) chromaticity diagram. See Georgia State University's [Hyperphysics](http://hyperphysics.phy-astr.gsu.edu/hbase/vision/cie.html) page for more information on this color space.
![CIE Scope](media/playbackfilter_CIE_scope.jpg)

## [Color Matrix](#color-matrix)

Allows for playback in various color spaces, including BT.601, BT.709, SMPTE240M and FCC. The filter includes a **Reveal** slider so that the original image and a version interpreted through the selected color matrix may be shown side-by-side for review.
![Color Matrix](media/playbackfilter_color_matrix.jpg)

## [Datascope](#datascope)
This filter displays an error as hexidecimals, corresponding to YUV levels. Y is plotted on one line and U/V on the other. Navigate the display setting an x and y coordinate.

![Datascope](media/playbackfilter_datascope.jpg)

## [Extract Planes Equalized](#extract-planes-equalized)

This filter extracts a specified video plane (such as Y, U, or V) which represents the luma or part of the chroma data from the video so that it may be reviewed on its own. The filter also may apply histogram equalization to redistributes the pixel intensities to equalize their distribution across the intensity range (this feature can help exaggerate or clarify the details of the plane image). This filter is useful for detecting lossy compression in video signals or establishing provenance.

This image shows the Normal display on the left and Extract Planes on the right. The Extract Planes filters reveals the macroblock pattern of lossy MPEG2 compression in the square patterns throughout the image.
![Extract Planes Equalized](media/playbackfilter_extract_planes.jpg)

## [Extract Planes UV Equalized](#extract-planes-uv-equalized)

This filter is similar to the **Extract Planes Equalized** filter but shows the two chroma planes (U and V) side by side.
![Extract Planes UV Equalized](media/playbackfilter_extract_planes_UV_equalized_2.jpg)

## [Field Difference](#field-difference)

This presentation visualizes the difference between video field 1 and field 2\. A middle gray image would mean that field 1 and field 2 were identical, whereas deviation to white or black indicates a difference.

![Field Difference](media/playbackfilter_field_difference.jpg)

## [Frame Tiles](#frame-tiles)

Displays a user-defined "tiled" mosaic of successive frames. Maximum 12x12 grid. Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image.
![Frame Tiles](media/playbackfilter_frame_tiles.jpg)

## [Help](#help)

This selection will launch the Playback Filters window where you may review the documentation.

## [Histogram](#histogram)

The histogram shows the frequency of occurrence of values per channel. Typically the histogram will show one graph per channel (one for each Y, U, and V or one for each red, green, and blue). Video with a lot of contrast and a well distributed range of luminance values will result in a histogram with an even spread. You may also select **'Field'** option which will depict fields 1 and 2 separately (field 1 on top, field 2 on bottom).
![Histogram](media/playbackfilter_histogram_3.jpg)

## [Line Select](#line-select)

Allows a user to select one line of video signal to display as a waveform. Includes **'Vertical'** and **'Background'** modes. When **'Vertical'** is enabled the user may select to plot a waveform of a single column rather than the default plot of a single row. The **'Background'** option shows the frame image under the waveform with the highlighted row or column highlighted in yellow.

![Line Select](media/playbackfilter_line_select.jpg)

## [No Display](#no-display)

This option enables you to remove one display, thereby only showing one view. This option allows a single playback window to occupy a greater amount of screen-space.
![No Display](media/playbackfilter_no_display.jpg)

## [Normal](#normal)

This view simply shows the video as QCTools interprets it, no special effects or filtering are added. In this view, however, you also have the option of enabling a **'Field'** display which splits the two video fields for the selected frame and displays them as discrete images. Thus all odd-numbered video lines appear are resorted to appear on the top of the image and the even-numbered lines appear on the bottom. Since many analog video issues occur differently between the two interlaced fields, splitting the fields into two distinct images can make it easier to see if a given issue is from problems with the analog video playback device (such as a head clog where the two fields would react very differently) and tape damage (where the two fields would react similarly).

This image shows two Normal displays side-by-side where the right image has **'Field'** enabled. By viewed the fields separated on the right, it is easily clear that while field 1 was read correctly from the tape, there was no color data was read for field 2\. This issue was due to a head clog and fixed by cleaning the video player and re-digitizing the content.

![Normal / Field Split](media/playbackfilter_normal_fieldsplit.jpg)

## [Pixel Offset Subtraction](#pixel-offset-subtraction)

Displays an image by subtracting the offset level from each successive pixel.
![Pixel Offset Subtraction](media/playbackfilter_pixel_offset_subtraction.jpg)

## [Sample Range](#sample-range)

This filter operates similarly to **Color Matrix** but shows the original image alongside a version interpreted through a selected sample range. Here you can see how the video would look it interpreted as either full range or broadcast range.<br>
![Sample Range](media/playbackfilter_sample_range.jpg)

## [Saturation Highlight](#saturation-highlight)

Highlights saturation with a user-defined range of minimum value to maximum value. Includes field mode.
![Saturation Highlight](media/playbackfilter_saturation_highlight_3.jpg)

## [Show CQT](#show-cqt)

Displays a visualization of the audio as a musical scale. See [FFmpeg's showcqt documentation](https://ffmpeg.org/ffmpeg-filters.html#showcqt) for information and examples.<br>
![Show CQT](media/playbackfilter_show_CQT_1.jpg)

## [Temporal Difference](#temporal-difference)

Displays an image obtained from the temporal difference between successive frames. Note that because this filter requires more than one frame to display the filter will need to be in playback mode to reveal an image.
![Temporal Difference](media/playbackfilter_temporal_difference.jpg)

## [Temporal Outlier Pixels](#temporal-outlier-pixels)

This is the same presentation as 'Normal' except that pixels that are labelled as temporal outliers are highlighted as white. Temporal outliers are pixels that significantly differ from their neighbors and often correspond to tape damage or playback error. Select **'Field'** to see the fields displayed separately.
![Temporal Outlier Pixels](media/playbackfilter_temporal_outlier_pixels_1.jpg)

## [Value Highlight](#value-highlight)

This filter selects a video plane and highlights values with a specified range of minimum value to maximum value. The original image of the plane will be presented in grayscale and values within the range will be highlighted as yellow; for instance to highlight Y values below NTSC broadcast range, set plane to Y, min to 0 and max to 16. The resulting image will highlight Y values below broadcast range in yellow.

![Value Highlight](media/playbackfilter_value_highlight.jpg)

## [Vectorscope](#vectorscope)

A vectorscope display. This display plots chroma values (U/V color placement) in two dimensional graph (which is called a vectorscope). It can be used to read the hue and saturation of the current frame. The whiter a pixel in the vectorscope, the more pixels of the input frame correspond to that pixel (that is the more pixels have this chroma value). The V component is displayed on the vertical (Y) axis, with the bottom edge being V = 0 and the top being V = 255\. The U component is displayed on the horizontal (Y) axis, with the left representing U = 0 and the right representing U = 255.
Six blocks are highlighted to depict standardized color points for red (90, 16), green (54, 222), blue (240, 146), cyan (166, 240), magenta (202, 44), and yellow (16, 110). All valid chroma values fall within a circlular shape from the center to the outer edge of the plot. You may also select **'Field'** option which will depict fields 1 and 2 separately (field 1 on top, field 2 on bottom).
![Vectorscope Split Screen](media/playback_layout_two_windows.jpg)

The vectorscope player provides the following options:

* Field: If the checkbox is enabled the player will show two waveforms side-by-side that depict field 1 and field 2 separately.
* Intensity: Set intensity. Smaller values are useful to find out how many values of the same luminance are distributed across input rows/columns. Default value is 0.1\. Allowed range is [0, 1].
* Mode: The vectorscope filter can be adjusted to different displays. The default is 'color3'. See [FFmpeg's vectorscope documentation](https://ffmpeg.org/ffmpeg-filters.html#vectorscope) for information on each option.
* Peak: If enabled, the vectorscope will outline the extent of the plotted values to show an envelope around the plotted values. Peak may be adjusted to outline the extent frame-per-frame or over time.

![Vectorscope](media/vectorscope_illegal.jpg)

## [Vectorscope Target](#vectorscope-target)

The Vectorscope Target is similar to the **Vectorscope** filter but a box is drawn over the image by setting an x and y coordinate and size of the box. The vectorscope image drawn will depict the vectorscope plotting of only the samples within the box. The original image may be shown as a background to the vectorscope by enabling the 'Background' checkbox.
![Vectorscope Target](media/playbackfilter_vectorscope_target.jpg)

## [Vertical Line Repetitions](#vertical-line-repeititons)

This filter displays repetitive lines of video data.
![Vertical Line Repetitions](media/playbackfilter_vertical_line_repetitions_2.jpg)

## [Waveform](#waveform)

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

## [Waveform Target](#waveform-target)

The Waveform Target is similar to the **Waveform** filter but a box is drawn over the image by setting an x and y coordinate and size of the box. The waveform image drawn will depict the waveform plotting of only the samples within the box. The original image may be shown as a background to the vectorscope by enabling the 'Background' checkbox.
![Waveform Target](media/playbackfilter_waveform_target.jpg)

## [Waveform / Vectorscope](#waveform-vectorscope)

This filter plots the Waveform and Vectorscope on top of each other so that both are shown in one display. The brightness of both the waveform and vectorscope may be adjusted for clarity.
![Waveform / Vectorscope](media/playbackfilter_waveform_vectorscope.jpg)

## [Zoom](#zoom)

Allows a user to zoom to a particular portion of the image by setting an x and y coordinate. Includes "Strength" and "Intensity" modes.

![Zoom](media/playbackfilter_zoom.jpg)
