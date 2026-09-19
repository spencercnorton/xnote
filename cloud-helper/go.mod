module github.com/spencercnorton/xnote/cloud-helper

// Go 1.26.8 includes the archive/tar fix for GO-2026-4869. This helper reads
// encrypted backup archives, so builds with older vulnerable toolchains must
// fail rather than silently ship an unsafe restore path.
go 1.26.8

require (
	golang.org/x/crypto v0.54.0
	golang.org/x/term v0.45.0
)

require golang.org/x/sys v0.47.0 // indirect
