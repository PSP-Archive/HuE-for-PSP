# Hello World for PSP
# 2005.04.30  created by nem 

		.set noreorder

		.text

		.extern xmain


##############################################################################


		.ent _start
		.weak _start
_start: 
      addiu    $sp, 0x10 
      sw      $ra, 0($sp)    
      sw      $s0, 4($sp) 
      sw      $s1, 8($sp) 

      la     $v0,_gp
     move   $gp,$v0

      move   $s0, $a0            # Save args 
      move   $s1, $a1 

      la     $a0, _main_thread_name   # Main thread setup 
      la      $a1, xmain 
      li      $a2, 0x20            # Priority 
      li      $a3, 0x40000         # Stack size 
      lui      $t0, 0x8000            # Attributes 
      jal      sceKernelCreateThread 
      move   $t1, $0 

      move   $a0, $v0            # Start thread 
      move   $a1, $s0 
      jal      sceKernelStartThread 
      move   $a2, $s1 

      lw      $ra, 0($sp) 
      lw      $s0, 4($sp) 
      lw      $s1, 8($sp) 
      move   $v0, $0 
      jr       $ra 
      addiu   $sp, 0x10 

_main_thread_name: 
      .asciiz   "user_main" 

##############################################################################


		.section	.lib.ent,"wa",@progbits
__lib_ent_top:
		.word 0
		.word 0x80000000
		.word 0x00010104
		.word __entrytable


		.section	.lib.ent.btm,"wa",@progbits
__lib_ent_bottom:
		.word	0


		.section	.lib.stub,"wa",@progbits
__lib_stub_top:


		.section	.lib.stub.btm,"wa",@progbits
__lib_stub_bottom:
		.word	0


##############################################################################

		.section	".xodata.sceModuleInfo","wa",@progbits

__moduleinfo:
		.byte	0,0,1,1

		.ascii	"USERPROG"		#up to 28 char
		.align	5

		.word	_gp
		.word	__lib_ent_top
		.word	__lib_ent_bottom
		.word	__lib_stub_top
		.word	__lib_stub_bottom

##############################################################################

		.section	.rodata.entrytable,"wa",@progbits
__entrytable:
		.word 0xD632ACDB
		.word 0xF01D73A7
		.word _start
		.word __moduleinfo
		.word 0



###############################################################################


	.macro	STUB_START	module,d1,d2

		.section	.rodata.stubmodulename
		.word	0
__stub_modulestr_\@:
		.asciz	"\module"
		.align	2

		.section	.lib.stub
		.word __stub_modulestr_\@
		.word \d1
		.word \d2
		.word __stub_idtable_\@
		.word __stub_text_\@

		.section	.rodata.stubidtable
__stub_idtable_\@:

		.section	.text.stub
__stub_text_\@:

	.endm


	.macro	STUB_END
	.endm


	.macro	STUB_FUNC	funcid,funcname

		.set push
		.set noreorder

		.section	.text.stub
		.weak	\funcname
\funcname:
		jr	$ra
		nop

		.section	.rodata.stubidtable
		.word	\funcid

		.set pop

	.endm

	STUB_START "sceDisplay",0x40010000,0x00110005 
	  STUB_FUNC 0x0e20f177,sceDisplaySetMode 
	  STUB_FUNC 0xdea197d4,sceDisplayGetMode 
	  STUB_FUNC 0xdba6c4c4,sceDisplayGetFramePerSec 
	  STUB_FUNC 0x7ed59bc4,sceDisplaySetHoldMode 
	  STUB_FUNC 0xa544c486,sceDisplaySetResumeMode 
	  STUB_FUNC 0x289d82fe,sceDisplaySetFrameBuf 
	  STUB_FUNC 0xeeda2e54,sceDisplayGetFrameBuf 
	  STUB_FUNC 0xb4f378fa,sceDisplayIsForeground 
	  STUB_FUNC 0x9c6eaad7,sceDisplayGetVcount 
	  STUB_FUNC 0x4d4e10ec,sceDisplayIsVblank 
	  STUB_FUNC 0x36cdfade,sceDisplayWaitVblank 
	  STUB_FUNC 0x8eb9ec49,sceDisplayWaitVblankCB 
	  STUB_FUNC 0x984c27e7,sceDisplayWaitVblankStart 
	  STUB_FUNC 0x46f186c3,sceDisplayWaitVblankStartCB 
	  STUB_FUNC 0x773dd3a3,sceDisplayGetCurrentHcount 
	  STUB_FUNC 0x210eab3a,sceDisplayGetAccumulatedHcount 
	  STUB_FUNC 0x31c4baa8,sceDisplaySetVblankCallback
	STUB_END 

	STUB_START   "sceCtrl",0x40010000,0x00040005
	STUB_FUNC   0x6a2774f3,sceCtrlSetSamplingCycle   # sceCtrlInit
	STUB_FUNC   0x1f4011e6,sceCtrlSetSamplingMode    # sceCtrlSetAnalogMode
	STUB_FUNC   0x1f803938,sceCtrlReadBufferPositive # sceCtrlRead
	STUB_FUNC   0x3a622550,sceCtrlPeekBufferPositive # sceCtrlPeek
	STUB_END

	STUB_START   "IoFileMgrForUser",0x40010000,0x000D0005 
	STUB_FUNC   0xb29ddf9c,sceIoDopen
	STUB_FUNC   0xe3eb004c,sceIoDread
	STUB_FUNC   0xeb092469,sceIoDclose
	STUB_FUNC   0x6a638d83,sceIoRead
	STUB_FUNC   0x42ec03ac,sceIoWrite
	STUB_FUNC   0x27eb27b8,sceIoLseek
	STUB_FUNC   0x810c4bc3,sceIoClose
	STUB_FUNC   0x109f50bc,sceIoOpen
	STUB_FUNC   0xF27A9C51,sceIoRemove
	STUB_FUNC   0x6A70004,sceIoMkdir
	STUB_FUNC   0x1117C65F,sceIoRmdir
	STUB_FUNC   0x54F5FB11,sceIoDevctl
	STUB_FUNC   0x779103A0,sceIoRename
	STUB_END

	STUB_START   "sceSuspendForUser",0x40000000,0x00020005 
	STUB_FUNC   0xEADB1BD7,"DisableSuspend" 
	STUB_FUNC   0x3AEE7261,"EnableSuspend" 
	STUB_END 

	STUB_START   "LoadExecForUser",0x40010000,0x20005 
	STUB_FUNC   0x5572A5F,sceKernelExitGame 
	STUB_FUNC   0x4AC57943,sceKernelRegisterExitCallback # sceExitSetCallback 
	STUB_END 

	STUB_START "scePower",0x40010000,0x00220005 
	  STUB_FUNC 0xefd3c963,scePowerTick 
	  STUB_FUNC 0xedc13fe5,scePowerGetIdleTimer 
	  STUB_FUNC 0x7f30b3b1,scePowerIdleTimerEnable 
	  STUB_FUNC 0x972ce941,scePowerIdleTimerDisable 
	  STUB_FUNC 0x27f3292c,scePowerBatteryUpdateInfo 
	  STUB_FUNC 0x87440f5e,scePowerIsPowerOnline 
	  STUB_FUNC 0x0afd0d8b,scePowerIsBatteryExist 
	  STUB_FUNC 0x1e490401,scePowerIsBatteryCharging 
	  STUB_FUNC 0xb4432bc8,scePowerGetBatteryChargingStatus 
	  STUB_FUNC 0xd3075926,scePowerIsLowBattery 
	  STUB_FUNC 0x2085d15d,scePowerGetBatteryLifePercent 
	  STUB_FUNC 0x8efb3fa2,scePowerGetBatteryLifeTime 
	  STUB_FUNC 0x28e12023,scePowerGetBatteryTemp 
	  STUB_FUNC 0x862ae1a6,scePowerGetBatteryElec 
	  STUB_FUNC 0x483ce86b,scePowerGetBatteryVolt 
	  STUB_FUNC 0xd6d016ef,scePowerLock 
	  STUB_FUNC 0xca3d34c1,scePowerUnlock 
	  STUB_FUNC 0xdb62c9cf,scePowerCancelRequest 
	  STUB_FUNC 0x7fa406dd,scePowerIsRequest 
	  STUB_FUNC 0x2b7c7cf4,scePowerRequestStandby 
	  STUB_FUNC 0xac32c9cc,scePowerRequestSuspend 
	  STUB_FUNC 0x3951af53,scePowerEncodeUBattery 
	  STUB_FUNC 0x0074ef9b,scePowerGetResumeCount 
	  STUB_FUNC 0x04b7766e,scePowerRegisterCallback 
	  STUB_FUNC 0xdfa8baf8,scePowerUnregisterCallback 
	  STUB_FUNC 0x843fbf43,scePowerSetCpuClockFrequency 
	  STUB_FUNC 0xb8d7b3fb,scePowerSetBusClockFrequency 
	  STUB_FUNC 0xfee03a2f,scePowerGetCpuClockFrequency 
	  STUB_FUNC 0x478fe6f5,scePowerGetBusClockFrequency 
	  STUB_FUNC 0xfdb5bfe9,scePowerGetCpuClockFrequencyInt 
	  STUB_FUNC 0xbd681969,scePowerGetBusClockFrequencyInt 
	  STUB_FUNC 0xb1a52c83,scePowerGetCpuClockFrequencyFloat 
	  STUB_FUNC 0x9badb3eb,scePowerGetBusClockFrequencyFloat 
	  STUB_FUNC 0x737486f2,scePowerSetClockFrequency 
	STUB_END 

	STUB_START	"sceAudio",0x40010000,0x00090005 
	  STUB_FUNC	0x136CAF51,sceAudioOutputBlocking       # sceAudio_0
	  STUB_FUNC	0xE2D56B2D,sceAudioOutputPanned         # sceAudio_1
	  STUB_FUNC	0x13F592BC,sceAudioOutputPannedBlocking # sceAudio_2
	  STUB_FUNC	0x5EC81C55,sceAudioChReserve            # sceAudio_3
	  STUB_FUNC	0x6FC46853,sceAudioChRelease            # sceAudio_4
	  STUB_FUNC	0xE9D97901,sceAudioGetChannelRestLen    # sceAudio_5
	  STUB_FUNC	0xCB2E439E,sceAudioSetChannelDataLen    # sceAudio_6
	  STUB_FUNC	0x95FD0C2D,sceAudioChangeChannelConfig  # sceAudio_7
	  STUB_FUNC	0xB7E1D8E7,sceAudioChangeChannelVolume  # sceAudio_8
	STUB_END

	STUB_START "UtilsForUser",0x40010000,0x00180005 
	  STUB_FUNC 0xbfa98062,sceKernelDcacheInvalidateRange 
	  STUB_FUNC 0xc8186a58,sceKernelUtilsMd5Digest 
	  STUB_FUNC 0x9e5c5086,sceKernelUtilsMd5BlockInit 
	  STUB_FUNC 0x61e1e525,sceKernelUtilsMd5BlockUpdate 
	  STUB_FUNC 0xb8d24e78,sceKernelUtilsMd5BlockResult 
	  STUB_FUNC 0x840259f1,sceKernelUtilsSha1Digest 
	  STUB_FUNC 0xf8fcd5ba,sceKernelUtilsSha1BlockInit 
	  STUB_FUNC 0x346f6da8,sceKernelUtilsSha1BlockUpdate 
	  STUB_FUNC 0x585f1c09,sceKernelUtilsSha1BlockResult 
	  STUB_FUNC 0xe860e75e,sceKernelUtilsMt19937Init 
	  STUB_FUNC 0x06fb8a63,sceKernelUtilsMt19937UInt 
	  STUB_FUNC 0x37fb5c42,sceKernelGetGPI 
	  STUB_FUNC 0x6ad345d7,sceKernelSetGPO 
	  STUB_FUNC 0x91e4f6a7,sceKernelLibcClock 
	  STUB_FUNC 0x27cc57f0,sceKernelLibcTime 
	  STUB_FUNC 0x71ec4271,sceKernelLibcGettimeofday 
	  STUB_FUNC 0x79d1c3fa,sceKernelDcacheWritebackAll 
	  STUB_FUNC 0xb435dec5,sceKernelDcacheWritebackInvalidateAll 
	  STUB_FUNC 0x3ee30821,sceKernelDcacheWritebackRange 
	  STUB_FUNC 0x34b9fa9e,sceKernelDcacheWritebackInvalidateRange 
	  STUB_FUNC 0x80001c4c,sceKernelDcacheProbe 
	  STUB_FUNC 0x16641d70,sceKernelDcacheReadTag 
	  STUB_FUNC 0x4fd31c9d,sceKernelIcacheProbe 
	  STUB_FUNC 0xfb05fad0,sceKernelIcacheReadTag 
	STUB_END 

	STUB_START  "sceDmac",0x40010000,0x00020005 
	STUB_FUNC	0x617f3fe6,sceDmacMemcpy 
	STUB_FUNC	0xd97f94d8,sceDmacTryMemcpy 
	STUB_END 

	STUB_START   "ThreadManForUser",0x40010000,0x00170005
	STUB_FUNC   0x446D8DE6,sceKernelCreateThread
	STUB_FUNC   0xF475845D,sceKernelStartThread
	STUB_FUNC   0xAA73C935,sceKernelExitThread
	STUB_FUNC   0x9ACE131E,sceKernelSleepThread
	STUB_FUNC   0x55C20A00,sceKernelCreateEventFlag
	STUB_FUNC   0xEF9E4C70,sceKernelDeleteEventFlag
	STUB_FUNC   0x1FB15A32,sceKernelSetEventFlag
	STUB_FUNC   0x812346E4,sceKernelClearEventFlag
	STUB_FUNC   0x402FCF22,sceKernelWaitEventFlag
	STUB_FUNC   0x82826F70,KernelPollCallbacks
	STUB_FUNC   0xE81CAF8F,sceKernelCreateCallback
	STUB_FUNC   0x278C0DF5,sceKernelWaitThreadEnd
	STUB_FUNC   0x9FA03CD3,sceKernelDeleteThread
	STUB_FUNC	0x293B45B8,sceKernelGetThreadId 
	STUB_FUNC	0xD6DA4BA1,sceKernelCreateSema 
	STUB_FUNC	0x28B6489C,sceKernelDeleteSema 
	STUB_FUNC	0x4E3A1105,sceKernelWaitSema 
	STUB_FUNC	0x58B1F937,sceKernelPollSema 
	STUB_FUNC	0x3F53E640,sceKernelSignalSema
	STUB_FUNC   0xceadeb47,sceKernelDelayThread 
	STUB_FUNC   0x9944f31f,sceKernelSuspendThread
	STUB_FUNC   0x75156e8f,sceKernelResumeThread
	STUB_FUNC   0xd59ead2f,sceKernelWakeupThread
	STUB_END

	STUB_START "sceGe_user",0x40010000,0x00110005 
	  STUB_FUNC 0x1f6752ad,sceGeEdramGetSize 	
	  STUB_FUNC 0xe47e40e4,sceGeEdramGetAddr 
	  STUB_FUNC 0xb77905ea,sceGeEdramSetAddrTranslation 
	  STUB_FUNC 0xdc93cfef,sceGeGetCmd 
	  STUB_FUNC 0x57c8945b,sceGeGetMtx 
	  STUB_FUNC 0x438a385a,sceGeSaveContext 
	  STUB_FUNC 0x0bf608fb,sceGeRestoreContext 
	  STUB_FUNC 0xab49e76a,sceGeListEnQueue 
	  STUB_FUNC 0x1c0d95a6,sceGeListEnQueueHead 
	  STUB_FUNC 0x5fb86ab0,sceGeListDeQueue 
	  STUB_FUNC 0xe0d68148,sceGeListUpdateStallAddr 
	  STUB_FUNC 0x03444eb4,sceGeListSync 
	  STUB_FUNC 0xb287bd61,sceGeDrawSync 
	  STUB_FUNC 0xb448ec0d,sceGeBreak 
	  STUB_FUNC 0x4c06e472,sceGeContinue 
	  STUB_FUNC 0xa4fc06a4,sceGeSetCallback 
	  STUB_FUNC 0x05db22ce,sceGeUnsetCallback 
	STUB_END 

	STUB_START	"InterruptManager",0x40000000,0x00090005
	STUB_FUNC	0xCA04A2B9,sceKernelRegisterSubIntrHandler
	STUB_FUNC	0xD61E6961,sceKernelReleaseSubIntrHandler
	STUB_FUNC	0xFB8E22EC,sceKernelEnableSubIntr
	STUB_FUNC	0x8A389411,sceKernelDisableSubIntr
	STUB_FUNC	0x5CB5A78B,sceKernelSuspendSubIntr
	STUB_FUNC	0x7860E0DC,sceKernelResumeSubIntr
	STUB_FUNC	0xFC4374B8,sceKernelIsSubInterruptOccurred
	STUB_FUNC	0xD2E8363F,QueryIntrHandlerInfo
	STUB_FUNC	0xEEE43F47,sceKernelRegisterUserSpaceIntrStack
	STUB_END


###############################################################################

	.text

	.end _start


