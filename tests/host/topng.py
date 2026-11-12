"""Wandelt die Framebuffer-Dumps (.rgb565) von render_test in 2x vergroesserte PNGs um."""
import struct, zlib, glob, os, sys
def png(path, w, h, rgb):
    raw = b''.join(b'\x00' + rgb[y*w*3:(y+1)*w*3] for y in range(h))
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    open(path,'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
for f in sorted(glob.glob(os.environ.get('OUT', '.') + '/*.rgb565')):
    d = open(f,'rb').read(); w,h=480,272
    out = bytearray()
    for i in range(w*h):
        v = d[2*i] | (d[2*i+1] << 8)
        r=(v>>11)&31; g=(v>>5)&63; b=v&31
        out += bytes(((r<<3)|(r>>2), (g<<2)|(g>>4), (b<<3)|(b>>2)))
    big = bytearray()
    for y in range(h):
        row = bytearray()
        for x in range(w): row += out[(y*w+x)*3:(y*w+x)*3+3]*2
        big += row*2
    png(f.replace('.rgb565','.png'), w*2, h*2, bytes(big))
print(sorted(os.path.basename(p) for p in glob.glob(os.environ.get('OUT', '.') + '/*.png')))
