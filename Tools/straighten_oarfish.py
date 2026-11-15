"""Straightens the oarfish model (pure Python, no packages).

    python3 Tools/straighten_oarfish.py <curled creature_oarfish.glb from Rodin> ArtDrop/creature_oarfish.glb
    DC_ART_ONLY=creature:oarfish Tools/import_art.sh

Rodin made the body curled into an S (the concept picture was drawn that way). A live oarfish holds its body straight.
"""
import sys, math
import json, struct, zlib
CT = {5120:'b',5121:'B',5122:'h',5123:'H',5125:'I',5126:'f'}
NC = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
class Glb:
    def __init__(s, path):
        d = open(path,'rb').read()
        assert d[:4]==b'glTF'
        jl = struct.unpack_from('<I', d, 12)[0]
        s.j = json.loads(d[20:20+jl])
        o = 20+jl
        bl = struct.unpack_from('<I', d, o)[0]
        s.bin = bytearray(d[o+8:o+8+bl])
    def acc(s, i):
        a = s.j['accessors'][i]; bv = s.j['bufferViews'][a['bufferView']]
        n = NC[a['type']]; f = CT[a['componentType']]; sz = struct.calcsize(f)*n
        stride = bv.get('byteStride', sz); off = bv.get('byteOffset',0)+a.get('byteOffset',0)
        return a, n, f, stride, off
    def read(s, i):
        a,n,f,stride,off = s.acc(i)
        out = []
        for k in range(a['count']):
            out.append(struct.unpack_from('<'+f*n, s.bin, off+k*stride))
        return out
    def write(s, i, vals):
        a,n,f,stride,off = s.acc(i)
        for k,v in enumerate(vals):
            struct.pack_into('<'+f*n, s.bin, off+k*stride, *v)
        if 'min' in a:
            a['min'] = [min(v[c] for v in vals) for c in range(n)]
            a['max'] = [max(v[c] for v in vals) for c in range(n)]
    def save(s, path):
        js = json.dumps(s.j, separators=(',',':')).encode()
        js += b' ' * ((4 - len(js)%4)%4)
        b = bytes(s.bin) + b'\0' * ((4-len(s.bin)%4)%4)
        tot = 12+8+len(js)+8+len(b)
        open(path,'wb').write(b'glTF'+struct.pack('<II',2,tot)+struct.pack('<I',len(js))+b'JSON'+js+struct.pack('<I',len(b))+b'BIN\0'+b)
def png(path, w, h, px):  # px: bytearray rgb
    raw = b''.join(b'\0'+bytes(px[y*w*3:(y+1)*w*3]) for y in range(h))
    def ch(t,d): return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d))
    open(path,'wb').write(b'\x89PNG\r\n\x1a\n'+ch(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+ch(b'IDAT',zlib.compress(raw))+ch(b'IEND',b''))


# Straightens the oarfish model: its body was generated curled into an S in the y-z plane. The body's middle line is
# found (a rough hand-drawn line, refined to the mean of the body round it), then every vertex is moved to
# (distance along the line, distance off it). Normals are turned with it. Textures and triangles stay as they are.
src, dst = sys.argv[1], sys.argv[2]
g = Glb(src); pr = g.j['meshes'][0]['primitives'][0]
P = g.read(pr['attributes']['POSITION']); Nn = g.read(pr['attributes']['NORMAL'])
zmin = min(v[2] for v in P); ymin = min(v[1] for v in P); S = 900/1.8892; H = int((max(v[1] for v in P)-ymin)*S)+40
rough_px = [(22,438),(100,480),(180,535),(260,560),(340,550),(385,500),(385,430),(340,370),(270,300),(225,230),(240,150),(300,90),(400,60),(520,65),(640,95),(740,140),(830,195),(915,235)]
line = [((x-20)/S+zmin, (H-1-y-20)/S+ymin) for x,y in rough_px]

def resample(L, n):
    d=[0.0]
    for i in range(1,len(L)): d.append(d[-1]+math.dist(L[i],L[i-1]))
    out=[]; k=0
    for i in range(n):
        t=d[-1]*i/(n-1)
        while k<len(L)-2 and d[k+1]<t: k+=1
        u=(t-d[k])/max(d[k+1]-d[k],1e-9)
        out.append((L[k][0]+(L[k+1][0]-L[k][0])*u, L[k][1]+(L[k+1][1]-L[k][1])*u))
    return out
def smooth(L, r, it):
    for _ in range(it):
        M=[L[0]]
        for i in range(1,len(L)-1):
            a=max(0,i-r); b=min(len(L)-1,i+r); w=min(i-a,b-i); a=i-w; b=i+w
            M.append((sum(p[0] for p in L[a:b+1])/(b-a+1), sum(p[1] for p in L[a:b+1])/(b-a+1)))
        M.append(L[-1]); L=M
    return L
def project(L, q, first=0):
    # nearest place on the line: (arc length, signed distance off it, tangent). Past either end the line runs on straight.
    best=None; s0=0.0
    for i in range(len(L)-1):
        if i < first:
            s0+=math.dist(L[i],L[i+1]); continue
        ax,ay=L[i]; bx,by=L[i+1]; dx=bx-ax; dy=by-ay; ln=math.hypot(dx,dy)
        t=((q[0]-ax)*dx+(q[1]-ay)*dy)/(ln*ln)
        lo = -1e9 if i==first else 0.0; hi = 1e9 if i==len(L)-2 else 1.0
        t=min(max(t,lo),hi)
        cx=ax+dx*t; cy=ay+dy*t; d2=(q[0]-cx)**2+(q[1]-cy)**2
        if best is None or d2<best[0]:
            tx,ty=dx/ln,dy/ln
            best=(d2, s0+t*ln, -(q[0]-cx)*ty+(q[1]-cy)*tx, tx, ty)
        s0+=ln
    return best[1:]
N=140
L=resample(line,N)
L=smooth(L,4,2)
body=[(v[2],v[1]) for v in P if abs(v[0])<0.07]   # near the middle plane: the trunk, not the side fins
for it in range(4):
    L=resample(L,N)
    tot=sum(math.dist(L[i],L[i+1]) for i in range(N-1))
    acc=[[0.0,0.0,0] for _ in range(N)]
    for q in body[::3]:
        s,n,tx,ty=project(L,q)
        if abs(n)>0.11: continue   # fins far off the trunk do not pull the line
        i=int(round(min(max(s/tot,0),1)*(N-1)))
        acc[i][0]+=q[0]; acc[i][1]+=q[1]; acc[i][2]+=1
    M=[]
    for i in range(N):
        # the head end keeps the hand-drawn line: crest and jaw there are not the trunk
        if acc[i][2]>=4 and i < N*0.86: M.append((acc[i][0]/acc[i][2], acc[i][1]/acc[i][2]))
        else: M.append(L[i])
    L=smooth(M,5,3)
L=resample(L,N)
tot=sum(math.dist(L[i],L[i+1]) for i in range(N-1))
print('line length', tot)
out=[]; outn=[]
for v,nv in zip(P,Nn):
    s,n,tx,ty=project(L,(v[2],v[1]))
    px=(v[2]-zmin)*S+20; py=H-1-((v[1]-ymin)*S+20)
    if 500<px<750 and py>185+(px-600)*0.25:
        s,n,tx,ty=project(L,(v[2],v[1]),int(N*0.72))   # the long belly fin: it belongs to the head end, whatever coil its tips were near; it keeps its shape
    out.append((v[0], n, s - tot*0.5))
    # the normal's part in the plane, turned by the same angle
    nz, ny = nv[2], nv[1]
    outn.append((nv[0], -nz*ty+ny*tx, nz*tx+ny*ty))
ylo=min(o[1] for o in out)
out=[(o[0],o[1]-ylo,o[2]) for o in out]
g.write(pr['attributes']['POSITION'], out); g.write(pr['attributes']['NORMAL'], outn)
g.save(dst)
print('bounds', [min(o[c] for o in out) for c in range(3)], [max(o[c] for o in out) for c in range(3)])
