#!bin/linux-x86/picoharp

dbLoadDatabase("dbd/picoharp.dbd")
picoharp_registerRecordDeviceDriver(pdbbase)
dbLoadRecords("test.db")
initPicoAsyn("PICO1", 101)

iocInit()

