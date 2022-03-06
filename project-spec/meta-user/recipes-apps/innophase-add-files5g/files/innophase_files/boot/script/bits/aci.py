import argparse
import enum

class ACI(enum.IntEnum):
    VO = 3
    VI = 2
    BE = 0
    BK = 1

    def cmp(self, other):
        if self == other:
            return 0
        if self == ACI.BK:
            return -1
        if other == ACI.BK:
            return 1
        return self - other

    @classmethod
    def argparse_type(cls, value):
        if isinstance(value, int):
            return cls(value)
        try:
            aci = int(value)
            return cls(aci)
        except ValueError:
            pass
        try:
            for k in cls.__members__:
                if k.upper() == value.upper():
                    return cls.__members__[k]
        except Exception:
            pass
        keys = ', '.join([k for k in cls.__members__])
        raise argparse.ArgumentTypeError(f'unknown ACI {value!r}, expect one of {keys}')


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('ac', type=ACI.argparse_type)
    opt = ap.parse_args()
    print(opt.ac)
    print(opt.ac.cmp(ACI.BK))
    print(opt.ac.cmp(ACI.BE))
    print(opt.ac.cmp(ACI.VI))
    print(opt.ac.cmp(ACI.VO))
    pass
