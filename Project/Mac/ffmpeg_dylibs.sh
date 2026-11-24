#!/bin/sh

script_directory="${SCRIPT_DIR:-$(cd "$(dirname "$0")" && pwd)}"
if [ -n "${FFMPEG}" ] ; then
    ffmpeg_root="${FFMPEG}"
elif [ -d "${script_directory}/../../../ffmpeg" ] ; then
    ffmpeg_root="$(cd "${script_directory}/../../.." && pwd)/ffmpeg"
else
    ffmpeg_root="$(cd "${script_directory}/../.." && pwd)/ffmpeg"
fi
ffmpeg_library_dir="${ffmpeg_root}/lib"

canonical_library_name() {
    printf '%s\n' "$1" | sed -E 's/(\.[0-9]+)+\.dylib$/.dylib/'
}

bundle_ffmpeg_dylibs() {
    destination="$1"
    dependency_prefix="$2"
    executable="$3"

    mkdir -p "${destination}"

    found_library=false
    for library in "${ffmpeg_library_dir}"/*.dylib ; do
        if [ ! -e "${library}" ] ; then
            continue
        fi
        found_library=true
        library_name="$(basename "${library}")"
        canonical_name="$(canonical_library_name "${library_name}")"
        if [ "${canonical_name}" != "${library_name}" ] && [ -e "${ffmpeg_library_dir}/${canonical_name}" ] ; then
            continue
        fi
        cp -Lf "${library}" "${destination}/${canonical_name}"
        install_name_tool -id "${dependency_prefix}/${canonical_name}" "${destination}/${canonical_name}"
    done

    if [ "${found_library}" = false ] ; then
        echo "No FFmpeg dynamic libraries found in ${ffmpeg_library_dir}" >&2
        return 1
    fi

    for target in "${executable}" "${destination}"/*.dylib ; do
        if [ ! -e "${target}" ] ; then
            continue
        fi
        otool -L "${target}" | awk 'NR > 1 {print $1}' | while IFS= read -r dependency ; do
            dependency_name="$(basename "${dependency}")"
            canonical_name="$(canonical_library_name "${dependency_name}")"
            if [ -e "${destination}/${canonical_name}" ] ; then
                install_name_tool -change "${dependency}" "${dependency_prefix}/${canonical_name}" "${target}"
            fi
        done
    done
}
