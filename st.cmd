#!bin/linux-x86/picoharp

dbLoadDatabase("dbd/picoharp.dbd")
picoharp_registerRecordDeviceDriver(pdbbase)
dbLoadRecords("test.db")

# Port, Event, Offset, CFDLevel0, CFDLevel1, CFDZeroX1, SyncDiv, Range
initPicoAsyn("PICO1", 101, 0, 300, 20, 11, 1, 3)

iocInit()

