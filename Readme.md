![Game list](ICON0.PNG)

<I><b>HuE for PSP 0.70 by E (T.Kawamorita)</b> - PC Engine/TurboGrafx-16, PC Engine CD/TurboGrafx-CD/SuperGrafx Emulators.</I>


![Game list](doc/CROB.PNG)

![Game list](doc/CROB1.PNG)

![Game list](doc/CROB2.PNG)
[Castlevania : Rondo of Blood English Patch](https://mega.nz/file/Pf5gECCR#2QPcqY7t7do7h9PNjkDO9zaSzDyHpbP-K9SpayMDFjA)

### Play PC Engine CD ROM [HuE for PSP]

- Put the ROM in the Memory Stick (Castlevania Dracula X - Rondo of Blood in English Patch, the CD ROM is in copying it to ms0:/PSP/GAME/HuE/Roms/Castlevania Dracula X - Rondo of Blood in English Patch).

A file explorer will appear directly, at this point you have to look for the <B>TurboGrafx CD Super System Card (U) (v3.0) .pce</B> BIOS , select it and give O. Once the bios is executed, you will see that you get a error toc error or similar and we will be in the "boot" screen, to enter the emulator menu in HuE you have to press <B> L + R + SELECT </B> at the same time .
Go to the emulator menu, there you have to go down to cdrom select , and you will have to look for the TOC file , where you have all the loom of the mp3 and renamed isos (by default in the download put it in: PSP/GAME/HuE/Roms/Castlevania Dracula X - Rondo of Blood in English Patch). Hit RESET in the menu, and <B> START (RUN)</B> on the screen where it asks you to hit RUN.

## Change Log

(1) Sprite Priority bugfix
    (A) Y's 1 & 2
    (B) F1 Circus 92

(2) SuperGrafx support (incomplete)

Change (3) the state is saved
    Data so far can not be used.
    Save format there may be a change in the future is greater.

<Have not been able to>
Speed ​​of games that use an interrupt (a) raster
    Postponed because of a problem I thought and was able to speed up.

<Schedule>
Related fixes (1) Sound
Implementation of improvement (2) CDROM, save state support


-------------------------------------------------- -----------------------------
<Acknowledgements>
Has let me use a length of 10 as (1) font.
Was used as the reference software for a variety of sources (2) PSP.
    I do not remember what I was referring to the various reference too (-;
    Thanks to all the people who who exposes the soft!
Mr. (3) Ruka, we had to speed up the CPU core. Thanks!
I referred to by Mr uoSnes9x how to use the GPU (4) y. Thanks!
I referred a variety of documents (5). Thanks to those who will create a document!


-------------------------------------------------- -----------------------------
<Reference>
(Http://cgfm2.emuviews.com/) Mr. (1) Charles MacDonald
    sgxtech.txt
    pcetech.txt
    The implementation is sgxtech.txt SuperGrafx was particularly useful.

(Http://www.geocities.jp/team_zero_three/PC2E/index.html) Mr. (2) PC2E Ki
    Used to check whether the results are correct emulation
    Was performed with reference to various implementations PC2E source code.
    Helped to fix a bug in the (A) SATB DMA.
    Helped fix (B) sprite priority.

(3) HuGo source code
    Helped to fix a bug in the (A) SATB DMA.
    Helped fix (B) sprite priority.

Source of (4) fpce / xpce / HuE
    PCEngine Emulator is great it was based.
    Started from here!

(5) YAME 0.38
    Used to check whether the results are correct emulation

(6) PCEP 0.70
    I was allowed to use the implementation for the CDROM emulation.

PCEngine Emulator and Source (7) Other
