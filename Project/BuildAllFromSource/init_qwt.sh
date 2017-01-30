#! /bin/bash

echo "PWD: " + $PWD

if [ ! -d qwt ] ; then
    wget https://github.com/osakared/qwt/archive/tags/qwt-6.1.2.zip
    unzip qwt-6.1.2.zip
    mv qwt-tags-qwt-6.1.2 qwt
    rm qwt-6.1.2.zip
    patch qwt/qwtconfig.pri <<EOF
--- qwtconfig.pri	2016-01-03 15:41:26.000000000 -0500
+++ qwtconfig.pri copy	2016-01-03 15:41:26.000000000 -0500
@@ -72,7 +72,7 @@
 # it will be a static library.
 ######################################################################

-QWT_CONFIG           += QwtDll
+#QWT_CONFIG           += QwtDll

 ######################################################################
 # QwtPlot enables all classes, that are needed to use the QwtPlot 
@@ -93,13 +93,13 @@
 # export a plot to a SVG document
 ######################################################################

-QWT_CONFIG     += QwtSvg
+#QWT_CONFIG     += QwtSvg

 ######################################################################
 # If you want to use a OpenGL plot canvas
 ######################################################################

-QWT_CONFIG     += QwtOpenGL
+#QWT_CONFIG     += QwtOpenGL

 ######################################################################
 # You can use the MathML renderer of the Qt solutions package to 
@@ -118,7 +118,7 @@
 # Otherwise you have to build it from the designer directory.
 ######################################################################

-QWT_CONFIG     += QwtDesigner
+#QWT_CONFIG     += QwtDesigner

 ######################################################################
 # Compile all Qwt classes into the designer plugin instead
EOF
fi
cd qwt
$BINQMAKE
make
cd "$INSTALL_DIR"
