# Image Loading Foundation

v0.2.4 provides a deliberately small, original decoder for the ASCII PPM P3
subset needed by bootstrap textures. It consumes explicit bytes and size and
produces caller-owned `HTHImageData`; it has no Resource, Material, Renderer,
filesystem, or OpenGL dependency.

## Accepted PPM Subset

The token sequence is:

```text
P3
width height
255
red green blue ...
```

Width and height are positive `uint32_t` values, maxval is exactly 255, and
there are exactly `width * height * 3` decimal integer channels in range
0–255. Spaces, tabs, LF, CRLF, bare CR, and `#` comments are accepted between
tokens and after the final channel. Any extra token is rejected. P6, other
maxval values, signed/non-integer channels, BOM, embedded NUL, truncation, and
trailing data are errors.

Dimension and byte-count multiplication are checked before allocation. Output
is tightly packed RGB8 with no alpha, gamma/colorspace metadata, or implicit
row padding. The decoder preserves file-native row order: the first PPM row is
the first row in memory. It performs no vertical flip; the renderer's fixed UV
convention determines presentation.

The caller releases pixels with `hth_image_data_release`, which resets the
entire object and is safe to call repeatedly. Decoded pixels remain valid after
the source Resource Data is released. Graphical initialization uploads them
synchronously and OpenGL retains its own GPU object; headless initialization
validates and then retains/releases the same CPU data through normal engine
lifecycle.

PPM P3 is a bootstrap validation format, not the final art pipeline. PNG or
another production format can be introduced later without moving file I/O into
the decoder or GPU types into image data.
