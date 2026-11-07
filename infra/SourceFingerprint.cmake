# Fingerprint every source and config file at configure time, then pass the
# combined digest to io_config so a written report records which sources
# produced it. CONFIGURE_DEPENDS re-runs the glob when files are added or
# removed, so the fingerprint stays accurate without a manual touch.
file(GLOB_RECURSE fingerprintFiles CONFIGURE_DEPENDS src/*.h src/*.cpp src/*.cu src/*.vert src/*.frag src/*.glsl configs/*.yaml)

set(fingerprint "")
foreach(source IN LISTS fingerprintFiles)
    file(SHA256 "${source}" digest)
    string(APPEND fingerprint "${digest}")
endforeach()

string(SHA256 sourceFingerprint "${fingerprint}")
