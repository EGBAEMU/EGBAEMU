# Gameboy Advance Simulator
Im Rahmen des ESCHO1 Abschlussrpojeektes hat unsere Gruppe einen Emulator für den Gabeboy Advance implementiert.

## Projekt Status
Der aktuelle Stand umfasst die folgenden Komponenten:
 
 - Die 2 Instruktionssätze, ARMv4 und Thumb
 - Die 4 DMA Controller
 - Die 4 verfügbaren Timer
 - Das LCD rendering
 - Interrupt abarbeitung

Damit können bereits die meisten Spiele problemlos emuliert werden. 
Als Referenz haben wir hier die _Super Mario Advance_ spiele genommen. 
Als offene, aber nicht essentielle Punkte verbleiben:

 - Die 6 Audiokanäle (Für 2 der Kanäle ist in entsprechendem Branch schon funktionsfähiger Anfang gemacht. Dieser ist allerdings nicht optimiert.)
 - Koprozessor Funktionaliäten
