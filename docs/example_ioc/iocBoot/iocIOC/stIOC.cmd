epicsEnvSet "EPICS_CA_MAX_ARRAY_BYTES", '2000000'
epicsEnvSet "EPICS_TS_MIN_WEST", '0'


# Device initialisation
# ---------------------

dbLoadDatabase "dbd/IOC.dbd"
IOC_registerRecordDeviceDriver(pdbbase)

scanPicoDevices
initPicoAsyn("PICO1", "1013107", 101, 102, 936, 3, 58540, 62, 533818)


# Final ioc initialisation
# ------------------------
dbLoadRecords 'db/IOC.db'
iocInit
