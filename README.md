# MIMA - Monochrome Image, Maybe Anything

This is the repository for the specification (mimaspec) and reference encoder/decoder (mima.h) for the MIMA file format.

MIMA is a file format intended to be an equivalent of QOI for grayscale images, that is a format that is one-pass encoding and
decoding.

It can also encode color and/or alpha images, and files, although it is bad at it.

## Performance

The reference encoder/decoder are not really optimised in terms of speed, but are at least an order of magnetude faster than 
the stb implementation of PNG.

In terms of file size, **in general**, MIMA always produces files smaller than QOI, and can be from 2 times smaller to 
2 times bigger than PNG, depending on compression. It can produce smaller files than low-loss JPEGs on occasions, but lossy
ones are always smaller.

MIMA is not intended to compress images with alpha and/or colors, and has poor performance with them. Still better than
BMP though.

## Copyright

The specification, convert program (mimatool), and reference encoder/decoder are dual-licensed MIT/Public Domain.
