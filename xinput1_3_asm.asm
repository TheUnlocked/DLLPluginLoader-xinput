.code
extern iImportFunctions:QWORD
DllMain_stub PROC
	jmp iImportFunctions[0*8]
DllMain_stub ENDP
XInputGetState PROC
	jmp iImportFunctions[1*8]
XInputGetState ENDP
XInputSetState PROC
	jmp iImportFunctions[2*8]
XInputSetState ENDP
XInputGetCapabilities PROC
	jmp iImportFunctions[3*8]
XInputGetCapabilities ENDP
XInputEnable PROC
	jmp iImportFunctions[4*8]
XInputEnable ENDP
XInputGetDSoundAudioDeviceGuids PROC
	jmp iImportFunctions[5*8]
XInputGetDSoundAudioDeviceGuids ENDP
XInputGetBatteryInformation PROC
	jmp iImportFunctions[6*8]
XInputGetBatteryInformation ENDP
XInputGetKeystroke PROC
	jmp iImportFunctions[7*8]
XInputGetKeystroke ENDP
end
