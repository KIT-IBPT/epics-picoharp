# Builder definitions for picoharp

from iocbuilder import Device, Substitution
from iocbuilder.modules import asyn

class Picoharp(Device):
    Dependencies = (asyn.Asyn,)

    LibFileList = ['pico']
    DbdFileList = ['picoharp']

    def __init__(self,
            port, serial, event_fast, event_5s,
            buckets, bin_size, valid_samples, samples_per_bucket,
            turns_per_sec):
        self.__super.__init__()
        self.port       = port
        self.serial     = serial
        self.event_fast = event_fast
        self.event_5s   = event_5s
        self.buckets = buckets
        self.bin_size = bin_size
        self.valid_samples = valid_samples
        self.samples_per_bucket = samples_per_bucket
        self.turns_per_sec = turns_per_sec

    def InitialiseOnce(self):
        print 'scanPicoDevices'

    def Initialise(self):
        print 'initPicoAsyn(' \
            '"%(port)s", "%(serial)s", %(event_fast)d, %(event_5s)d, ' \
            '%(buckets)d, %(bin_size)d, %(valid_samples)d, ' \
            '%(samples_per_bucket)d, %(turns_per_sec)g)' % self.__dict__


class PicoharpDb(Substitution):
    Dependencies = (Picoharp,)
    TemplateFile = 'picoharp.db'
    Arguments = (
        'DEVICE', 'PORT', 'CURRENT', 'EVENT_FAST', 'EVENT_5S',
        'BUCKETS', 'BUCKETS_1', 'PROFILE')

class PicoharpInstance:
    pico_port = 0
    pico_event = 100

    @classmethod
    def alloc_port(cls):
        cls.pico_port += 1
        return cls.pico_port

    @classmethod
    def alloc_event(cls):
        cls.pico_event += 1
        return cls.pico_event

    def __init__(self, device, serial, current,
            buckets = 936, f_rev = None, f_rf = None):

        # Compute key picoharp parameters
        bin_size, valid_samples, samples_per_bucket, turns_per_sec = \
            compute_parameters(buckets, f_rev, f_rf)

        port = 'PICO%d' % self.alloc_port()
        event_fast = self.alloc_event()
        event_5s   = self.alloc_event()

        Picoharp(
            port, serial, event_fast, event_5s,
            buckets, bin_size, valid_samples, samples_per_bucket, turns_per_sec)

        PicoharpDb(
            DEVICE = device, PORT = port, CURRENT = current,
            EVENT_FAST = event_fast, EVENT_5S = event_5s,
            BUCKETS = buckets, BUCKETS_1 = buckets - 1,
            PROFILE = samples_per_bucket)


def compute_parameters(buckets, f_rev, f_rf):
    c = 299792458

    # Compute turn duration from specified frequency and bucket count (if
    # appropriate).  Default to nominal 500MHz if not specified.
    assert f_rev == None or f_rf == None, 'Can\'t specify both frequencies'
    if f_rev is None and f_rf is None:
        f_rf = c / 0.6          # Nominal 500 MHz for 60cm bunch spacing
    if f_rev is None:
        f_rev = f_rf / buckets
    duration = 1e12 / f_rev     # In picoseconds

    # Determine bin size such that turn fits into picoharp sample space.
    # The bin size is specified as a number in the range 0..7 corresponding to
    # an actual bin duration of 2^bin_size*4ps.
    for bin_size in range(8):
        bin_duration = 4 * 2**bin_size
        memory = 65536 * bin_duration
        if memory >= duration:
            break
    else:
        assert False, \
            'Storage ring orbit of %g us is too big for picoharp!' % (
                duration * 1e-6)

    # We have the bin size and bucket count, now we can work out how many
    # samples per revolution and the samples per bucket.
    valid_samples = int(round(duration / bin_duration))
    samples_per_bucket = valid_samples // buckets

    return bin_size, valid_samples, samples_per_bucket, f_rev
