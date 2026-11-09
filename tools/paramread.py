import socket, struct, time

def crc16(data, extra):
    crc = 0xFFFF
    for b in data + bytes([extra]):
        tmp = b ^ (crc & 0xFF)
        tmp = (tmp ^ (tmp << 4)) & 0xFF
        crc = ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF
    return crc

def pkt(seq, msgid, payload, extra):
    hdr = bytes([len(payload), 0, 0, seq & 0xFF, 250, 191]) + struct.pack('<I', msgid)[:3]
    return b'\xfd' + hdr + payload + struct.pack('<H', crc16(hdr + payload, extra))

def heartbeat(seq):
    return pkt(seq, 0, struct.pack('<IBBBBB', 0, 18, 8, 0, 4, 3), 50)

def param_req(seq, name):
    payload = struct.pack('<hBB16s', -1, 1, 1, name.encode())
    return pkt(seq, 20, payload, 214)

def parse(buf):
    i = 0
    while i < len(buf):
        if buf[i] != 0xFD or i + 12 > len(buf):
            i += 1
            continue
        ln, inc = buf[i + 1], buf[i + 2]
        sysid = buf[i + 5]
        msgid = buf[i + 7] | buf[i + 8] << 8 | buf[i + 9] << 16
        end = i + 10 + ln + 2 + (13 if inc & 1 else 0)
        if end > len(buf):
            break
        yield sysid, msgid, buf[i + 10:i + 10 + ln]
        i = end

names = ['COMPASS_DEV_ID', 'COMPASS_DEV_ID2', 'COMPASS_ORIENT', 'COMPASS_ENABLE', 'COMPASS_AUTO_ROT', 'SYSID_SW_TYPE']
want = set(names)
got = {}
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(0.5)
dst = ('127.0.0.1', 14540)
seq = 0
t0 = time.time()
while time.time() - t0 < 12 and len(got) < len(names):
    s.sendto(heartbeat(seq), dst); seq += 1
    for n in names:
        if n not in got:
            s.sendto(param_req(seq, n), dst); seq += 1
    end = time.time() + 1.5
    while time.time() < end:
        try:
            data, _ = s.recvfrom(2048)
        except socket.timeout:
            continue
        for sysid, msgid, pl in parse(data):
            if sysid == 1 and msgid == 22:
                p = pl.ljust(25, b'\0')
                val, cnt, idx = struct.unpack_from('<fHH', p, 0)
                name = p[8:24].split(b'\0')[0].decode()
                if name in want:
                    got[name] = val
for n in names:
    print('%-18s %s' % (n, got.get(n, 'no reply')))
