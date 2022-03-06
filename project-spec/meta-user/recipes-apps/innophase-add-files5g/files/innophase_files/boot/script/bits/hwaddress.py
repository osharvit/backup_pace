import re

class HWAddress(bytes):
    def __new__(self, value):
        if len(value) != 6:
            raise ValueError('hardware address must be 6 bytes')
        return bytes.__new__(self, value)

    def __str__(self):
        return '-'.join('{:02x}'.format(c) for c in self)

    @classmethod
    def from_string(cls, string):
        '''This function parses a string as a mac-address. It accepts single
        or double hex digits separated with either colon, dash or period,
        and double hex digits without separators, leading and trailing
        white space is ignored.

        Return true on success, or false on malformed strings.

        Example input:
          a-b-c-d-e-f
          aa-bb-cc-dd-ee-ff
          aa:b:c:d:ee:ff
          aabbccddeeff
          aabb.ccdd.eeff

        Degenerate forms are also accepted:
          aabb-cc:dd.eef
        '''
        pattern = r'([0-9a-fA-F]{1,2})[-:.]?' * 5 + r'([0-9a-fA-F]{1,2})' + r'$'

        m = re.match(pattern, string.strip())
        if not m:
            raise ValueError('{!r} is not a valid hardware address'.format(string))
        return cls(bytes(int(x, 16) for x in m.groups()))

    @classmethod
    def to_string(cls, mac_bytes):
        return cls(mac_bytes)
    pass

def testvector():
    vector = [
        ('00:11:22:33:44:55', '00-11-22-33-44-55', b'\x00\x11"3DU'),
        ('0:1:2:3:4:5', '00-01-02-03-04-05', b'\x00\x01\x02\x03\x04\x05'),
        ('00-11-22-33-44-55', '00-11-22-33-44-55', b'\x00\x11"3DU'),
        ('0-1-2-3-4-5', '00-01-02-03-04-05', b'\x00\x01\x02\x03\x04\x05'),
        ('001122334455', '00-11-22-33-44-55', b'\x00\x11"3DU'),
        ('0011.2233.4455', '00-11-22-33-44-55', b'\x00\x11"3DU')
    ]

    for a, b, c in vector:
        m = HWAddress.from_string(a)
        assert m == c
        assert HWAddress(m) == c
        assert str(m) == b
        pass
    return

if __name__ == '__main__':
    testvector()
    pass
