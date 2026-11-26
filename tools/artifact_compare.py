"""Compare generated artwork independently of the zlib implementation.

PNG files are deflate-compressed, and different zlib builds (stock zlib,
zlib-ng, Apple's libz) emit different but equally valid bytes for the same
pixels.  Generator --check modes therefore compare PNGs by their decoded
chunks; every other artifact is still compared byte for byte.
"""
import zlib

PNG_SIGNATURE = b'\x89PNG\r\n\x1a\n'


def _png_chunks(data):
    """Return [(type, payload)] with all IDAT data joined and inflated."""
    chunks, idat, at = [], b'', len(PNG_SIGNATURE)
    while at + 8 <= len(data):
        length = int.from_bytes(data[at:at + 4], 'big')
        kind = data[at + 4:at + 8]
        payload = data[at + 8:at + 8 + length]
        if len(payload) != length:
            raise ValueError('truncated PNG chunk')
        if kind == b'IDAT':
            idat += payload
        else:
            if kind == b'IEND' and idat:
                chunks.append((b'IDAT', zlib.decompress(idat)))
                idat = b''
            chunks.append((kind, payload))
        at += 12 + length
    if idat:
        chunks.append((b'IDAT', zlib.decompress(idat)))
    return chunks


def same_artifact(existing, generated):
    """True when EXISTING already holds GENERATED's content."""
    if existing == generated:
        return True
    if existing.startswith(PNG_SIGNATURE) and generated.startswith(PNG_SIGNATURE):
        try:
            return _png_chunks(existing) == _png_chunks(generated)
        except (ValueError, zlib.error):
            return False
    return False
