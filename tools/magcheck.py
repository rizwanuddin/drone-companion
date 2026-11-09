#checks compass health
import socket, struct, time

def crc16(data, extra):
    crc = 0xFFFF
    for b in data + bytes([extra]):
        tmp = b ^ (crc & 0xFF)
        tmp = (tmp ^ (tmp << 4)) & 0xFF
        crc = ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF
    return crc

def heartbeat(seq):
    payload = struct.pack('<IBBBBB', 0, 18, 8, 0, 4, 3)
    hdr = bytes([9, 0, 0, seq, 250, 191, 0, 0, 0])
    c = crc16(hdr + payload, 50)
    return b'\xfd' + hdr + payload + struct.pack('<H', c)

def parse(buf):
    i = 0
    while i < len(buf):
        if buf[i] != 0xFD or i + 12 > len(buf):
            i += 1
            continue
        ln, inc = buf[i + 1], buf[i + 2]
        sysid, comp = buf[i + 5], buf[i + 6]
        msgid = buf[i + 7] | buf[i + 8] << 8 | buf[i + 9] << 16
        end = i + 10 + ln + 2 + (13 if inc & 1 else 0)
        if end > len(buf):
            break
        yield sysid, comp, msgid, buf[i + 10:i + 10 + ln]
        i = end

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(1.0)
dst = ('127.0.0.1', 14540)
sys_status = None
raw_imu = None
texts = []
seen = {}
t0 = time.time()
seq = 0
while time.time() - t0 < 12:
    s.sendto(heartbeat(seq & 0xFF), dst)
    seq += 1
    end = time.time() + 1
    while time.time() < end:
        try:
            data, _ = s.recvfrom(2048)
        except socket.timeout:
            break
        for sysid, comp, msgid, pl in parse(data):
            if sysid != 1:
                continue
            seen[msgid] = seen.get(msgid, 0) + 1
            if msgid == 253:
                texts.append(pl[1:51].split(b'\0')[0].decode('ascii','replace'))
            if msgid == 1:
                p = pl.ljust(31, b'\0')
                sys_status = struct.unpack_from('<III', p, 0)
            elif msgid == 27:
                p = pl.ljust(29, b'\0')
                raw_imu = struct.unpack_from('<hhh', p, 20)

print('message ids seen from FC:', sorted(seen))
if sys_status:
    present, enabled, health = sys_status
    for name, bit in (('gyro', 1), ('accel', 2), ('mag', 4), ('baro', 8), ('gps', 32)):
        print('%-5s present=%d enabled=%d healthy=%d' % (name, bool(present & bit), bool(enabled & bit), bool(health & bit)))
else:
    print('no SYS_STATUS received')
print('RAW_IMU mag (x,y,z):', raw_imu if raw_imu else 'not received')
print('STATUSTEXT:', texts if texts else 'none')
