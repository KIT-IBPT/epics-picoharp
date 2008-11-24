#!bin/linux-x86/picoharp

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "1000000")

dbLoadDatabase("dbd/picoharp.dbd")
picoharp_registerRecordDeviceDriver(pdbbase)
dbLoadRecords("db/picoharp.db", "DEVICE=SR-DI-PICO-01")

# Port, Event, Offset, CFDLevel0, CFDLevel1, CFDZeroX0, CFDZeroX1, SyncDiv, Range
initPicoAsyn("PICO1", 101, 0, 300, 20, 10, 11, 1, 3)

iocInit()

